#include "WeatherItemHandler.hpp"

#include <algorithm>
#include <utility>

#include "game/GameThreadDispatcher.hpp"
#include "game/controllers/WeatherController.hpp"

namespace sr2ap {
namespace {

class WeatherItemHandler final : public ItemHandler {
   public:
    WeatherItemHandler(GameThreadDispatcher& dispatcher,
                       std::vector<std::string> managedItems)
        : dispatcher_{dispatcher}, managedItems_{std::move(managedItems)} {
    }

    [[nodiscard]] bool Install() override {
        if (!std::ranges::all_of(managedItems_, [](const auto& name) {
                return WeatherController::SupportsItem(name);
            })) {
            return false;
        }
        return controller_.Install(managedItems_);
    }

    [[nodiscard]] bool CanHandleItem(
        const std::string_view itemName) const noexcept override {
        return WeatherController::SupportsItem(itemName);
    }

    [[nodiscard]] bool Handle(const std::string_view itemName,
                              ItemCompletion completion) override {
        return controller_.ActivateReceivedItem(itemName, dispatcher_,
                                                std::move(completion));
    }

    void Remove() override {
        controller_.Remove();
    }

   private:
    GameThreadDispatcher& dispatcher_;
    std::vector<std::string> managedItems_;
    WeatherController controller_;
};

}  // namespace

ItemHandlerPtr CreateWeatherItemHandler(GameThreadDispatcher& dispatcher,
                                        std::vector<std::string> managedItems) {
    return std::make_unique<WeatherItemHandler>(dispatcher,
                                                std::move(managedItems));
}

}  // namespace sr2ap
