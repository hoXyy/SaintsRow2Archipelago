#pragma once

#include <memory>
#include <optional>
#include <string>

#include "game/DeathSuppression.hpp"
#include "game/GameState.hpp"
#include "util/GameEvents.hpp"

namespace sr2ap {
class GameThreadDispatcher;

// Kills the player for deaths received through DeathLink, once gameplay is
// interactive. Deaths received together collapse into one kill.
class DeathLinkController {
   public:
    struct Death {
        // The player who died; empty when unknown.
        std::string source;
        // The message their game sent, e.g. "X died trying to swim in lava";
        // may be empty.
        std::string cause;
    };

    DeathLinkController(GameThreadDispatcher& dispatcher,
                        std::shared_ptr<GameEventQueue> events,
                        std::shared_ptr<DeathSuppression> suppression);
    ~DeathLinkController();

    DeathLinkController(const DeathLinkController&) = delete;
    DeathLinkController& operator=(const DeathLinkController&) = delete;

    bool Install();
    void Remove();
    // Plugin thread.
    void Receive(std::string source, std::string cause);
    // Plugin thread, once per loop. Returns the death applied this time.
    std::optional<Death> Update(const GameContext& context);
    // Plugin thread: the world changed, so pending deaths no longer apply.
    void Reset();

   private:
    struct Implementation;
    GameThreadDispatcher& dispatcher_;
    std::shared_ptr<GameEventQueue> events_;
    std::shared_ptr<DeathSuppression> suppression_;
    std::unique_ptr<Implementation> implementation_;
};
}  // namespace sr2ap
