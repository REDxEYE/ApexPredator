#pragma once
#include "modules/game_module_api.h"
#include "platform/app_state.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Modules {
    std::filesystem::path executable_directory();

    std::string utf8(const std::filesystem::path &path);

    std::filesystem::path normalize_path(const std::filesystem::path &path);

    class Library {
    public:
        explicit Library(const std::filesystem::path &path);

        ~Library();

        Library(const Library &) = delete;

        Library &operator=(const Library &) = delete;

        [[nodiscard]] const GameModule &api() const { return *api_; }
        [[nodiscard]] const std::filesystem::path &path() const { return path_; }

        int probe(const std::filesystem::path &root, std::string &reason) const;

    private:
        void *handle_{};
        const GameModule *api_{};
        std::filesystem::path path_;
    };

    class Registry {
    public:
        // Extra directories augment the executable's modules directory.
        explicit Registry(const std::vector<std::filesystem::path> &directories = {});

        std::shared_ptr<Library> select(const std::string &id_or_path,
                                        const std::filesystem::path &game_root = {});

        [[nodiscard]] const std::vector<std::shared_ptr<Library> > &libraries() const { return libraries_; }
        [[nodiscard]] const std::vector<std::string> &diagnostics() const { return diagnostics_; }

    private:
        std::vector<std::shared_ptr<Library> > libraries_;
        std::vector<std::string> diagnostics_;
    };

    class Session {
    public:
        Session(std::shared_ptr<Library> library, const std::filesystem::path &root,
                const std::filesystem::path &database, const std::filesystem::path &output, bool skip_textures);

        ~Session();

        Session(const Session &) = delete;

        Session &operator=(const Session &) = delete;

        void extract(const std::string &asset, bool raw);

        void animation(const std::string &skeleton, const std::string &asset, bool root_motion);

    private:
        // Keep the library alive until the session destructor has destroyed the last session object.
        std::shared_ptr<Library> library_;
        ApexAppState state_;
        std::unique_ptr<GameSession> session_;
    };
}
