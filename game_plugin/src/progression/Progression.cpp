#include "Progression.hpp"

#include "game/GameState.hpp"

namespace sr2ap {
ProgressionSnapshot GetProgressionSnapshot(const GameContext& context) {
    return {GetHitmanSnapshot(context),    GetChopShopSnapshot(context),
            GetMissionSnapshot(context),   GetActivitySnapshot(context),
            GetRacingSnapshot(context),    GetCdSnapshot(context),
            GetStyleLevelSnapshot(context)};
}

}  // namespace sr2ap
