#pragma once

#include "game/ItemHandler.hpp"

namespace sr2ap {
class GameThreadDispatcher;
class RespectController;

[[nodiscard]] ItemHandlerPtr CreateRespectItemHandler(
    GameThreadDispatcher& dispatcher, RespectController& controller);

}  // namespace sr2ap
