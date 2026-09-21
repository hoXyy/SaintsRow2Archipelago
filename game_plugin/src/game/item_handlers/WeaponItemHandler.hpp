#pragma once

#include "game/GameThreadDispatcher.hpp"
#include "game/ItemHandler.hpp"

namespace sr2ap {
[[nodiscard]] ItemHandlerPtr CreateWeaponItemHandler(
    GameThreadDispatcher& dispatcher);
}