#include "archive.hpp"
#include "modules/game_module_api.h"
#include "modules/archive_mount.h"
#include "exporter/raw_export.h"
#include "apex/adf/generated/adf_types.h"
#include "havok/generated/havok_types.h"
#include "platform/app_state.h"
#include "utils/lookup3.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

#include "modules/shared.hpp"


void normal_export(ApexAppState &, uint64_t);

// void export_anim(ApexAppState &, uint64_t, uint64_t, bool);

namespace {
    namespace fs = std::filesystem;
    std::mutex operation_mutex;
    std::once_flag initialize_once;

    fs::path installation_root(const fs::path &input) {
        if (input.empty()) throw std::invalid_argument("Game root is empty");
        auto root = fs::absolute(input).lexically_normal();
        if (root.filename().empty()) root = root.parent_path();
        if (root.filename() == "archives_win64" || root.filename() == "patch_win64" ||
            root.filename() == "feral_overrides" || root.filename() == "dlc_win64")
            root = root.parent_path();
        if (root.filename() == "data" && root.parent_path().filename() == "share")
            root = root.parent_path().parent_path();
        return root;
    }

    fs::path archive_data_root(const fs::path &root) {
        const auto linux_data = root / "share/data";
        return fs::is_directory(linux_data / "archives_win64") ? linux_data : root;
    }


    bool identified(const fs::path &root) {
        if (fs::is_regular_file(root / "bin/MadMax") || fs::is_regular_file(root / "MadMax.exe")) return true;
        for (const auto &marker_path: {root / "bin/steam_appid.txt", root / "steam_appid.txt"}) {
            std::ifstream marker(marker_path);
            uint32_t appid{};
            if (marker >> appid && appid == 234140) return true;
        }
        return false;
    }

    Modules::ProbeResult probe_root(const fs::path &path) {
        try {
            const auto root = installation_root(path);
            const auto base = archive_data_root(root) / "archives_win64";
            if (!identified(root)) return {0, "Missing MadMax executable or Steam app ID 234140"};
            const auto reason = MadMax::probe_archive_directory(base);
            return reason.empty()
                       ? Modules::ProbeResult{100, "Mad Max 2048-aligned TAB/ARC archives; raw extraction supported"}
                       : Modules::ProbeResult{0, reason};
        } catch (const std::exception &e) {
            return {-1, e.what()};
        }
    }

    uint32_t asset_hash(std::string_view asset) {
        if (asset.empty()) throw std::invalid_argument("Asset is empty");
        auto digits = asset;
        int base = 10;
        if (digits.starts_with("0x") || digits.starts_with("0X")) {
            digits.remove_prefix(2);
            base = 16;
        } else if (!std::all_of(digits.begin(), digits.end(), [](char c) { return c >= '0' && c <= '9'; })) {
            // The installed 0x800 archives use the basename, not the full virtual path.
            // const auto last = asset.find_last_of("/\\");
            // const auto name = asset.substr(last == std::string_view::npos ? 0 : last + 1);
            // if (name.empty()) throw std::invalid_argument("Asset name is empty");
            return hashlittle(asset.data(), asset.size(), 0);
        }
        uint32_t hash{};
        const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), hash, base);
        if (parsed.ec != std::errc{} || parsed.ptr != digits.data() + digits.size())
            throw std::invalid_argument("Invalid 32-bit asset hash");
        return hash;
    }

    class Session final : public Modules::GameSession {
        AssetDB db;

    public:
        explicit Session(ApexAppState &app) : db(app.database_path) {
            mount_game_archives(app);
        }

        void extract(ApexAppState &app, std::string_view asset) override {
            if (asset.empty()) throw std::invalid_argument("Asset is empty");
            ActiveDatabase active(operation_mutex, db);
            const auto hash = asset_hash(asset);
            if (!app.manager().has(hash)) throw std::runtime_error("Asset not found: " + std::string(asset));
            if (app.extract_raw) raw_export(app, hash);
            else normal_export(app, hash);
        }
    };

    class Module final : public Modules::GameModule {
    public:
        [[nodiscard]] std::string_view id() const noexcept override { return "mad-max"; }
        [[nodiscard]] std::string_view name() const noexcept override { return "Mad Max"; }
        [[nodiscard]] Modules::ProbeResult probe(const fs::path &root) const override { return probe_root(root); }

        std::unique_ptr<Modules::GameSession> open(ApexAppState &app) const override {
            const auto result = probe(app.game_root());
            if (result.score <= 0) throw std::invalid_argument(result.reason);
            std::call_once(initialize_once, [] {
                init_adf_type_info();
                HavokTypes::init_havok_type_info();
            });
            return std::make_unique<Session>(app);
        }
    } api;
}

extern "C" APEX_MODULE_EXPORT const Modules::GameModule *APEX_MODULE_CALL apex_game_module_v2(uint32_t abi) {
    return abi == APEX_GAME_MODULE_ABI ? &api : nullptr;
}
