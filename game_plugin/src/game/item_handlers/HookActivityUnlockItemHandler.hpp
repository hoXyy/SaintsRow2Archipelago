#pragma once

#include <string>

#include "game/GameThreadDispatcher.hpp"
#include "game/ItemHandler.hpp"

namespace sr2ap {
ItemHandlerPtr CreateHookActivityUnlockItemHandler(
    GameThreadDispatcher& dispatcher, std::vector<std::string> managedItems);
}