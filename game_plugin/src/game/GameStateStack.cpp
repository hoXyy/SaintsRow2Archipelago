#include "GameStateStack.hpp"

#include "Addresses.hpp"
#include "ModuleInfo.hpp"

namespace sr2ap {
namespace {
constexpr int kMaximumDepth = 16;

template <typename T>
T Read(const std::uintptr_t address) noexcept {
    return *reinterpret_cast<const volatile T*>(address);
}
}  // namespace

GameStateStack::GameStateStack(const ModuleInfo& game)
    : stack_{game.base + addresses::kGameStateStackRva},
      topIndex_{game.base + addresses::kGameStateTopIndexRva} {
}

int GameStateStack::TopIndex() const {
    const auto index = Read<int>(topIndex_);
    return index >= 0 && index < kMaximumDepth ? index : -1;
}

std::optional<std::uint32_t> GameStateStack::StateAt(const int index) const {
    if (index < 0 || index > TopIndex()) {
        return std::nullopt;
    }
    return Read<std::uint32_t>(stack_ + static_cast<std::uintptr_t>(index) *
                                            sizeof(std::uint32_t));
}

std::optional<std::uint32_t> GameStateStack::Top() const {
    return StateAt(TopIndex());
}

bool GameStateStack::IsGameplayOnTop() const {
    return Top() == addresses::kGameplayGameState;
}

}  // namespace sr2ap
