#pragma once

#include "game/ModuleInfo.hpp"

namespace sr2ap {
enum class GameReadiness {
    UnsupportedExecutable,
    MainMenu,
    Loading,
    GameplayUnavailable,
    GameplayReady,
    GameplayInteractive
};

[[nodiscard]] bool IsGameplayLoaded(GameReadiness value) noexcept;
[[nodiscard]] bool IsGameplayInteractive(GameReadiness value) noexcept;

struct GameContext {
    const ModuleInfo* module{};
    GameReadiness readiness{GameReadiness::UnsupportedExecutable};

    [[nodiscard]] bool IsSupported() const noexcept {
        return module != nullptr;
    }

    [[nodiscard]] bool IsLoaded() const noexcept {
        return IsGameplayLoaded(readiness);
    }

    [[nodiscard]] bool IsInteractive() const noexcept {
        return IsGameplayInteractive(readiness);
    }
};

GameContext ReadGameContext(const ModuleInfo* game);

const char* ToString(GameReadiness value);
}  // namespace sr2ap
