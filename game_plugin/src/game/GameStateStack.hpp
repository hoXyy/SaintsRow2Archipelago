#pragma once

#include <cstdint>
#include <optional>

namespace sr2ap {

struct ModuleInfo;

// The game's stack of top-level states, drawn bottom to top each frame:
// main menu (2), gameplay (5), the in-game menu over gameplay (6), loading
// screens (7, 10) and so on.
class GameStateStack {
   public:
    explicit GameStateStack(const ModuleInfo& game);

    // -1 when the stack is empty or the index is out of range.
    [[nodiscard]] int TopIndex() const;
    [[nodiscard]] std::optional<std::uint32_t> StateAt(int index) const;
    [[nodiscard]] std::optional<std::uint32_t> Top() const;
    // Gameplay is on top, so the gameplay HUD is loaded and showing.
    [[nodiscard]] bool IsGameplayOnTop() const;

   private:
    std::uintptr_t stack_{};
    std::uintptr_t topIndex_{};
};

}  // namespace sr2ap
