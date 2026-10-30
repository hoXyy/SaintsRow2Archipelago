#include "ItemHandlerRegistry.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

#include "GameThreadDispatcher.hpp"
#include "controllers/WeatherController.hpp"
#include "item_handlers/CheatItemHandler.hpp"
#include "item_handlers/ConsumableItemHandler.hpp"
#include "item_handlers/HookActivityUnlockItemHandler.hpp"
#include "item_handlers/LuaActivityUnlockItemHandler.hpp"
#include "item_handlers/MoneyItemHandler.hpp"
#include "item_handlers/NotorietyItemHandler.hpp"
#include "item_handlers/RespectItemHandler.hpp"
#include "item_handlers/UnlockableItemHandler.hpp"
#include "item_handlers/WeaponItemHandler.hpp"
#include "item_handlers/WeatherItemHandler.hpp"

namespace sr2ap {

ItemHandlers CreateItemHandlers(ItemHandlerConfiguration configuration,
                                GameThreadDispatcher& dispatcher,
                                RespectController& respectController) {
    ItemHandlers handlers;
    handlers.reserve(10);

    // Weather items arrive with the cheats but are set without them.
    auto& managedCheats = configuration.managedCheats;
    const auto weather =
        std::ranges::stable_partition(managedCheats, [](const auto& name) {
            return !WeatherController::SupportsItem(name);
        });
    std::vector<std::string> managedWeather{
        std::make_move_iterator(weather.begin()),
        std::make_move_iterator(weather.end())};
    managedCheats.erase(weather.begin(), weather.end());

    if (!managedCheats.empty()) {
        handlers.push_back(
            CreateCheatItemHandler(dispatcher, std::move(managedCheats)));
    }
    if (!managedWeather.empty()) {
        handlers.push_back(
            CreateWeatherItemHandler(dispatcher, std::move(managedWeather)));
    }
    if (configuration.notorietyTraps) {
        handlers.push_back(CreateNotorietyItemHandler(dispatcher));
    }
    if (configuration.exclusiveRespect) {
        handlers.push_back(
            CreateRespectItemHandler(dispatcher, respectController));
    }
    if (!configuration.managedUnlockables.empty()) {
        handlers.push_back(CreateUnlockableItemHandler(
            dispatcher, configuration.blockVanillaUnlockables,
            std::move(configuration.managedUnlockables)));
    }

    handlers.push_back(CreateMoneyItemHandler(dispatcher));
    handlers.push_back(CreateLuaActivityUnlockHandler(dispatcher));

    if (!configuration.persistentItems.empty()) {
        handlers.push_back(CreateHookActivityUnlockItemHandler(
            dispatcher, std::move(configuration.persistentItems)));
    }

    handlers.push_back(CreateWeaponItemHandler(dispatcher));
    handlers.push_back(CreateConsumableItemHandler(dispatcher));

    return handlers;
}

}  // namespace sr2ap
