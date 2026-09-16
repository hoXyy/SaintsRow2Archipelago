#pragma once

#include "game/ItemHandler.hpp"

namespace sr2ap {
class GameThreadDispatcher;

ItemHandlerPtr CreateNotorietyItemHandler(GameThreadDispatcher& dispatcher);
}  // namespace sr2ap
