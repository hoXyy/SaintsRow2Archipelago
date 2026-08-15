#pragma once

#include "sr2ap/ReaderResult.hpp"

#include <string>
#include <vector>

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

    MissionSnapshot GetMissionSnapshot();
    void LogMissionSnapshot(const MissionSnapshot& snapshot, bool full);
}  // namespace sr2ap
