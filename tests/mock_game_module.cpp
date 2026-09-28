#include "modules/game_module_api.h"
#include "platform/app_state.h"
#include <fstream>

#ifndef FIXTURE_ID
#define FIXTURE_ID "fixture-one"
#endif
namespace {
    struct Session final : Modules::GameSession {
        void extract(ApexAppState &state, std::string_view asset) override {
            if (asset == "throw") throw std::runtime_error("Fixture extract exception");
            std::filesystem::create_directories(state.export_path());
            std::ofstream stream(state.export_path() / (std::string(FIXTURE_ID) + ".txt"));
            stream << FIXTURE_ID << '\n' << asset << '\n' << state.extract_raw;
            if (!stream) throw std::runtime_error("Extract failed");
        }
    };
    class Module final : public Modules::GameModule {
    public:
        std::string_view id() const noexcept override { return FIXTURE_ID; }
        std::string_view name() const noexcept override { return "Test game module"; }
        Modules::ProbeResult probe(const std::filesystem::path &root) const override {
            const bool supported = std::filesystem::is_regular_file(root / (std::string(FIXTURE_ID) + ".game"));
            return {supported ? 100 : 0, supported ? "Fixture root supported" : "Fixture marker not found"};
        }
        std::unique_ptr<Modules::GameSession> open(ApexAppState &state) const override {
            if (probe(state.game_root()).score <= 0) throw std::runtime_error("Open failed");
            return std::make_unique<Session>();
        }
        std::vector<std::string> search(const std::filesystem::path &, std::string_view query) const override {
            return {std::string(query), std::string(4096, 'x')};
        }
    };
    const Module api;
}
extern "C" APEX_MODULE_EXPORT const Modules::GameModule *APEX_MODULE_CALL apex_game_module_v2(uint32_t requested_abi) {
#ifdef FIXTURE_BAD_ABI
    return nullptr;
#else
    return requested_abi == APEX_GAME_MODULE_ABI ? &api : nullptr;
#endif
}
