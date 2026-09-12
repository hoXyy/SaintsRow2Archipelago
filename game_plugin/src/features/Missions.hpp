#pragma once

#include <string>
#include <vector>

#include "game/GameState.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
using MissionReadResult = ReaderResult;

struct MissionStatus {
    std::string missionId;
    bool complete{};
};

struct MissionSnapshot {
    MissionReadResult result{MissionReadResult::ReaderUnavailable};
    std::vector<MissionStatus> missions;
};

MissionSnapshot GetMissionSnapshot(const GameContext& context);
}  // namespace sr2ap
