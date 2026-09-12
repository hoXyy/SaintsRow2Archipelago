#pragma once

#include "features/Activities.hpp"
#include "features/ChopShop.hpp"
#include "features/Collectibles.hpp"
#include "features/Hitman.hpp"
#include "features/Missions.hpp"
#include "features/Racing.hpp"
#include "features/StyleLevel.hpp"
#include "game/GameState.hpp"

namespace sr2ap {
struct ProgressionSnapshot {
    HitmanSnapshot hitman;
    ChopShopSnapshot chopShop;
    MissionSnapshot missions;
    ActivitySnapshot activities;
    RacingSnapshot racing;
    CdSnapshot cds;
    StyleLevelSnapshot styleLevel;
};

ProgressionSnapshot GetProgressionSnapshot(const GameContext& context);
}  // namespace sr2ap
