#include "ConsumableItemHandler.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <string_view>

#include "fmt/format.h"
#include "game/GameThreadDispatcher.hpp"
#include "game/ItemHandler.hpp"
#include "game/Lua.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
struct ConsumableItem {
    std::string_view itemName;
    std::string_view drug;
};

constexpr std::array<ConsumableItem, 18> supportedItems{{
    // Food
    {"The Fist", "the_fist"},
    {"The Twins", "burgera"},
    {"Chicken Bazooms", "chicken_bazooms"},
    {"Hot Gyro", "hot_gyro"},
    {"Lil' Gyro", "lil_gyro"},
    {"Kabob", "steaming_kabob"},
    {"Belly Jelly Donut", "belly_jelly_donut"},
    {"Brown Donut", "brown_donut"},
    {"Donut Holes", "donut_holes"},
    {"Strange Egg Roll", "EggRoll"},
    {"Sum Old Guy", "sum_old_guy"},
    {"Sum Yung Guy", "sum_yung_guy"},

    // Drinks
    {"Big Swallow", "large_softdrink"},
    {"Grecian Juice", "grecian_juice"},
    {"Coffee", "coffee"},
    {"Dong Tea", "dong_tea"},

    // Booze
    {"40 oz.", "40oz"},

    // Weed
    {"Blunt", "blunt"},
}};

const ConsumableItem* FindItem(const std::string_view itemName) noexcept {
    const auto found = std::ranges::find_if(
        supportedItems,
        [itemName](const auto& item) { return item.itemName == itemName; });
    return found == supportedItems.end() ? nullptr : &*found;
}

class ConsumableItemHandler final : public ItemHandler {
   public:
    explicit ConsumableItemHandler(GameThreadDispatcher& dispatcher)
        : dispatcher_(dispatcher) {
    }

    [[nodiscard]] bool CanHandleItem(
        const std::string_view itemName) const noexcept override {
        return FindItem(itemName) != nullptr;
    }

    [[nodiscard]] bool Install() override {
        return Lua::CanGiveConsumable();
    }

    [[nodiscard]] bool Handle(const std::string_view itemName,
                              ItemCompletion completion) override {
        const auto* const item = FindItem(itemName);
        if (!item) {
            return false;
        }

        const auto queued = dispatcher_.Dispatch(
            [drug = std::string{item->drug}, itemName = std::string{itemName},
             completion] {
                if (!completion.CanApply()) {
                    return;
                }
                const bool given = Lua::GiveConsumable(drug.c_str());
                if (!given) {
                    LogWarning(
                        "Consumable",
                        fmt::format("Failed to trigger item={}", itemName));
                }
                completion.Complete(given);
            });

        if (!queued) {
            LogWarning("Consumable",
                       fmt::format("Failed to queue item={}", itemName));
        }

        return queued;
    }

    void Remove() override {
    }

   private:
    GameThreadDispatcher& dispatcher_;
};
}  // namespace

ItemHandlerPtr CreateConsumableItemHandler(GameThreadDispatcher& dispatcher) {
    return std::make_unique<ConsumableItemHandler>(dispatcher);
}
}  // namespace sr2ap
