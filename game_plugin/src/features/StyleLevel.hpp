#pragma once

#include <cstdint>

#include "game/GameState.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
struct StyleLevelSnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::uint32_t storedLevel{};
    std::uint32_t displayedLevel{};
    std::uint32_t points{};
    std::uint32_t currentMinimumPoints{};
    std::uint32_t nextMinimumPoints{};
    float respectBonus{};
};

StyleLevelSnapshot GetStyleLevelSnapshot(const GameContext& context);
void LogStyleLevelSnapshot(const StyleLevelSnapshot& snapshot, bool full);
}  // namespace sr2ap
