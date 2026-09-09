#include "GameState.hpp"

#include <windows.h>

#include <cstdint>

#include "Addresses.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"

namespace sr2ap {

GameReadiness DetermineGameReadiness(std::uint8_t gameLoaded,
                                     std::uint8_t menuState,
                                     std::uint8_t cutsceneActive,
                                     std::uint32_t player) {
    if (menuState == addresses::kLoadingMenuState) {
        return GameReadiness::Loading;
    }

    if (gameLoaded == 0) {
        return GameReadiness::MainMenu;
    }

    if ((menuState == addresses::kGameplayMenuState ||
         menuState == addresses::kGameplayBusyState) &&
        cutsceneActive == 0 && player != 0) {
        return GameReadiness::GameplayInteractive;
    }

    return GameReadiness::GameplayReady;
}

GameContext ReadGameContext(const ModuleInfo* game) {
    GameContext context;
    context.module = game;

    if (!game) {
        return context;
    }

    std::uint8_t gameLoaded{};
    std::uint8_t menuState{};
    std::uint8_t cutsceneActive{};
    std::uint32_t player{};
    if (!SafeCopy(reinterpret_cast<const void*>(game->base +
                                                addresses::kGameLoadedRva),
                  &gameLoaded, sizeof(gameLoaded)) ||
        !SafeCopy(reinterpret_cast<const void*>(game->base +
                                                addresses::kMenuStateRva),
                  &menuState, sizeof(menuState)) ||
        !SafeCopy(reinterpret_cast<const void*>(game->base +
                                                addresses::kCutsceneActiveRva),
                  &cutsceneActive, sizeof(cutsceneActive)) ||
        !SafeCopy(reinterpret_cast<const void*>(game->base +
                                                addresses::kPlayerGlobalRva),
                  &player, sizeof(player))) {
        context.readiness = GameReadiness::GameplayUnavailable;
        return context;
    }

    context.readiness =
        DetermineGameReadiness(gameLoaded, menuState, cutsceneActive, player);

    return context;
};

bool IsGameplayLoaded(const GameReadiness value) noexcept {
    return value == GameReadiness::GameplayReady ||
           value == GameReadiness::GameplayInteractive;
}

bool IsGameplayInteractive(const GameReadiness value) noexcept {
    return value == GameReadiness::GameplayInteractive;
}

const char* ToString(GameReadiness value) {
    switch (value) {
        case GameReadiness::UnsupportedExecutable:
            return "unsupported_executable";
        case GameReadiness::MainMenu:
            return "main_menu";
        case GameReadiness::Loading:
            return "loading";
        case GameReadiness::GameplayUnavailable:
            return "gameplay_unavailable";
        case GameReadiness::GameplayReady:
            return "gameplay_ready";
        case GameReadiness::GameplayInteractive:
            return "gameplay_interactive";
    }
    return "unknown";
}
}  // namespace sr2ap
