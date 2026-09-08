#include "Progression.hpp"

#include "game/GameState.hpp"

namespace sr2ap {
ProgressionSnapshot GetProgressionSnapshot(const GameContext& context) {
    return {GetHitmanSnapshot(context),    GetChopShopSnapshot(context),
            GetMissionSnapshot(context),   GetActivitySnapshot(context),
            GetRacingSnapshot(context),    GetCdSnapshot(context),
            GetStyleLevelSnapshot(context)};
}

void LogProgressionSnapshot(const ProgressionSnapshot& snapshot, bool full) {
    LogHitmanSnapshot(snapshot.hitman, full);
    LogChopShopSnapshot(snapshot.chopShop, full);
    LogMissionSnapshot(snapshot.missions, full);
    LogActivitySnapshot(snapshot.activities, full);
    LogRacingSnapshot(snapshot.racing, full);
    LogCdSnapshot(snapshot.cds, full);
    LogStyleLevelSnapshot(snapshot.styleLevel, full);
}
}  // namespace sr2ap
