#pragma once

#include <memory>
#include <string_view>
#include <vector>

namespace sr2ap {

class ItemHandler {
   public:
    ItemHandler() = default;
    virtual ~ItemHandler() = default;

    ItemHandler(const ItemHandler&) = delete;
    ItemHandler& operator=(const ItemHandler&) = delete;
    ItemHandler(ItemHandler&&) = delete;
    ItemHandler& operator=(ItemHandler&&) = delete;

    [[nodiscard]] virtual bool Install() = 0;
    [[nodiscard]] virtual bool CanHandleItem(
        std::string_view itemName) const noexcept = 0;
    [[nodiscard]] virtual bool Handle(std::string_view itemName) = 0;

    [[nodiscard]] virtual bool ResetState() {
        return true;
    }

    virtual void Update() {
    }

    virtual void Remove() = 0;
};

using ItemHandlerPtr = std::unique_ptr<ItemHandler>;
using ItemHandlers = std::vector<ItemHandlerPtr>;

}  // namespace sr2ap
