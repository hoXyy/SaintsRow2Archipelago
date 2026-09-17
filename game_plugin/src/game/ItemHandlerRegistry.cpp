#include "ItemHandlerRegistry.hpp"

#include <utility>

#include "GameThreadDispatcher.hpp"
#include "item_handlers/CheatItemHandler.hpp"
#include "item_handlers/MoneyItemHandler.hpp"
#include "item_handlers/NotorietyItemHandler.hpp"
#include "item_handlers/RespectItemHandler.hpp"
#include "item_handlers/UnlockableItemHandler.hpp"

namespace sr2ap {

ItemHandlers CreateItemHandlers(ItemHandlerConfiguration configuration,
                                GameThreadDispatcher& dispatcher,
                                RespectController& respectController) {
    ItemHandlers handlers;
    handlers.reserve(5);

    if (!configuration.managedCheats.empty()) {
        handlers.push_back(CreateCheatItemHandler(
            dispatcher, std::move(configuration.managedCheats)));
    }
    if (configuration.notorietyTraps) {
        handlers.push_back(CreateNotorietyItemHandler(dispatcher));
    }
    if (configuration.exclusiveRespect) {
        handlers.push_back(CreateRespectItemHandler(respectController));
    }
    if (!configuration.managedUnlockables.empty()) {
        handlers.push_back(CreateUnlockableItemHandler(
            configuration.blockVanillaUnlockables,
            std::move(configuration.managedUnlockables)));
    }

    handlers.push_back(CreateMoneyItemHandler(dispatcher));

    return handlers;
}

}  // namespace sr2ap
