#pragma once

#include "sr2ap/ReaderResult.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sr2ap {
    using ActivityReadResult = ReaderResult;

    struct ActivityInstanceStatus {
        std::string instanceTag;
        std::uint32_t completedLevels{};
        std::uint32_t totalLevels{};
        std::uint8_t completionFlags{};
    };

    struct ActivitySnapshot {
        ActivityReadResult result{ActivityReadResult::ReaderUnavailable};
        std::vector<ActivityInstanceStatus> instances;
    };

    ActivitySnapshot GetActivitySnapshot();
    void LogActivitySnapshot(const ActivitySnapshot& snapshot, bool full);
}  // namespace sr2ap
