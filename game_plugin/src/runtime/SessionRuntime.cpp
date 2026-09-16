#include "SessionRuntime.hpp"

#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "game/GameState.hpp"
#include "game/SaveRevisionMonitor.hpp"
#include "rust/cxx.h"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
std::vector<std::string> Strings(const ::rust::Vec<::rust::String>& values) {
    std::vector<std::string> result;
    result.reserve(values.size());
    for (const auto& value : values) {
        result.emplace_back(value.data(), value.size());
    }
    return result;
}
}  // namespace

SessionRuntime::SessionRuntime(bool gameSupported,
                               const std::filesystem::path& path)
    : session_{rust::session_new(
          gameSupported,
          {reinterpret_cast<const std::uint16_t*>(path.native().data()),
           path.native().size()})},
      gameSupported_{gameSupported} {
}

bool SessionRuntime::InstallPolicies(const rust::SessionRequest& request) {
    ItemHandlerConfiguration configuration{
        .managedCheats = Strings(request.managed_cheats),
        .managedUnlockables = Strings(request.managed_unlockables),
        .notorietyTraps = request.notoriety_traps,
        .exclusiveRespect = request.exclusive_respect,
        .blockVanillaUnlockables = request.block_vanilla_unlockables,
    };
    if (!itemHandlers_.Install(std::move(configuration), saveMonitoring_)) {
        LogWarning("Session",
                   "Could not install item handlers for requested policies");
        return false;
    }
    return true;
}

bool SessionRuntime::ActivateItem(std::string_view name) {
    return itemHandlers_.Handle(name);
}

void SessionRuntime::ShutdownItemHandlers() {
    itemHandlers_.Remove();
}

bool SessionRuntime::InstallSaveMonitoring(SaveRevisionMonitor& monitor,
                                           bool enabled) {
    saveMonitoring_ = enabled && gameSupported_ &&
                      monitor.Install(
                          [this](std::uint32_t checksum) {
                              rust::session_save_loaded(*session_, checksum);
                          },
                          [this](std::uint32_t checksum) {
                              rust::session_save_written(*session_, checksum);
                          },
                          [this](std::uint32_t threadId) {
                              itemHandlers_.PermitNextSaveRestore(threadId);
                          });
    rust::session_set_save_monitoring(*session_, saveMonitoring_);
    return saveMonitoring_;
}

void SessionRuntime::Connect(std::uint16_t port) {
    rust::session_connect(*session_, port);
}

void SessionRuntime::PollNetwork(const GameContext& context) {
    for (;;) {
        const auto request = rust::session_next_request(
            *session_, IsGameplayInteractive(context.readiness));
        if (request.kind == rust::GameplayRequestKind::None) {
            return;
        }

        bool accepted = false;
        try {
            if (request.kind == rust::GameplayRequestKind::InstallPolicies) {
                accepted = InstallPolicies(request);
            } else {
                accepted =
                    ActivateItem({request.name.data(), request.name.size()});
            }
        } catch (const std::exception& error) {
            if (request.kind != rust::GameplayRequestKind::InstallPolicies) {
                throw;
            }
            ShutdownItemHandlers();
            LogError("Session",
                     std::string{"Gameplay request failed: "} + error.what());
        }
        rust::session_report_result(*session_, accepted);
    }
}

void SessionRuntime::UpdateReadiness(GameReadiness current) {
    rust::session_update_readiness(*session_,
                                   current == GameReadiness::MainMenu,
                                   IsGameplayLoaded(current));
}

void SessionRuntime::SendProgression(const ProgressionEvent& event) {
    try {
        rust::session_progression(
            *session_,
            {
                reinterpret_cast<const std::uint8_t*>(event.category.data()),
                event.category.size(),
            },
            {
                reinterpret_cast<const std::uint8_t*>(event.key.data()),
                event.key.size(),
            },
            event.previous, event.current);
    } catch (const ::rust::Error& error) {
        LogError("Session",
                 std::string{"Could not send progression: "} + error.what());
    }
}

bool SessionRuntime::CommunicationsActive() const noexcept {
    return rust::session_active(*session_);
}

void SessionRuntime::UpdateItemHandlers() {
    itemHandlers_.Update();
}

void SessionRuntime::Shutdown() {
    rust::session_shutdown(*session_);
    ShutdownItemHandlers();
}
}  // namespace sr2ap
