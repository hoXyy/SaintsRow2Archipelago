#include "RespectItemHandler.hpp"

#include <algorithm>
#include <array>

#include "game/GameThreadDispatcher.hpp"
#include "game/controllers/RespectController.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {

constexpr std::array<std::string_view, 2> kRespectItems{"+1 Respect",
                                                        "+1 Bonus Respect"};

class RespectItemHandler final : public ItemHandler {
   public:
    RespectItemHandler(GameThreadDispatcher& dispatcher,
                       RespectController& controller)
        : dispatcher_{dispatcher}, controller_{controller} {
    }

    [[nodiscard]] bool Install() override {
        return controller_.Install();
    }

    [[nodiscard]] bool CanHandleItem(
        const std::string_view itemName) const noexcept override {
        return std::ranges::find(kRespectItems, itemName) !=
               kRespectItems.end();
    }

    [[nodiscard]] bool Handle(const std::string_view itemName,
                              ItemCompletion completion) override {
        if (!CanHandleItem(itemName)) {
            return false;
        }

        // This should probably should've been ran on the game thread a while
        // ago
        const bool queued = dispatcher_.Dispatch([this, completion] {
            if (!completion.CanApply()) {
                return;
            }
            completion.Complete(controller_.GrantBar());
        });

        if (!queued) {
            LogWarning("Respect",
                       "Failed to queue respect item for game thread");
        }
        return queued;
    }

    void Update() override {
        controller_.Update();
    }

    void Remove() override {
        controller_.Remove();
    }

   private:
    GameThreadDispatcher& dispatcher_;
    RespectController& controller_;
};

}  // namespace

ItemHandlerPtr CreateRespectItemHandler(GameThreadDispatcher& dispatcher,
                                        RespectController& controller) {
    return std::make_unique<RespectItemHandler>(dispatcher, controller);
}

}  // namespace sr2ap
