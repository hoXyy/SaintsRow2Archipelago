#include "RespectItemHandler.hpp"

#include <algorithm>
#include <array>

#include "game/controllers/RespectController.hpp"

namespace sr2ap {
namespace {

constexpr std::array<std::string_view, 2> kRespectItems{"+1 Respect",
                                                        "+1 Bonus Respect"};

class RespectItemHandler final : public ItemHandler {
   public:
    explicit RespectItemHandler(RespectController& controller)
        : controller_{controller} {
    }

    [[nodiscard]] bool Install() override {
        return controller_.Install();
    }

    [[nodiscard]] bool CanHandleItem(
        const std::string_view itemName) const noexcept override {
        return std::ranges::find(kRespectItems, itemName) !=
               kRespectItems.end();
    }

    [[nodiscard]] bool Handle(const std::string_view itemName) override {
        return CanHandleItem(itemName) && controller_.GrantBar();
    }

    void Update() override {
        controller_.Update();
    }

    void Remove() override {
        controller_.Remove();
    }

   private:
    RespectController& controller_;
};

}  // namespace

ItemHandlerPtr CreateRespectItemHandler(RespectController& controller) {
    return std::make_unique<RespectItemHandler>(controller);
}

}  // namespace sr2ap
