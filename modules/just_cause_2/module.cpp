#include "modules/game_module_api.h"
#include "platform/app_state.h"

#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>

#include "apex/asset_db.h"
#include "modules/shared.hpp"

void raw_export(ApexAppState &, uint64_t);

void normal_export(ApexAppState &, uint64_t);

// void export_anim(ApexAppState &, uint64_t, uint64_t, bool);

namespace {
    namespace fs = std::filesystem;
    std::mutex operation_mutex;

    fs::path installation_root(const fs::path &path) {
        if (path.empty()) throw std::invalid_argument("Game root is empty");
        auto root = fs::absolute(path).lexically_normal();
        while (root.has_relative_path() && root.filename().empty()) root = root.parent_path();
        if (root.filename() == "archives_win32") root = root.parent_path();
        return root;
    }


    Modules::ProbeResult probe_root(const fs::path &path) {
        try {
            const auto root = installation_root(path);
            if (!fs::is_regular_file(root / "JustCause2.exe")) return {0, "Missing JustCause2.exe"};
            if (!fs::is_directory(root / "archives_win32")) return {0, "Missing archives_win32"};
            return {100, std::format("Just Cause 2 TAB archives")};
        } catch (const std::exception &e) {
            return {0, e.what()};
        }
    }

    uint32_t asset_hash(std::string_view asset) {
        if (asset.empty()) throw std::invalid_argument("Asset is empty");
        uint32_t hash{};
        auto digits = asset;
        int base = 10;
        if (digits.starts_with("0x") || digits.starts_with("0X")) {
            digits.remove_prefix(2);
            base = 16;
        } else if (!std::all_of(digits.begin(), digits.end(), [](char c) { return c >= '0' && c <= '9'; })) {
            std::string path(asset);
            std::replace(path.begin(), path.end(), '\\', '/');
            return hashlittle(path.data(), path.size(), 0);
        }
        const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), hash, base);
        if (parsed.ec != std::errc{} || parsed.ptr != digits.data() + digits.size())
            throw std::invalid_argument("Invalid 32-bit asset hash");
        return hash;
    }


    struct Session final : Modules::GameSession {
        AssetDB db;

        explicit Session(ApexAppState &app) : db(app.database_path) {
            app.mount_archives();
        }
        void extract(ApexAppState &app, std::string_view asset) override;
        void animation(ApexAppState &app, std::string_view skeleton, std::string_view asset) override;
    };

    void Session::extract(ApexAppState &app, std::string_view asset) {
        if (asset.empty()) throw std::invalid_argument("Asset is empty");
        ActiveDatabase active(operation_mutex, db);
        const auto hash = asset_hash(asset);
        if (!app.manager().has(hash)) throw std::runtime_error("Asset not found: " + std::string(asset));
        if (app.extract_raw) raw_export(app, hash);
        else normal_export(app, hash);
    }

    void Session::animation(ApexAppState &app, std::string_view skeleton, std::string_view asset) {
        ActiveDatabase active(operation_mutex, db);
        app.models().reset();
        // export_anim(app, asset_hash(skeleton), asset_hash(asset), app.root_motion);
    }


    class Module final : public Modules::GameModule {
    public:
        [[nodiscard]] std::string_view id() const noexcept override { return "just-cause-2"; }
        [[nodiscard]] std::string_view name() const noexcept override { return "Just Cause 2"; }
        [[nodiscard]] Modules::ProbeResult probe(const fs::path &root) const override { return probe_root(root); }

        std::unique_ptr<Modules::GameSession> open(ApexAppState &app) const override {
            const auto result = probe(app.game_root());
            if (result.score <= 0) throw std::invalid_argument(result.reason);
            return std::make_unique<Session>(app);
        }
    };

    const Module api;
}

extern "C" APEX_MODULE_EXPORT const Modules::GameModule *APEX_MODULE_CALL apex_game_module_v2(uint32_t requested_abi) {
    return requested_abi == APEX_GAME_MODULE_ABI ? &api : nullptr;
}
