#include "sr2ap/GameState.hpp"
#include "sr2ap/Addresses.hpp"
#include "sr2ap/Hitman.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <windows.h>
#include <cstdint>

namespace sr2ap {
    bool IsGameplayLoaded(const GameReadiness value) noexcept {
        return value == GameReadiness::GameplayReady || value == GameReadiness::GameplayInteractive;
    }

    bool IsGameplayInteractive(const GameReadiness value) noexcept {
        return value == GameReadiness::GameplayInteractive;
    }

    GameReadiness GetGameReadiness() {
        const auto hitmanResult = GetHitmanSnapshot().result;
        switch (hitmanResult) {
            case HitmanReadResult::Success:
                break;
            case HitmanReadResult::UnsupportedVersion:
                return GameReadiness::UnsupportedExecutable;
            case HitmanReadResult::GameNotReady:
                return GameReadiness::MainMenu;
            default:
                return GameReadiness::GameplayUnavailable;
        }

        const auto game = InspectSupportedGameModule();
        if (!game) {
            return GameReadiness::UnsupportedExecutable;
        }

        std::uint8_t gameLoaded{};
        std::uint8_t menuState{};
        std::uint8_t cutsceneActive{};
        std::uint32_t player{};
        if (!SafeCopy(reinterpret_cast<const void*>(game->base + addresses::kGameLoadedRva), &gameLoaded,
                      sizeof(gameLoaded)) ||
            !SafeCopy(reinterpret_cast<const void*>(game->base + addresses::kMenuStateRva), &menuState,
                      sizeof(menuState)) ||
            !SafeCopy(reinterpret_cast<const void*>(game->base + addresses::kCutsceneActiveRva), &cutsceneActive,
                      sizeof(cutsceneActive)) ||
            !SafeCopy(reinterpret_cast<const void*>(game->base + addresses::kPlayerGlobalRva), &player,
                      sizeof(player))) {
            return GameReadiness::GameplayUnavailable;
        }

        if (menuState == addresses::kLoadingMenuState) {
            return GameReadiness::Loading;
        }
        if (gameLoaded != 0 && menuState == addresses::kGameplayMenuState && cutsceneActive == 0 && player != 0) {
            return GameReadiness::GameplayInteractive;
        }
        return GameReadiness::GameplayReady;
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
