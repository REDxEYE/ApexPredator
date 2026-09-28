#include "modules/game_module_api.h"
#include "platform/app_state.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

#include <charconv>
#include <mutex>
#include <vector>

#include "apex/asset_db.h"
#include "apex/package/tab_archive.h"
#include "apex/adf/generated/adf_types.h"
#include "havok/generated/havok_types.h"
#include "modules/shared.hpp"
#include "utils/murmur3.h"

void raw_export(ApexAppState &, uint64_t);

void normal_export(ApexAppState &, uint64_t);

void export_anim(ApexAppState &, uint64_t, uint64_t, bool);

// Rage 2 supports raw archive extraction and game-specific asset conversion.
namespace {
    std::mutex operation_mutex;
    std::once_flag initialize_once;

    constexpr uint32_t steam_app_id = 548570;

    uint64_t asset_hash(const std::string_view asset) {
        if (asset.empty()) throw std::invalid_argument("Asset is empty");
        std::string text(asset);
        std::ranges::replace(text, '\\', '/');
        uint32_t value{};
        std::string_view digits(text);
        int base = 10;
        if (digits.starts_with("0x") || digits.starts_with("0X")) {
            digits.remove_prefix(2);
            base = 16;
        } else if (!std::ranges::all_of(digits, [](char c) { return c >= '0' && c <= '9'; }))
            return asset_path_hash(text);
        const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), value, base);
        if (parsed.ec != std::errc{} || parsed.ptr != digits.data() + digits.size())
            throw std::invalid_argument(
                "Invalid 32-bit asset hash");
        return value;
    }

    std::filesystem::path archive_root(const std::filesystem::path &path) {
        if (path.empty()) throw std::invalid_argument("Game root is empty");
        auto root = std::filesystem::absolute(path).lexically_normal();
        // Remove a trailing separator before looking beside archives_win64 for
        // the executable/Steam marker; parent_path() otherwise returns itself.
        while (root.has_relative_path() && root.filename().empty()) root = root.parent_path();
        if (std::filesystem::is_directory(root / "archives_win64")) root /= "archives_win64";
        return root;
    }

    bool game_identity(const std::filesystem::path &root) {
        for (const auto &directory: {root, root.parent_path()}) {
            if (std::filesystem::is_regular_file(directory / "RAGE2.exe")) return true;
            std::ifstream appid(directory / "steam_appid.txt");
            if (uint32_t value{}; appid >> value && value == steam_app_id) return true;
        }
        return false;
    }

    Modules::ProbeResult probe_root(const std::filesystem::path &path) {
        try {
            const auto root = archive_root(path);
            if (!std::filesystem::is_directory(root / "initial")) {
                return {0, "Missing initial archive directory"};
            }
            if (!game_identity(root)) {
                return {0, "No RAGE2.exe or steam_appid.txt identifying app 548570 beside the archive root"};
            }
            size_t archives = 0;
            for (const auto &part: {"initial", "optional", "supplemental"}) {
                const auto folder = root / part;
                if (!std::filesystem::is_directory(folder)) continue;
                // Rage 2 also stores language archives in nested directories.
                for (const auto &entry: std::filesystem::recursive_directory_iterator(folder)) {
                    if (!entry.is_regular_file() || entry.path().extension() != ".tab") continue;
                    std::array<unsigned char, 32> header{};
                    std::ifstream tab(entry.path(), std::ios::binary);
                    tab.read(reinterpret_cast<char *>(header.data()), header.size());
                    auto arc = entry.path();
                    arc.replace_extension(".arc");
                    if (!tab || std::memcmp(header.data(), "TAB\0", 4) != 0 ||
                        header[4] != 3 || header[5] != 0 || header[6] != 1 || header[7] != 0 ||
                        !std::filesystem::is_regular_file(arc)) {
                        return {0, "Unsupported or incomplete Rage 2 archive (expected TAB 3.1 with matching ARC)"};
                    }
                    ++archives;
                }
            }
            if (!archives) {
                return {0, "No TAB archives found"};
            }
            return {100, "Rage 2 installation with TAB 3.1 archives; AMF model/mesh conversion and raw extraction supported"};
        } catch (const std::exception &error) {
            return {-1, error.what()};
        } catch (...) {
            return {-1, "Rage 2 root probe failed"};
        }
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
        export_anim(app, asset_hash(skeleton), asset_hash(asset), app.root_motion);
    }

    class Module final : public Modules::GameModule {
    public:
        [[nodiscard]] std::string_view id() const noexcept override { return "rage2"; }
        [[nodiscard]] std::string_view name() const noexcept override { return "Rage 2"; }
        [[nodiscard]] Modules::ProbeResult probe(const std::filesystem::path &root) const override { return probe_root(root); }
        std::unique_ptr<Modules::GameSession> open(ApexAppState &app) const override {
            const auto result = probe(app.game_root());
            if (result.score <= 0) throw std::invalid_argument(result.reason);
            std::call_once(initialize_once, [] {
            init_adf_type_info();
            init_havok_type_info();
            // ADF::register_custom_types();
        });
            return std::make_unique<Session>(app);
        }
        [[nodiscard]] std::vector<std::string> search(const std::filesystem::path &database, std::string_view query) const override {
            AssetDB db(database);
            ActiveDatabase active(operation_mutex, db);
            std::vector<std::string> results;
            db.files_search(query, results);
            return results;
        }
    };
    const Module api;
}

extern "C" APEX_MODULE_EXPORT const Modules::GameModule *APEX_MODULE_CALL apex_game_module_v2(uint32_t requested_abi) {
    return requested_abi == APEX_GAME_MODULE_ABI ? &api : nullptr;
}
