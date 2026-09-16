#pragma once

#include <string>
#include <vector>

#include "ItemHandler.hpp"

namespace sr2ap {
class GameThreadDispatcher;
class RespectController;

struct ItemHandlerConfiguration {
    std::vector<std::string> managedCheats;
    std::vector<std::string> managedUnlockables;
    bool notorietyTraps{};
    bool exclusiveRespect{};
    bool blockVanillaUnlockables{};
};

[[nodiscard]] ItemHandlers CreateItemHandlers(
    ItemHandlerConfiguration configuration, GameThreadDispatcher& dispatcher,
    RespectController& respectController);

}  // namespace sr2ap
