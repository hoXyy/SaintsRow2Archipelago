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

    // Call on the game thread after Install() and retry until it succeeds.
    // It returns false while gameplay is not interactive. On success it
    // establishes the native requested state and applies the initial locked
    // policy before snapshot items are replayed.
    [[nodiscard]] bool InitializePolicy();

    [[nodiscard]] bool SupportsItem(std::string_view itemName) const noexcept;

    // Grant and ResetPersistentState call native game functions and must run
    // on the game thread.
    [[nodiscard]] bool Grant(std::string_view itemName);

    void ResetPersistentState();

    void Remove();

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

}  // namespace sr2ap
