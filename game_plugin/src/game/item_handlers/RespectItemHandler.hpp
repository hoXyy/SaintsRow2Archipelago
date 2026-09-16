#pragma once

#include "game/ItemHandler.hpp"

namespace sr2ap {
class RespectController;

[[nodiscard]] ItemHandlerPtr CreateRespectItemHandler(
    RespectController& controller);

}  // namespace sr2ap
