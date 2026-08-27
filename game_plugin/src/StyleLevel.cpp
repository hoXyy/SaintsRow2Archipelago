#include "sr2ap/StyleLevel.hpp"

#include <cmath>
#include <sstream>

#include "sr2ap/Addresses.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

namespace sr2ap {
StyleLevelSnapshot GetStyleLevelSnapshot() {
    StyleLevelSnapshot snapshot;
    const auto game = InspectSupportedGameModule();
    if (!game) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    std::uint32_t player{};
    std::uint32_t table{};
    std::uint32_t count{};
    if (!SafeCopy(reinterpret_cast<const void*>(game->base +
                                                addresses::kPlayerGlobalRva),
                  &player, sizeof(player)) ||
        !SafeCopy(reinterpret_cast<const void*>(game->base +
                                                addresses::kStyleLevelTableRva),
                  &table, sizeof(table)) ||
        !SafeCopy(reinterpret_cast<const void*>(game->base +
                                                addresses::kStyleLevelCountRva),
                  &count, sizeof(count))) {
        snapshot.result = ReaderResult::InvalidPointer;
        return snapshot;
    }
    if (player == 0 || table == 0 || count == 0) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }
    if (count != addresses::kExpectedStyleLevelCount) {
        snapshot.result = ReaderResult::InvalidData;
        return snapshot;
    }

    if (!SafeCopy(
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(player) +
                                          addresses::kPlayerStyleLevelOffset),
            &snapshot.storedLevel, sizeof(snapshot.storedLevel)) ||
        !SafeCopy(
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(player) +
                                          addresses::kPlayerStylePointsOffset),
            &snapshot.points, sizeof(snapshot.points))) {
        snapshot.result = ReaderResult::InvalidPointer;
        return snapshot;
    }
    if (snapshot.storedLevel >= count) {
        snapshot.result = ReaderResult::InvalidData;
        return snapshot;
    }

    const auto entry = static_cast<std::uintptr_t>(table) +
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
