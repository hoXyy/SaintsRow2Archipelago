#pragma once

#include <string>
#include <vector>

#include "game/ItemHandler.hpp"

namespace sr2ap {
class GameThreadDispatcher;

[[nodiscard]] ItemHandlerPtr CreateWeatherItemHandler(
    GameThreadDispatcher& dispatcher, std::vector<std::string> managedItems);

}  // namespace sr2ap
