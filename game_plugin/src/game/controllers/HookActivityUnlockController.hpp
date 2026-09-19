#pragma once

#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace sr2ap {

class HookActivityUnlockController {
   public:
    HookActivityUnlockController();
    ~HookActivityUnlockController();

    HookActivityUnlockController(const HookActivityUnlockController&) = delete;
    HookActivityUnlockController& operator=(
        const HookActivityUnlockController&) = delete;
    HookActivityUnlockController(HookActivityUnlockController&&) = delete;
    HookActivityUnlockController& operator=(HookActivityUnlockController&&) =
        delete;

    [[nodiscard]] bool Install(std::span<const std::string> managedItems);

    [[nodiscard]] bool InitializePolicy();

    [[nodiscard]] bool SupportsItem(std::string_view itemName) const noexcept;

    [[nodiscard]] bool Grant(std::string_view itemName);

    void ResetPersistentState();

    void RefreshCollectiblePolicy();

    void Remove();

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

}  // namespace sr2ap
