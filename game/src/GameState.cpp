#include "sr2ap/GameState.hpp"
#include "sr2ap/Hitman.hpp"

namespace sr2ap {
    GameReadiness GetGameReadiness() {
        switch (GetHitmanSnapshot().result) {
            case HitmanReadResult::Success:
                return GameReadiness::GameplayReady;
            case HitmanReadResult::UnsupportedVersion:
                return GameReadiness::UnsupportedExecutable;
            case HitmanReadResult::GameNotReady:
                return GameReadiness::MainMenu;
            default:
                return GameReadiness::GameplayUnavailable;
        }
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
        }
        return "unknown";
    }
}  // namespace sr2ap
