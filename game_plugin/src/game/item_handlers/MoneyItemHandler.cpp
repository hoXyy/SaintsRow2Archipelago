#include "MoneyItemHandler.hpp"

#include <algorithm>
#include <array>
#include <string_view>

#include "fmt/format.h"
#include "game/GameThreadDispatcher.hpp"
#include "game/Lua.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
struct MoneyItem {
    std::string_view itemName;
    std::int32_t amount;
};

constexpr std::array<MoneyItem, 6> supportedItems{
    {{.itemName = "$1,000", .amount = 1000},
     {.itemName = "$5,000", .amount = 5000},
     {.itemName = "$10,000", .amount = 10000},
     {.itemName = "$25,000", .amount = 25000},
     {.itemName = "$50,000", .amount = 50000},
     {.itemName = "$100,000", .amount = 100000}}};

const MoneyItem* FindItem(const std::string_view itemName) noexcept {
    const auto found = std::ranges::find_if(
        supportedItems,
        [itemName](const auto& trap) { return trap.itemName == itemName; });
    return found == supportedItems.end() ? nullptr : &*found;
}

class MoneyItemHandler : public ItemHandler {
   public:
    explicit MoneyItemHandler(GameThreadDispatcher& dispatcher)
        : dispatcher_(dispatcher) {
    }

    [[nodiscard]] bool CanHandleItem(
        const std::string_view itemName) const noexcept override {
        return FindItem(itemName) != nullptr;
    }

    [[nodiscard]] bool Install() override {
        return Lua::CanGiveMoney();
    }

    [[nodiscard]] bool Handle(const std::string_view itemName) override {
        const auto* const item = FindItem(itemName);
        if (!item) {
            return false;
        }

        const auto queued = dispatcher_.Dispatch(
            [amount = item->amount, itemName = std::string{itemName}] {
                if (!Lua::GiveMoney(amount)) {
                    LogWarning("Money", fmt::format("Failed to trigger item={}",
                                                    itemName));
                }
            });

        if (!queued) {
            LogWarning("Money",
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

ItemHandlerPtr CreateMoneyItemHandler(GameThreadDispatcher& dispatcher) {
    return std::make_unique<MoneyItemHandler>(dispatcher);
}
}  // namespace sr2ap