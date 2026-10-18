#pragma once

#include <functional>
#include <memory>

namespace sr2ap {

class GameThreadDispatcher {
   public:
    using Task = std::function<void()>;

    GameThreadDispatcher();
    ~GameThreadDispatcher();

    GameThreadDispatcher(const GameThreadDispatcher&) = delete;
    GameThreadDispatcher& operator=(const GameThreadDispatcher&) = delete;
    GameThreadDispatcher(GameThreadDispatcher&&) = delete;
    GameThreadDispatcher& operator=(GameThreadDispatcher&&) = delete;

    bool Install();
    void Remove();
    [[nodiscard]] bool IsInstalled() const noexcept;
    // Tasks run on the game thread once per game logic frame, in order.
    bool Dispatch(Task task);
    // Drops every queued task and waits for a batch that is already running,
    // so nothing captured by them is used afterwards. Not from the game
    // thread.
    void CancelPending();

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

}  // namespace sr2ap
