#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include "features/Cheats.hpp"
#include "features/Notoriety.hpp"
#include "features/Respect.hpp"
#include "features/Unlockables.hpp"
#include "game/GameState.hpp"
#include "progression/ProgressionEventSink.hpp"
#include "sr2ap/src/ffi/session_ffi.rs.h"

namespace sr2ap {
class SaveRevisionMonitor;

// Called only by the plugin thread. The save monitor must be removed before
// destruction.
class SessionRuntime {
   public:
    SessionRuntime(bool gameSupported,
                   const std::filesystem::path& revisionJournalPath);

    bool InstallSaveMonitoring(SaveRevisionMonitor& monitor, bool enabled);
    void Connect(std::uint16_t port);
    void PollNetwork();
    void UpdateReadiness(GameReadiness current);
    void SendProgression(const ProgressionEvent& event);
    bool CommunicationsActive() const noexcept;
    void UpdateControllers();
    void Shutdown();

   private:
    bool InstallPolicies(const rust::SessionRequest& request);
    bool ActivateItem(std::string_view name);
    void ShutdownControllers();

    ::rust::Box<rust::SessionRuntime> session_;
    CheatController cheats_;
    NotorietyController notoriety_;
    RespectController respect_;
    UnlockableController unlockables_;
    bool gameSupported_;
    bool saveMonitoring_{};
    bool respectInstalled_{};
};
}  // namespace sr2ap
