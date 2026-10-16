#pragma once

#include <string>
#include <vector>

#include "game/ItemHandler.hpp"

namespace sr2ap {
class GameThreadDispatcher;

[[nodiscard]] ItemHandlerPtr CreateUnlockableItemHandler(
    GameThreadDispatcher& dispatcher, bool blockVanillaRewards,
    std::vector<std::string> managedItems);

}  // namespace sr2ap
