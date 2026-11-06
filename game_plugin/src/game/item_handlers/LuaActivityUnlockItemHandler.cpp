#include "LuaActivityUnlockItemHandler.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <safetyhook.hpp>
#include <string_view>
#include <utility>
#include <vector>

#include "fmt/format.h"
#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/GameThreadDispatcher.hpp"
#include "game/Lua.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
struct ActivityUnlockItem {
    std::string_view itemName;
    std::array<const char*, 2> activitiesToUnlock;
};

constexpr std::array<ActivityUnlockItem, 14> supportedItems{
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
      .activitiesToUnlock = {"torch_ap", "torch_dt"}},
     {.itemName = "Dex's Phone Number", .activitiesToUnlock = {"em01"}},
     {.itemName = "Stilwater Caverns Stronghold Unlock",
      .activitiesToUnlock = {"sh_tss_caverns"}}}};

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
        return Lua::CanUnlockActivity() && InstallCheckpointHook();
    }

    [[nodiscard]] bool Handle(const std::string_view itemName,
                              ItemCompletion completion) override {
        const auto* const item = FindItem(itemName);
        if (!item) {
            return false;
        }

        const auto queued = dispatcher_.Dispatch([this, item, completion] {
            if (!completion.CanApply()) {
                return;
            }

            const bool unlocked = ApplyItem(*item);
            if (unlocked && !std::ranges::contains(appliedItems_, item)) {
                appliedItems_.push_back(item);
            }

            completion.Complete(unlocked);
        });

        if (!queued) {
            LogWarning("ActivityUnlock",
                       fmt::format("Failed to queue item={}", itemName));
        }

        return queued;
    }

    // Runs when a save loads or gameplay ends. A checkpoint can only lose items
    // applied since its level began, so earlier ones needn't be tracked.
    [[nodiscard]] bool ResetState() override {
        return dispatcher_.Dispatch([this] {
            appliedItems_.clear();
            checkpointRestored.store(false, std::memory_order_release);
        });
    }

    void Update() override {
        if (!checkpointRestored.exchange(false, std::memory_order_acq_rel)) {
            return;
        }

        if (!dispatcher_.Dispatch([this] { ReapplyItems(); })) {
            checkpointRestored.store(true, std::memory_order_release);
        }
    }

    void Remove() override {
        if (checkpointLoadHook_) {
            if (const auto disabled = checkpointLoadHook_.disable();
                !disabled) {
                LogError("ActivityUnlock",
                         "Failed to disable checkpoint load hook");
            }
            checkpointLoadHook_.reset();
        }
        checkpointRestored.store(false, std::memory_order_release);
    }

   private:
    [[nodiscard]] bool InstallCheckpointHook() {
        const auto* const game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }

        const auto loadCall = game->base + addresses::kCheckpointLoadCallRva;
        const auto loadTarget = game->base + addresses::kSaveLoadAllRva;
        if (ResolveRelativeCallTarget(loadCall) != loadTarget) {
            LogError("ActivityUnlock",
                     "Unexpected checkpoint load call target");
            return false;
        }

        auto hook = safetyhook::MidHook::create(
            reinterpret_cast<void*>(loadCall), &CheckpointLoadHook,
            safetyhook::MidHook::StartDisabled);
        if (!hook) {
            LogError("ActivityUnlock", "Could not create checkpoint load hook");
            return false;
        }

        checkpointLoadHook_ = std::move(*hook);
        checkpointRestored.store(false, std::memory_order_release);
        if (const auto enabled = checkpointLoadHook_.enable(); !enabled) {
            LogError("ActivityUnlock", "Could not enable checkpoint load hook");
            return false;
        }
        return true;
    }

    // The game thread is about to overwrite every activity's unlock flags with
    // the checkpoint's, which may predate items applied since the level began.
    // Frame dispatches can't run until the restore has finished.
    static void CheckpointLoadHook(safetyhook::Context&) {
        checkpointRestored.store(true, std::memory_order_release);
    }

    static bool ApplyItem(const ActivityUnlockItem& item) {
        bool unlocked = true;
        for (const auto& activity : item.activitiesToUnlock) {
            // Items with a single activity leave the second slot null.
            if (!activity) {
                continue;
            }

            if (!Lua::UnlockActivity(activity)) {
                unlocked = false;
                LogWarning(
                    "ActivityUnlock",
                    fmt::format("Failed to unlock activity={} for item={}",
                                activity, item.itemName));
            }
        }
        return unlocked;
    }

    // Unlocking is idempotent, so items the checkpoint kept are unaffected.
    void ReapplyItems() {
        const auto* const game = InspectSupportedGameModule();
        if (!game || !ReadGameContext(game).IsInteractive()) {
            checkpointRestored.store(true, std::memory_order_release);
            return;
        }

        for (const auto* const item : appliedItems_) {
            ApplyItem(*item);
        }
        if (!appliedItems_.empty()) {
            LogDebug(
                "ActivityUnlock",
                fmt::format("Reapplied {} item(s) after checkpoint restore",
                            appliedItems_.size()));
        }
    }

    GameThreadDispatcher& dispatcher_;
    safetyhook::MidHook checkpointLoadHook_;
    // Only touched on the game thread.
    std::vector<const ActivityUnlockItem*> appliedItems_;
    inline static std::atomic_bool checkpointRestored{};
};
}  // namespace

ItemHandlerPtr CreateLuaActivityUnlockHandler(
    GameThreadDispatcher& dispatcher) {
    return std::make_unique<LuaActivityUnlockItemHandler>(dispatcher);
}
}  // namespace sr2ap
