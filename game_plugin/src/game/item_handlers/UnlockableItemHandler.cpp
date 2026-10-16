#include "UnlockableItemHandler.hpp"

#include <algorithm>
#include <utility>

#include "game/GameThreadDispatcher.hpp"
#include "game/controllers/UnlockableController.hpp"

namespace sr2ap {
namespace {

class UnlockableItemHandler final : public ItemHandler {
   public:
    UnlockableItemHandler(GameThreadDispatcher& dispatcher,
                          const bool blockVanillaRewards,
                          std::vector<std::string> managedItems)
        : dispatcher_{dispatcher},
          blockVanillaRewards_{blockVanillaRewards},
          managedItems_{std::move(managedItems)} {
    }

    [[nodiscard]] bool Install() override {
        if (!std::ranges::all_of(managedItems_, [](const auto& name) {
                return UnlockableController::SupportsItem(name);
            })) {
            return false;
        }
        return controller_.Install(blockVanillaRewards_, managedItems_);
    }

    [[nodiscard]] bool CanHandleItem(
        const std::string_view itemName) const noexcept override {
        return UnlockableController::SupportsItem(itemName);
    }

    [[nodiscard]] bool Handle(const std::string_view itemName,
                              ItemCompletion completion) override {
        return controller_.QueueReceivedItem(itemName, dispatcher_,
                                             std::move(completion));
    }

    void Update() override {
        controller_.Update(dispatcher_);
    }

    void Remove() override {
        controller_.Remove();
    }

   private:
    GameThreadDispatcher& dispatcher_;
    bool blockVanillaRewards_{};
    std::vector<std::string> managedItems_;
    UnlockableController controller_;
};

}  // namespace

ItemHandlerPtr CreateUnlockableItemHandler(
    GameThreadDispatcher& dispatcher, const bool blockVanillaRewards,
    std::vector<std::string> managedItems) {
    return std::make_unique<UnlockableItemHandler>(
        dispatcher, blockVanillaRewards, std::move(managedItems));
}

}  // namespace sr2ap
