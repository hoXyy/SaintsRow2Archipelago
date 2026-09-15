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
    bool Dispatch(Task task);

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

}  // namespace sr2ap
