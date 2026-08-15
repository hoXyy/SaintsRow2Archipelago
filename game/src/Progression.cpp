#include "sr2ap/Progression.hpp"

namespace sr2ap {
    ProgressionSnapshot GetProgressionSnapshot() {
        return {GetHitmanSnapshot(), GetChopShopSnapshot(), GetMissionSnapshot(), GetActivitySnapshot(),
                GetCdSnapshot()};
    }

    void LogProgressionSnapshot(const ProgressionSnapshot& snapshot, bool full) {
        LogHitmanSnapshot(snapshot.hitman, full);
        LogChopShopSnapshot(snapshot.chopShop, full);
        LogMissionSnapshot(snapshot.missions, full);
        LogActivitySnapshot(snapshot.activities, full);
        LogCdSnapshot(snapshot.cds, full);
    }
}  // namespace sr2ap
