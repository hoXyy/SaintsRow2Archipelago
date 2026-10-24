#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "game/DeathSuppression.hpp"
#include "game/GameState.hpp"
#include "game/ItemHandlerManager.hpp"
#include "game/controllers/DeathLinkController.hpp"
#include "progression/ProgressionEvent.hpp"
#include "sr2ap/src/ffi/session_ffi.rs.h"
#include "util/GameEvents.hpp"

namespace sr2ap {
class ActivityListMonitor;
class DeathMonitor;
class SaveRevisionMonitor;

class SessionRuntime {
   public:
    SessionRuntime(bool gameSupported,
                   const std::filesystem::path& revisionJournalPath,
                   const ModuleInfo* game, GameThreadDispatcher& dispatcher);

    bool InstallSaveMonitoring(SaveRevisionMonitor& monitor, bool enabled);
    bool InstallDeathMonitoring(DeathMonitor& monitor, bool enabled);
    bool InstallListMonitoring(ActivityListMonitor& monitor, bool enabled);
    void Connect(std::uint16_t port);
    void ProcessGameEvents();
    void PollNetwork(const GameContext& context);
    void UpdateReadiness(GameReadiness current);
    void SendProgression(const ProgressionEvent& event);
    bool CommunicationsActive() const noexcept;
    rust::ConnectionState ClientConnectionState() const noexcept;
    std::optional<rust::SessionNotification> NextNotification();
    void UpdateItemHandlers();
    // Applies received DeathLink deaths; returns the one applied now.
    std::optional<DeathLinkController::Death> UpdateDeathLink(
        const GameContext& context);
    // Debug hotkey: acts as if another player died.
    void QueueTestDeathLink();
    void Shutdown();

   private:
    bool InstallPolicies(const rust::SessionRequest& request);
    bool ActivateItem(std::string_view name, ItemCompletion completion);
    void ShutdownItemHandlers();
    void ResetDeathLink();

    std::shared_ptr<GameEventQueue> events_;
    ::rust::Box<rust::SessionRuntime> session_;
    ItemHandlerManager itemHandlers_;
    std::shared_ptr<DeathSuppression> deathSuppression_;
    DeathLinkController deathLink_;
    DeathMonitor* deathMonitor_{};
    bool gameSupported_;
    bool saveMonitoring_{};
    bool gameplayLoaded_{};
};
}  // namespace sr2ap
