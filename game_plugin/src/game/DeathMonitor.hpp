#pragma once

#include <functional>
#include <memory>

#include "game/DeathSuppression.hpp"
#include "game/GameState.hpp"

namespace sr2ap {
// Reports the local player's deaths through a hook on the game's "human dies"
// routine. Reports at most one death per life: after reporting it waits until
// the player has been alive for a while again.
class DeathMonitor {
   public:
    // Game thread.
    using Callback = std::function<void()>;

    DeathMonitor();
    ~DeathMonitor();
    DeathMonitor(const DeathMonitor&) = delete;
    DeathMonitor& operator=(const DeathMonitor&) = delete;

    bool Install(std::shared_ptr<DeathSuppression> suppression,
                 Callback playerDied);
    // Plugin thread, once per loop.
    void Poll(const GameContext& context);
    // Plugin thread: the world changed (save load, quit to menu).
    void Reset();
    void Remove();

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};
}  // namespace sr2ap
