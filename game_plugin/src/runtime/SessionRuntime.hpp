#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include "game/GameState.hpp"
#include "game/ItemHandlerManager.hpp"
#include "progression/ProgressionEvent.hpp"
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
    void PollNetwork(const GameContext& context);
    void UpdateReadiness(GameReadiness current);
    void SendProgression(const ProgressionEvent& event);
    bool CommunicationsActive() const noexcept;
    void UpdateItemHandlers();
    void Shutdown();

   private:
    bool InstallPolicies(const rust::SessionRequest& request);
    bool ActivateItem(std::string_view name);
    void ShutdownItemHandlers();

    ::rust::Box<rust::SessionRuntime> session_;
    ItemHandlerManager itemHandlers_;
    bool gameSupported_;
    bool saveMonitoring_{};
};
}  // namespace sr2ap
