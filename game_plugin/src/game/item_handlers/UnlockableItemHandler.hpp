#pragma once

#include <string>
#include <vector>

#include "game/ItemHandler.hpp"

namespace sr2ap {

[[nodiscard]] ItemHandlerPtr CreateUnlockableItemHandler(
    bool blockVanillaRewards, std::vector<std::string> managedItems);

}  // namespace sr2ap
