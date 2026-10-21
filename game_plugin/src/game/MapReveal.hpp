#pragma once

#include <cstdint>
#include <memory>

#include "game/GameState.hpp"

namespace sr2ap {

class GameThreadDispatcher;

// Uncovers the whole pause map: clears the fog on every neighborhood and
// reveals every map icon, so the taxi can go anywhere from the start.
class MapReveal {
   public:
    explicit MapReveal(GameThreadDispatcher& dispatcher);
    ~MapReveal();

    MapReveal(const MapReveal&) = delete;
    MapReveal& operator=(const MapReveal&) = delete;
    MapReveal(MapReveal&&) = delete;
    MapReveal& operator=(MapReveal&&) = delete;

    bool Install();
    void Remove();
    // Plugin thread, once per loop: queues a sweep on the game thread now and
    // then while gameplay is interactive. Triggers come and go (activities
    // and Chop Shop lists replace theirs), so one sweep is not enough.
    void Update(const GameContext& context);

   private:
    struct Implementation;
    GameThreadDispatcher& dispatcher_;
    std::shared_ptr<Implementation> implementation_;
    std::uint64_t nextSweep_{};
};

}  // namespace sr2ap
