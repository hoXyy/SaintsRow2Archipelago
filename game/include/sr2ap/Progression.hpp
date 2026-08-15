#pragma once

#include "sr2ap/Activities.hpp"
#include "sr2ap/ChopShop.hpp"
#include "sr2ap/Collectibles.hpp"
#include "sr2ap/Hitman.hpp"
#include "sr2ap/Missions.hpp"

namespace sr2ap {
    struct ProgressionSnapshot {
        HitmanSnapshot hitman;
        ChopShopSnapshot chopShop;
        MissionSnapshot missions;
        ActivitySnapshot activities;
        CdSnapshot cds;
    };

    ProgressionSnapshot GetProgressionSnapshot();
    void LogProgressionSnapshot(const ProgressionSnapshot& snapshot, bool full);
}  // namespace sr2ap
