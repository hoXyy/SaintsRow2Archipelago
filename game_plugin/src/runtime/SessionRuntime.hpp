#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

#include "game/GameState.hpp"
#include "game/ItemHandlerManager.hpp"
#include "progression/ProgressionEvent.hpp"
#include "sr2ap/src/ffi/session_ffi.rs.h"
#include "util/GameEvents.hpp"

namespace sr2ap {
class SaveRevisionMonitor;

class SessionRuntime {
   public:
    SessionRuntime(bool gameSupported,
                   const std::filesystem::path& revisionJournalPath,
                   const ModuleInfo* game);

    bool InstallSaveMonitoring(SaveRevisionMonitor& monitor, bool enabled);
    void Connect(std::uint16_t port);
    void ProcessGameEvents();
    void PollNetwork(const GameContext& context);
    void UpdateReadiness(GameReadiness current);
    void SendProgression(const ProgressionEvent& event);
    bool CommunicationsActive() const noexcept;
    void UpdateItemHandlers();
    void Shutdown();

   private:
    bool InstallPolicies(const rust::SessionRequest& request);
    bool ActivateItem(std::string_view name, ItemCompletion completion);
    void ShutdownItemHandlers();

    std::shared_ptr<GameEventQueue> events_;
    ::rust::Box<rust::SessionRuntime> session_;
    ItemHandlerManager itemHandlers_;
    bool gameSupported_;
    bool saveMonitoring_{};
    bool gameplayLoaded_{};
};
}  // namespace sr2ap
