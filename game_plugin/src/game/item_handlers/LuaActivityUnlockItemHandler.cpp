#include "LuaActivityUnlockItemHandler.hpp"

#include <array>
#include <string_view>

#include "fmt/format.h"
#include "game/GameThreadDispatcher.hpp"
#include "game/Lua.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
struct ActivityUnlockItem {
    std::string_view itemName;
    std::array<const char*, 2> activitiesToUnlock;
};

constexpr std::array<ActivityUnlockItem, 12> supportedItems{
    {{.itemName = "Insurance Fraud Activity Unlock Pass",
      .activitiesToUnlock = {"fraud_mu", "fraud_fc"}},
     {.itemName = "Crowd Control Activity Unlock Pass",
      .activitiesToUnlock = {"crowd_ht", "crowd_su"}},
     {.itemName = "Demolition Derby Activity Unlock Pass",
      .activitiesToUnlock = {"demoderby_un"}},
     {.itemName = "Drug Trafficking Activity Unlock Pass",
      .activitiesToUnlock = {"drug_ht", "drug_ai"}},
     {.itemName = "Escort Activity Unlock Pass",
      .activitiesToUnlock = {"escort_un", "escort_rl"}},
     {.itemName = "Fight Club Activity Unlock Pass",
      .activitiesToUnlock = {"fight_ar", "fight_pr"}},
     {.itemName = "FUZZ Activity Unlock Pass",
      .activitiesToUnlock = {"fuzz_pj", "fuzz_sx"}},
     {.itemName = "Heli Assault Activity Unlock Pass",
      .activitiesToUnlock = {"heli_br", "heli_tp"}},
     {.itemName = "Mayhem Activity Unlock Pass",
      .activitiesToUnlock = {"mayhem_nu", "mayhem_st"}},
     {.itemName = "Septic Avenger Activity Unlock Pass",
      .activitiesToUnlock = {"sewage_rl", "sewage_sx"}},
     {.itemName = "Snatch Activity Unlock Pass",
      .activitiesToUnlock = {"snatch_ct", "snatch_dt"}},
     {.itemName = "Trail Blazing Activity Unlock Pass",
      .activitiesToUnlock = {"torch_ap", "torch_dt"}}}};

const ActivityUnlockItem* FindItem(const std::string_view itemName) noexcept {
    const auto found = std::ranges::find_if(
        supportedItems,
        [itemName](const auto& item) { return item.itemName == itemName; });
    return found == supportedItems.end() ? nullptr : &*found;
}

class LuaActivityUnlockItemHandler : public ItemHandler {
   public:
    explicit LuaActivityUnlockItemHandler(GameThreadDispatcher& dispatcher)
        : dispatcher_(dispatcher) {
    }

    [[nodiscard]] bool CanHandleItem(
        const std::string_view itemName) const noexcept override {
        return FindItem(itemName) != nullptr;
    }

    [[nodiscard]] bool Install() override {
        return Lua::CanUnlockActivity();
    }

    [[nodiscard]] bool Handle(const std::string_view itemName) override {
        const auto* const item = FindItem(itemName);
        if (!item) {
            return false;
        }

        const auto queued =
            dispatcher_.Dispatch([activitiesToUnlock = item->activitiesToUnlock,
                                  itemName = std::string{itemName}] {
                for (const auto& activity : activitiesToUnlock) {
                    if (!Lua::UnlockActivity(activity)) {
                        LogWarning(
                            "ActivityUnlock",
                            fmt::format(
                                "Failed to trigger item={} to unlock activity",
                                itemName, activity));
                    }
                }
            });

        if (!queued) {
            LogWarning("ActivityUnlock",
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

ItemHandlerPtr CreateLuaActivityUnlockHandler(
    GameThreadDispatcher& dispatcher) {
    return std::make_unique<LuaActivityUnlockItemHandler>(dispatcher);
}
}  // namespace sr2ap
