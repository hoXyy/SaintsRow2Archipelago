#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "util/GameEvents.hpp"

namespace sr2ap {
class GameThreadDispatcher;

class UnlockableController {
   public:
    UnlockableController();
    ~UnlockableController();

    UnlockableController(const UnlockableController&) = delete;
    UnlockableController& operator=(const UnlockableController&) = delete;

    bool Install(bool blockVanillaRewards,
                 const std::vector<std::string>& managedItems);
    void Remove();
    bool QueueReceivedItem(std::string_view itemName,
                           GameThreadDispatcher& dispatcher,
                           ItemCompletion completion);
    void Update(GameThreadDispatcher& dispatcher);
    [[nodiscard]] static bool SupportsItem(std::string_view itemName);

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};
}  // namespace sr2ap
