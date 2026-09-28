#include "modules/module_loader.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#include <mach-o/dyld.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace Modules {
    std::string utf8(const std::filesystem::path &path) {
        const auto value = path.u8string();
        return {reinterpret_cast<const char *>(value.data()), value.size()};
    }

    std::filesystem::path normalize_path(const std::filesystem::path &path) {
#ifndef _WIN32
        auto value = utf8(path);
        if (value.size() >= 3 && std::isalpha(static_cast<unsigned char>(value[0])) && value[1] == ':' && (
                value[2] == '\\' || value[2] == '/')) {
            std::replace(value.begin(), value.end(), '\\', '/');
            return "/mnt/" + std::string(1, static_cast<char>(std::tolower(static_cast<unsigned char>(value[0])))) +
                   value.substr(2);
        }
#endif
        return path;
    }

    std::filesystem::path executable_directory() {
#ifdef _WIN32
        std::vector<wchar_t> buffer(512);
        for (;;) {
            auto size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (!size) throw std::runtime_error("Cannot locate executable");
            if (size < buffer.size()) return std::filesystem::path(std::wstring(buffer.data(), size)).parent_path();
            buffer.resize(buffer.size() * 2);
        }
#elif defined(__APPLE__)
        uint32_t size = 0; _NSGetExecutablePath(nullptr, &size);
        std::vector<char> buffer(size);
        if (_NSGetExecutablePath(buffer.data(), &size)) throw std::runtime_error("Cannot locate executable");
        return std::filesystem::canonical(buffer.data()).parent_path();
#else
        return std::filesystem::read_symlink("/proc/self/exe").parent_path();
#endif
    }

    namespace {
        void unload(void *handle) {
            if (!handle) return;
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(handle));
#else
            dlclose(handle);
#endif
        }

        bool module_file(const std::filesystem::path &path) {
#ifdef _WIN32
            auto ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
            return ext == ".dll";
#elif defined(__APPLE__)
            return path.extension() == ".dylib" || path.extension() == ".so";
#else
            return path.extension() == ".so";
#endif
        }
    }

    Library::Library(const std::filesystem::path &path) : path_(std::filesystem::absolute(path)) {
#ifdef _WIN32
        handle_ = LoadLibraryExW(path_.c_str(), nullptr,
                                 LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!handle_) throw std::runtime_error(
            "Cannot load " + utf8(path_) + " (Windows error " + std::to_string(GetLastError()) + ")");
        auto entry = reinterpret_cast<ApexGetGameModule>(GetProcAddress(static_cast<HMODULE>(handle_),
                                                                        APEX_GAME_MODULE_ENTRY));
#else
        handle_ = dlopen(path_.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle_) throw std::runtime_error("Cannot load " + utf8(path_) + ": " + dlerror());
        auto entry = reinterpret_cast<ApexGetGameModule>(dlsym(handle_, APEX_GAME_MODULE_ENTRY));
#endif
        try {
            if (!entry) throw std::runtime_error("Missing " APEX_GAME_MODULE_ENTRY " in " + utf8(path_));
            api_ = entry(APEX_GAME_MODULE_ABI);
            if (!api_) throw std::runtime_error("Incompatible game module ABI: " + utf8(path_));
            if (api_->id().empty() || api_->name().empty())
                throw std::runtime_error("Incomplete game module API: " + utf8(path_));
        } catch (...) {
            unload(handle_);
            handle_ = nullptr;
            throw;
        }
    }

    Library::~Library() { unload(handle_); }

    int Library::probe(const std::filesystem::path &root, std::string &reason) const {
        try {
            auto result = api_->probe(normalize_path(root));
            if (result.score < -1 || result.score > 100)
                throw std::runtime_error("Invalid probe result from " + std::string(api_->id()));
            reason = std::move(result.reason);
            return result.score;
        } catch (const std::exception &error) {
            reason = error.what();
            return -1;
        }
    }

    Registry::Registry(const std::vector<std::filesystem::path> &directories) {
        auto paths = directories;
        paths.insert(paths.begin(), executable_directory() / "modules");
        std::vector<std::filesystem::path> candidates;
        std::unordered_set<std::string> seen;
        for (const auto &directory: paths) {
            if (!std::filesystem::exists(directory)) continue;
            if (!std::filesystem::is_directory(directory)) throw std::runtime_error(
                "Not a module directory: " + utf8(directory));
            for (const auto &entry: std::filesystem::directory_iterator(directory)) {
                if (!entry.is_regular_file() || !module_file(entry.path())) continue;
                auto path = std::filesystem::canonical(entry.path());
                if (seen.insert(utf8(path)).second) candidates.push_back(path);
            }
        }
        std::ranges::sort(candidates);
        for (const auto &path: candidates) {
            std::shared_ptr<Library> library;
            try { library = std::make_shared<Library>(path); } catch (const std::exception &error) {
                diagnostics_.emplace_back(error.what());
                continue;
            }
            for (const auto &existing: libraries_)
                if (std::string(existing->api().id()) == library->api().id())
                    throw std::runtime_error(
                        "Duplicate module id '" + std::string(library->api().id()) + "': " + utf8(existing->path()) +
                        " and " + utf8(path));
            libraries_.push_back(std::move(library));
        }
    }

    std::shared_ptr<Library> Registry::select(const std::string &id_or_path, const std::filesystem::path &game_root) {
        std::shared_ptr<Library> selected;
        if (!id_or_path.empty()) {
            auto path = std::filesystem::path(id_or_path);
            if (path.has_parent_path() || path.has_extension()) selected = std::make_shared<Library>(path);
            else for (const auto &library: libraries_) if (id_or_path == library->api().id()) selected = library;
            if (!selected) throw std::runtime_error("Game module not found: " + id_or_path);
            if (!game_root.empty()) {
                if (std::string reason; selected->probe(game_root, reason) <= 0) throw std::runtime_error(
                    "Module '" + id_or_path + "' does not support this root: " + reason);
            }
            return selected;
        }
        if (game_root.empty()) {
            if (libraries_.size() == 1) return libraries_.front();
            throw std::runtime_error("Select a module with --module for a command without a game root");
        }
        int best = 0;
        std::vector<std::string> matches;
        std::ostringstream reasons;
        for (const auto &library: libraries_) {
            std::string reason;
            const auto score = library->probe(game_root, reason);
            reasons << "\n  " << library->api().id() << ": " << reason;
            if (score > best) {
                best = score;
                selected = library;
                matches = {std::string(library->api().id())};
            } else if (score > 0 && score == best) matches.emplace_back(library->api().id());
        }
        if (matches.size() > 1) {
            std::string message = "Multiple modules support this root; select --module:";
            for (const auto &id: matches) message += " " + id;
            throw std::runtime_error(message);
        }
        if (!selected) {
            for (const auto &diagnostic: diagnostics_) reasons << "\n  " << diagnostic;
            throw std::runtime_error("No installed game module supports this root." + reasons.str());
        }
        return selected;
    }

    Session::Session(std::shared_ptr<Library> library, const std::filesystem::path &root,
                     const std::filesystem::path &database, const std::filesystem::path &output, bool skip_textures)
        : library_(std::move(library)), state_(normalize_path(root)) {
        state_.database_path = normalize_path(database);
        if (!output.empty()) state_.export_path(normalize_path(output));
        state_.skip_textures = skip_textures;
        session_ = library_->api().open(state_);
        if (!session_) throw std::runtime_error("Could not open game session");
    }

    Session::~Session() = default;

    void Session::extract(const std::string &asset, bool raw) {
        state_.extract_raw = raw;
        session_->extract(state_, asset);
    }

    void Session::animation(const std::string &skeleton, const std::string &asset, bool root_motion) {
        state_.root_motion = root_motion;
        session_->animation(state_, skeleton, asset);
    }
}
