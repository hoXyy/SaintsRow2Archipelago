#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "game/GameState.hpp"
#include "util/ReaderResult.hpp"

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

ActivitySnapshot GetActivitySnapshot(const GameContext& context);
}  // namespace sr2ap
