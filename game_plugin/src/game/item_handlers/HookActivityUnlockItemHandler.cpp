#include "HookActivityUnlockItemHandler.hpp"

#include <atomic>
#include <chrono>
#include <utility>

#include "fmt/format.h"
#include "game/controllers/HookActivityUnlockController.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
constexpr auto kCollectibleRefreshInterval = std::chrono::milliseconds{250};

class HookActivityUnlockItemHandler final : public ItemHandler {
   public:
    HookActivityUnlockItemHandler(GameThreadDispatcher& dispatcher,
                                  std::vector<std::string> managedItems)
        : dispatcher_(dispatcher), managedItems_(std::move(managedItems)) {
    }

    [[nodiscard]] bool Install() override {
        return controller_.Install(managedItems_);
    }

    [[nodiscard]] bool CanHandleItem(
        std::string_view itemName) const noexcept override {
        return controller_.SupportsItem(itemName);
    }

    [[nodiscard]] bool Handle(std::string_view itemName) override {
        if (!initialized_.load(std::memory_order_acquire) ||
            !CanHandleItem(itemName)) {
            return false;
        }

        return dispatcher_.Dispatch([this, itemName = std::string{itemName}] {
            if (!controller_.Grant(itemName)) {
                LogWarning("HookActivityUnlock",
                           fmt::format("Could not grant item {}", itemName));
            }
        });
    }

    [[nodiscard]] bool ResetState() override {
        if (!initialized_.load(std::memory_order_acquire)) {
            return false;
        }

        return dispatcher_.Dispatch(
            [this] { controller_.ResetPersistentState(); });
    }

    void Update() override {
        if (initialized_.load(std::memory_order_acquire)) {
            RefreshCollectiblePolicy();
            return;
        }

        if (initializationQueued_.exchange(true, std::memory_order_acq_rel)) {
            return;
        }

        if (!dispatcher_.Dispatch([this] {
                if (controller_.InitializePolicy()) {
                    initialized_.store(true, std::memory_order_release);
                }

                initializationQueued_.store(false, std::memory_order_release);
            })) {
            initializationQueued_.store(false, std::memory_order_release);
        }
    }

    void Remove() override {
        initialized_.store(false, std::memory_order_release);
        initializationQueued_.store(false, std::memory_order_release);
        refreshQueued_.store(false, std::memory_order_release);
        controller_.Remove();
    }

   private:
    void RefreshCollectiblePolicy() {
        const auto now = std::chrono::steady_clock::now();
        if (now < nextCollectibleRefresh_ ||
            refreshQueued_.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        nextCollectibleRefresh_ = now + kCollectibleRefreshInterval;

        if (!dispatcher_.Dispatch([this] {
                controller_.RefreshCollectiblePolicy();
                refreshQueued_.store(false, std::memory_order_release);
            })) {
            refreshQueued_.store(false, std::memory_order_release);
        }
    }

    GameThreadDispatcher& dispatcher_;
    std::vector<std::string> managedItems_;
    HookActivityUnlockController controller_;

    std::atomic_bool initialized_{};
    std::atomic_bool initializationQueued_{};
    std::atomic_bool refreshQueued_{};
    std::chrono::steady_clock::time_point nextCollectibleRefresh_{};
};
}  // namespace

ItemHandlerPtr CreateHookActivityUnlockItemHandler(
    GameThreadDispatcher& dispatcher, std::vector<std::string> managedItems) {
    return std::make_unique<HookActivityUnlockItemHandler>(
        dispatcher, std::move(managedItems));
}
}  // namespace sr2ap
