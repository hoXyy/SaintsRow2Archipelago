#pragma once

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
    GameReadiness GetGameReadiness();
    const char* ToString(GameReadiness value);
}  // namespace sr2ap
