#pragma once
#include "game/ItemHandler.hpp"

namespace sr2ap {
class GameThreadDispatcher;

ItemHandlerPtr CreateLuaActivityUnlockHandler(GameThreadDispatcher& dispatcher);
}  // namespace sr2ap
