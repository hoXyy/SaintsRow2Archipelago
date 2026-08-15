#pragma once

namespace sr2ap {
    enum class GameReadiness {
        UnsupportedExecutable,
        MainMenu,
        Loading,
        GameplayUnavailable,
        GameplayReady
    };
    GameReadiness GetGameReadiness();
    const char* ToString(GameReadiness value);
}  // namespace sr2ap
