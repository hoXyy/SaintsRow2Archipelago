#include "SessionRuntime.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

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
    const auto managedCheats = Strings(request.managed_cheats);
    const auto managedUnlockables = Strings(request.managed_unlockables);
    const bool supported =
        std::all_of(managedCheats.begin(), managedCheats.end(),
                    [](const auto& name) {
                        return CheatController::SupportsItem(name);
                    }) &&
        std::all_of(managedUnlockables.begin(), managedUnlockables.end(),
                    [](const auto& name) {
                        return UnlockableController::SupportsItem(name);
                    });
    if (!supported) {
        LogWarning("Session",
                   "Rejected session containing unsupported managed items");
        return false;
    }

    const bool needsGameThread =
        !managedCheats.empty() || request.notoriety_traps;
    const bool cheatsInstalled =
        needsGameThread && cheats_.Install(managedCheats);
    const bool notorietyInstalled =
        request.notoriety_traps && notoriety_.Install();
    respectInstalled_ =
        request.exclusive_respect && saveMonitoring_ && respect_.Install();
    const bool unlockablesInstalled =
        !managedUnlockables.empty() &&
        unlockables_.Install(request.block_vanilla_unlockables,
                             managedUnlockables);

    const bool failed = (needsGameThread && !cheatsInstalled) ||
                        (request.notoriety_traps && !notorietyInstalled) ||
                        (request.exclusive_respect && !respectInstalled_) ||
                        (!managedUnlockables.empty() && !unlockablesInstalled);
    if (failed) {
        ShutdownControllers();
    }
    return !failed;
}

bool SessionRuntime::ActivateItem(std::string_view name) {
    return cheats_.ActivateReceivedItem(name) ||
           notoriety_.ActivateReceivedItem(name, cheats_) ||
           respect_.ActivateReceivedItem(name) ||
           unlockables_.QueueReceivedItem(name);
}

void SessionRuntime::ShutdownControllers() {
    unlockables_.Remove();
    respect_.Remove();
    notoriety_.Remove();
    cheats_.Remove();
    respectInstalled_ = false;
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
                              respect_.PermitNextSaveRestore(threadId);
                          });
    rust::session_set_save_monitoring(*session_, saveMonitoring_);
    return saveMonitoring_;
}

void SessionRuntime::Connect(std::uint16_t port) {
    rust::session_connect(*session_, port);
}

void SessionRuntime::PollNetwork() {
    for (;;) {
        const auto request = rust::session_next_request(
            *session_, IsGameplayInteractive(GetGameReadiness()));
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
            ShutdownControllers();
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
            *session_, static_cast<std::uint8_t>(event.kind),
            {reinterpret_cast<const std::uint8_t*>(event.key.data()),
             event.key.size()},
            event.previous, event.current);
    } catch (const ::rust::Error& error) {
        LogError("Session",
                 std::string{"Could not send progression: "} + error.what());
    }
}

bool SessionRuntime::CommunicationsActive() const noexcept {
    return rust::session_active(*session_);
}

void SessionRuntime::UpdateControllers() {
    if (respectInstalled_) {
        respect_.Update();
    }
}

void SessionRuntime::Shutdown() {
    rust::session_shutdown(*session_);
    ShutdownControllers();
}
}  // namespace sr2ap
