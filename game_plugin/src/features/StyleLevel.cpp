#include "StyleLevel.hpp"

#include <cmath>
#include <sstream>

#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Logger.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
StyleLevelSnapshot GetStyleLevelSnapshot(const GameContext& context) {
    StyleLevelSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;
    const auto gameBase = game.base;

    auto player =
        ReadMemory<std::uint32_t>(gameBase + addresses::kPlayerGlobalRva);
    auto table =
        ReadMemory<std::uint32_t>(gameBase + addresses::kStyleLevelTableRva);
    auto count =
        ReadMemory<std::uint32_t>(gameBase + addresses::kStyleLevelCountRva);
    if (!player || !table || !count) {
        snapshot.result = ReaderResult::InvalidPointer;
        return snapshot;
    }

    if (*player == 0 || *table == 0 || *count == 0) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    if (*count != addresses::kExpectedStyleLevelCount) {
        snapshot.result = ReaderResult::InvalidData;
        return snapshot;
    }

    if (!SafeCopy(
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(*player) +
                                          addresses::kPlayerStyleLevelOffset),
            &snapshot.storedLevel, sizeof(snapshot.storedLevel)) ||
        !SafeCopy(
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(*player) +
                                          addresses::kPlayerStylePointsOffset),
            &snapshot.points, sizeof(snapshot.points))) {
        snapshot.result = ReaderResult::InvalidPointer;
        return snapshot;
    }

    if (snapshot.storedLevel >= *count) {
        snapshot.result = ReaderResult::InvalidData;
        return snapshot;
    }

    const auto entry = static_cast<std::uintptr_t>(*table) +
                       snapshot.storedLevel * addresses::kStyleLevelEntryStride;

    if (!SafeCopy(reinterpret_cast<const void*>(
                      entry + addresses::kStyleLevelMinimumPointsOffset),
                  &snapshot.currentMinimumPoints,
                  sizeof(snapshot.currentMinimumPoints)) ||
        !SafeCopy(reinterpret_cast<const void*>(
                      entry + addresses::kStyleLevelRespectBonusOffset),
                  &snapshot.respectBonus, sizeof(snapshot.respectBonus)) ||
        !std::isfinite(snapshot.respectBonus)) {
        snapshot.result = ReaderResult::InvalidData;
        return snapshot;
    }

    if (snapshot.storedLevel + 1 < count) {
        const auto nextEntry = entry + addresses::kStyleLevelEntryStride;
        if (!SafeCopy(
                reinterpret_cast<const void*>(
                    nextEntry + addresses::kStyleLevelMinimumPointsOffset),
                &snapshot.nextMinimumPoints,
                sizeof(snapshot.nextMinimumPoints))) {
            snapshot.result = ReaderResult::InvalidPointer;
            return snapshot;
        }
    }

    snapshot.displayedLevel = snapshot.storedLevel;
    snapshot.result = ReaderResult::Success;
    return snapshot;
}

void LogStyleLevelSnapshot(const StyleLevelSnapshot& snapshot,
                           const bool full) {
    std::ostringstream message;
    message << "[Manual snapshot] result=" << ToString(snapshot.result);
    if (snapshot.result == ReaderResult::Success) {
        message << " stored_level=" << snapshot.storedLevel
                << " displayed_level=" << snapshot.displayedLevel
                << " points=" << snapshot.points;
        if (full) {
            message << " current_minimum=" << snapshot.currentMinimumPoints
                    << " next_minimum=";
            if (snapshot.storedLevel + 1 <
                addresses::kExpectedStyleLevelCount) {
                message << snapshot.nextMinimumPoints;
            } else {
                message << "none";
            }
            message << " respect_bonus=" << snapshot.respectBonus;
        }
    }
    LogInfo("StyleLevel", message.str());
}
}  // namespace sr2ap
