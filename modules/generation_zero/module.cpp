#include "modules/game_module_api.h"
#include "platform/app_state.h"
#include "apex/asset_db.h"
#include "apex/adf/generated/adf_types.h"
#include "apex/adf/adf_custom_types.hpp"
#include "havok/generated/havok_types.h"
#include "utils/hash_helper.h"
#include <array>
#include <cstring>
#include <fstream>
#include <mutex>
#include <charconv>

#include "modules/shared.hpp"

void raw_export(ApexAppState &, uint64_t);

void normal_export(ApexAppState &, uint64_t);

void export_anim(ApexAppState &, uint64_t, uint64_t, bool);

namespace {
    std::mutex operation_mutex;
    std::once_flag initialize_once;

    std::filesystem::path archive_root(const std::filesystem::path &path) {
        if (path.empty()) throw std::invalid_argument("Game root is empty");
        auto root = std::filesystem::absolute(path).lexically_normal();
        if (std::filesystem::is_directory(root / "archives_win64")) root /= "archives_win64";
        return root;
    }

    bool game_identity(const std::filesystem::path &root) {
        for (const auto &directory: {root, root.parent_path()}) {
            if (std::filesystem::is_regular_file(directory / "GenerationZero_F.exe")) return true;
            std::ifstream appid(directory / "steam_appid.txt");
            uint32_t value{};
            if (appid >> value && value == 704270) return true;
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
                return {0, "No GenerationZero_F.exe or steam_appid.txt identifying app 704270 beside the archive root"};
            }
            size_t count = 0;
            for (const auto &part: {"initial", "optional", "supplemental"}) {
                const auto folder = root / part;
                if (!std::filesystem::is_directory(folder)) continue;
                for (const auto &entry: std::filesystem::directory_iterator(folder)) {
                    if (entry.path().extension().string() != ".tab" || !entry.is_regular_file()) continue;
                    std::array<unsigned char, 12> header{};
                    std::ifstream tab(entry.path(), std::ios::binary);
                    tab.read(reinterpret_cast<char *>(header.data()), header.size());
                    auto arc = entry.path();
                    arc.replace_extension(".arc");
                    if (!tab || std::memcmp(header.data(), "TAB\0", 4) != 0 || header[4] != 2 || header[5] != 0 ||
                        header[6]
                        != 1 || header[7] != 0 ||
                        (entry.file_size() - header.size()) % 12 || !std::filesystem::is_regular_file(arc)) {
                        return {0, "Unsupported or incomplete TAB archive (expected Generation Zero TAB 2.1 with matching ARC)"};
                    }
                    ++count;
                }
            }
            if (!count) {
                return {0, "No TAB archives found"};
            }
            return {100, "Generation Zero installation with TAB 2.1 archives"};
        } catch (const std::exception &e) {
            return {-1, e.what()};
        } catch (...) {
            return {-1, "Game-root probe failed"};
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

    uint32_t asset_hash(std::string_view asset) {
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
        [[nodiscard]] std::string_view id() const noexcept override { return "generation-zero"; }
        [[nodiscard]] std::string_view name() const noexcept override { return "Generation Zero"; }
        [[nodiscard]] Modules::ProbeResult probe(const std::filesystem::path &root) const override { return probe_root(root); }
        std::unique_ptr<Modules::GameSession> open(ApexAppState &app) const override {
            const auto result = probe(app.game_root());
            if (result.score <= 0) throw std::invalid_argument(result.reason);
            if (app.database_path.empty()) throw std::invalid_argument("Database path is empty");
            std::call_once(initialize_once, [] {
                init_adf_type_info();
                init_havok_type_info();
                ADF::register_custom_types();
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
