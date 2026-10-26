#include "SessionRuntime.hpp"

#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "game/ActivityListMonitor.hpp"
#include "game/DeathMonitor.hpp"
#include "game/GameState.hpp"
#include "game/JobLevelMonitor.hpp"
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

// Same names as the progression categories.
std::string_view CategoryName(const ActivityList activityList) {
    switch (activityList) {
        case ActivityList::Hitman:
            return "hitman";
        case ActivityList::ChopShop:
            return "chop_shop";
    }
    return {};
}

std::string_view CategoryName(const Job job) {
    switch (job) {
        case Job::Paramedic:
            return "paramedic";
        case Job::Firefighter:
            return "firefighter";
        case Job::Taxi:
            return "taxi";
        case Job::Towing:
            return "towing";
        case Job::Hoing:
            return "hoing";
    }
    return {};
}
}  // namespace

SessionRuntime::SessionRuntime(bool gameSupported,
                               const std::filesystem::path& path,
                               const ModuleInfo* game,
                               GameThreadDispatcher& dispatcher)
    : events_{std::make_shared<GameEventQueue>(
          [game] { return ReadGameContext(game).IsInteractive(); })},
      session_{rust::session_new(
          gameSupported,
          {reinterpret_cast<const std::uint16_t*>(path.native().data()),
           path.native().size()})},
      itemHandlers_{dispatcher},
      deathSuppression_{std::make_shared<DeathSuppression>()},
      deathLink_{dispatcher, events_, deathSuppression_},
      gameSupported_{gameSupported} {
}

bool SessionRuntime::InstallPolicies(const rust::SessionRequest& request) {
    ItemHandlerConfiguration configuration{
        .managedCheats = Strings(request.managed_cheats),
        .managedUnlockables = Strings(request.managed_unlockables),
        .persistentItems = Strings(request.persistent_items),
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

bool SessionRuntime::ActivateItem(std::string_view name,
                                  ItemCompletion completion) {
    return itemHandlers_.Handle(name, std::move(completion));
}

void SessionRuntime::ShutdownItemHandlers() {
    itemHandlers_.Remove();
}

bool SessionRuntime::InstallSaveMonitoring(SaveRevisionMonitor& monitor,
                                           bool enabled) {
    saveMonitoring_ = enabled && gameSupported_ &&
                      monitor.Install(
                          [events = events_](std::uint32_t checksum) {
                              events->PushSaveLoaded(checksum);
                          },
                          [events = events_](std::uint32_t checksum) {
                              events->PushSaveWritten(checksum);
                          },
                          [this](std::uint32_t threadId) {
                              itemHandlers_.PermitNextSaveRestore(threadId);
                          });
    rust::session_set_save_monitoring(*session_, saveMonitoring_);
    return saveMonitoring_;
}

bool SessionRuntime::InstallDeathMonitoring(DeathMonitor& monitor,
                                            bool enabled) {
    if (!enabled || !gameSupported_) {
        return false;
    }
    if (!deathLink_.Install()) {
        LogWarning("DeathLink", "Kills unavailable; received deaths ignored");
    }
    if (!monitor.Install(deathSuppression_,
                         [events = events_] { events->PushPlayerDied(); })) {
        return false;
    }
    deathMonitor_ = &monitor;
    return true;
}

bool SessionRuntime::InstallListMonitoring(ActivityListMonitor& monitor,
                                           bool enabled) {
    return enabled && gameSupported_ &&
           monitor.Install([events = events_](ActivityList activityList,
                                              std::uint32_t list) {
               events->PushListOpened(activityList, list);
           });
}

bool SessionRuntime::InstallJobMonitoring(JobLevelMonitor& monitor,
                                          bool enabled) {
    return enabled && gameSupported_ &&
           monitor.Install([events = events_](Job job, std::uint32_t level) {
               events->PushJobLevelCompleted(job, level);
           });
}

void SessionRuntime::Connect(std::uint16_t port) {
    rust::session_connect(*session_, port);
}

void SessionRuntime::ProcessGameEvents() {
    for (const auto& event : events_->Drain()) {
        try {
            switch (event.kind) {
                case GameEventKind::SaveLoaded:
                    ResetDeathLink();
                    rust::session_save_loaded(*session_, event.checksum);
                    break;
                case GameEventKind::SaveWritten:
                    rust::session_save_written(*session_, event.checksum);
                    break;
                case GameEventKind::RequestCompleted:
                    rust::session_complete_request(*session_, event.requestId,
                                                   event.applied);
                    break;
                case GameEventKind::PlayerDied:
                    rust::session_player_died(*session_);
                    break;
                case GameEventKind::ListOpened: {
                    const auto category = CategoryName(event.activityList);
                    rust::session_list_opened(
                        *session_,
                        {reinterpret_cast<const std::uint8_t*>(category.data()),
                         category.size()},
                        event.list);
                    break;
                }
                case GameEventKind::JobLevelCompleted:
                    // Only the finished level, never the levels before it:
                    // those were reported when they were finished.
                    SendProgression(
                        {.category = std::string{CategoryName(event.job)},
                         .key = "levels",
                         .previous = event.level - 1,
                         .current = event.level});
                    break;
            }
        } catch (const ::rust::Error& error) {
            LogError("Session", std::string{"Could not process game event: "} +
                                    error.what());
        }
    }
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
            switch (request.kind) {
                case rust::GameplayRequestKind::InstallPolicies:
                    accepted = InstallPolicies(request);
                    break;
                case rust::GameplayRequestKind::ResetPersistentItems:
                    accepted = itemHandlers_.ResetState();
                    break;
                case rust::GameplayRequestKind::ReplayPersistentItem:
                case rust::GameplayRequestKind::ActivateItem: {
                    const std::string_view name{request.name.data(),
                                                request.name.size()};
                    if (!ActivateItem(name,
                                      ItemCompletion{events_, request.id})) {
                        LogDebug("Session", "No handler could start item " +
                                                std::string{name});
                    }
                    continue;
                }
                case rust::GameplayRequestKind::None:
                    return;
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
    // Leaving gameplay (quit to menu, world reload) ends the world that any
    // still-queued item was for.
    const bool loaded = IsGameplayLoaded(current);
    if (gameplayLoaded_ && !loaded) {
        events_->InvalidateWorld();
        ResetDeathLink();
    }
    gameplayLoaded_ = loaded;

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

rust::ConnectionState SessionRuntime::ClientConnectionState() const noexcept {
    return rust::session_connection_state(*session_);
}

std::optional<rust::SessionNotification> SessionRuntime::NextNotification() {
    auto notification = rust::session_next_notification(*session_);
    if (notification.kind == rust::NotificationKind::None) {
        return std::nullopt;
    }
    return notification;
}

void SessionRuntime::UpdateItemHandlers() {
    itemHandlers_.Update();
}

std::optional<DeathLinkController::Death> SessionRuntime::UpdateDeathLink(
    const GameContext& context) {
    for (;;) {
        const auto death = rust::session_next_death_link(*session_);
        if (!death.present) {
            break;
        }
        deathLink_.Receive(
            std::string{death.source.data(), death.source.size()},
            std::string{death.cause.data(), death.cause.size()});
    }
    return deathLink_.Update(context);
}

void SessionRuntime::QueueTestDeathLink() {
    LogInfo("DeathLink", "Test death queued by hotkey");
    deathLink_.Receive("", "Test death from the DeathLink hotkey");
}

void SessionRuntime::ResetDeathLink() {
    deathLink_.Reset();
    if (deathMonitor_) {
        deathMonitor_->Reset();
    }
}

void SessionRuntime::Shutdown() {
    rust::session_shutdown(*session_);
    ShutdownItemHandlers();
    deathLink_.Remove();
}
}  // namespace sr2ap
