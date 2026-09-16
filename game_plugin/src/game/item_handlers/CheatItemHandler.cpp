#include "CheatItemHandler.hpp"

#include <algorithm>
#include <utility>

#include "game/GameThreadDispatcher.hpp"
#include "game/controllers/CheatController.hpp"

namespace sr2ap {
namespace {

class CheatItemHandler final : public ItemHandler {
   public:
    CheatItemHandler(GameThreadDispatcher& dispatcher,
                     std::vector<std::string> managedItems)
        : dispatcher_{dispatcher}, managedItems_{std::move(managedItems)} {
    }

    [[nodiscard]] bool Install() override {
        if (!std::ranges::all_of(managedItems_, [](const auto& name) {
                return CheatController::SupportsItem(name);
            })) {
            return false;
        }
        return controller_.Install(managedItems_);
    }

    [[nodiscard]] bool CanHandleItem(
        const std::string_view itemName) const noexcept override {
        return CheatController::SupportsItem(itemName);
    }

    [[nodiscard]] bool Handle(const std::string_view itemName) override {
        return controller_.ActivateReceivedItem(itemName, dispatcher_);
    }

    void Remove() override {
        controller_.Remove();
    }

   private:
    GameThreadDispatcher& dispatcher_;
    std::vector<std::string> managedItems_;
    CheatController controller_;
};

}  // namespace

ItemHandlerPtr CreateCheatItemHandler(GameThreadDispatcher& dispatcher,
                                      std::vector<std::string> managedItems) {
    return std::make_unique<CheatItemHandler>(dispatcher,
                                              std::move(managedItems));
}

}  // namespace sr2ap
