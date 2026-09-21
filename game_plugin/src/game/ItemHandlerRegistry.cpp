#include "ItemHandlerRegistry.hpp"

#include <utility>

#include "GameThreadDispatcher.hpp"
#include "item_handlers/CheatItemHandler.hpp"
#include "item_handlers/HookActivityUnlockItemHandler.hpp"
#include "item_handlers/LuaActivityUnlockItemHandler.hpp"
#include "item_handlers/MoneyItemHandler.hpp"
#include "item_handlers/NotorietyItemHandler.hpp"
#include "item_handlers/RespectItemHandler.hpp"
#include "item_handlers/UnlockableItemHandler.hpp"
#include "item_handlers/WeaponItemHandler.hpp"

namespace sr2ap {

ItemHandlers CreateItemHandlers(ItemHandlerConfiguration configuration,
                                GameThreadDispatcher& dispatcher,
                                RespectController& respectController) {
    ItemHandlers handlers;
    handlers.reserve(8);

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
    handlers.push_back(CreateLuaActivityUnlockHandler(dispatcher));

    if (!configuration.persistentItems.empty()) {
        handlers.push_back(CreateHookActivityUnlockItemHandler(
            dispatcher, std::move(configuration.persistentItems)));
    }

    handlers.push_back(CreateWeaponItemHandler(dispatcher));

    return handlers;
}

}  // namespace sr2ap
