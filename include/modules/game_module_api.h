#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// Host and modules are built together with the same C++ toolchain/runtime.
#define APEX_GAME_MODULE_ABI 2u
#define APEX_GAME_MODULE_ENTRY "apex_game_module_v2"
#if defined(_WIN32)
#define APEX_MODULE_CALL __cdecl
#if defined(APEX_BUILD_GAME_MODULE)
#define APEX_MODULE_EXPORT __declspec(dllexport)
#else
#define APEX_MODULE_EXPORT
#endif
#else
#define APEX_MODULE_CALL
#define APEX_MODULE_EXPORT __attribute__((visibility("default")))
#endif

class ApexAppState;

namespace Modules {
    struct ProbeResult {
        int score; // 1..100 supported, 0 unsupported, -1 probe failure.
        std::string reason;
    };

    class GameSession {
    public:
        virtual ~GameSession() = default;
        virtual void extract(ApexAppState &state, std::string_view asset) = 0;
        virtual void animation(ApexAppState &state, std::string_view skeleton, std::string_view asset) {
            throw std::runtime_error("Selected module does not support animation export");
        }
    };

    class GameModule {
    public:
        virtual ~GameModule() = default;
        [[nodiscard]] virtual std::string_view id() const noexcept = 0;
        [[nodiscard]] virtual std::string_view name() const noexcept = 0;
        [[nodiscard]] virtual ProbeResult probe(const std::filesystem::path &root) const = 0;
        virtual std::unique_ptr<GameSession> open(ApexAppState &state) const = 0;
        [[nodiscard]] virtual std::vector<std::string> search(const std::filesystem::path &database, std::string_view query) const {
            throw std::runtime_error("Selected game module does not support database search");
        }
    };
}

// Borrowed module singleton. Keep the library loaded until all returned C++
// objects (including sessions) are destroyed. Only symbol lookup uses C linkage.
using ApexGetGameModule = const Modules::GameModule *(APEX_MODULE_CALL *)(uint32_t requested_abi);
extern "C" APEX_MODULE_EXPORT const Modules::GameModule *APEX_MODULE_CALL apex_game_module_v2(uint32_t requested_abi);
