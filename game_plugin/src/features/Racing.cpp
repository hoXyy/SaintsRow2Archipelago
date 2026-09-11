#include "Racing.hpp"

#include <cmath>
#include <sstream>

#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Logger.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
RacingSnapshot GetRacingSnapshot(const GameContext& context) {
    RacingSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    if (!context.IsLoaded()) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    std::uint32_t count{};
    if (!SafeCopy(reinterpret_cast<const void*>(
                      game.base + addresses::kRacingRecordCountRva),
                  &count, sizeof(count)) ||
        count != kRaceDefinitions.size()) {
        snapshot.result = ReaderResult::ManagerUnavailable;
        return snapshot;
    }

    snapshot.races.reserve(kRaceDefinitions.size());
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto record =
            game.base + addresses::kRacingRecordTableRva +
            static_cast<std::uintptr_t>(index) * addresses::kRacingRecordStride;

        auto identityHash = ReadMemory<std::uint32_t>(
            record + addresses::kRacingRecordIdentityHashOffset);
        auto rawMedal = ReadMemory<std::uint32_t>(
            record + addresses::kRacingRecordMedalOffset);
        auto bestTime =
            ReadMemory<float>(record + addresses::kRacingRecordBestTimeOffset);
        auto raceClass = ReadMemory<std::uint32_t>(
            record + addresses::kRacingRecordClassOffset);

        if (!identityHash || !rawMedal || !bestTime || !raceClass) {
            snapshot.result = ReaderResult::InvalidPointer;
            snapshot.races.clear();
            return snapshot;
        }

        const auto& definition = kRaceDefinitions[index];
        if (*identityHash != definition.identityHash ||
            *raceClass != definition.raceClass || *rawMedal > 4 ||
            !std::isfinite(*bestTime) || *bestTime < 0.0F ||
            (*rawMedal == 0 && *bestTime != 0.0F)) {
            snapshot.result = ReaderResult::InvalidData;
            snapshot.races.clear();
            return snapshot;
        }

        snapshot.races.push_back({definition.name, index, *identityHash,
                                  static_cast<RacingMedal>(*rawMedal),
                                  *bestTime, *raceClass});
    }
    snapshot.result = ReaderResult::Success;
    return snapshot;
}

void LogRacingSnapshot(const RacingSnapshot& snapshot, const bool full) {
    std::ostringstream summary;
    summary << "[Manual snapshot] result=" << ToString(snapshot.result)
            << " races=" << snapshot.races.size();
    LogInfo("Racing", summary.str());

    if (!full || snapshot.result != ReaderResult::Success) {
        return;
    }

    for (const auto& race : snapshot.races) {
        LogInfo("Racing", std::string{race.name} + "=" + ToString(race.medal) +
                              " best_time=" + std::to_string(race.bestTime));
    }
}
}  // namespace sr2ap
