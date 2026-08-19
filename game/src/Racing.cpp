#include "sr2ap/Racing.hpp"

#include "sr2ap/Addresses.hpp"
#include "sr2ap/Hitman.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <cmath>
#include <sstream>

namespace sr2ap {
    RacingSnapshot GetRacingSnapshot() {
        RacingSnapshot snapshot;
        const auto game = InspectSupportedGameModule();
        if (!game) {
            snapshot.result = ReaderResult::UnsupportedVersion;
            return snapshot;
        }
        if (GetHitmanSnapshot().result != HitmanReadResult::Success) {
            snapshot.result = ReaderResult::GameNotReady;
            return snapshot;
        }

        std::uint32_t count{};
        if (!SafeCopy(reinterpret_cast<const void*>(game->base + addresses::kRacingRecordCountRva), &count,
                      sizeof(count)) ||
            count != kRaceDefinitions.size()) {
            snapshot.result = ReaderResult::ManagerUnavailable;
            return snapshot;
        }

        snapshot.races.reserve(kRaceDefinitions.size());
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto record = game->base + addresses::kRacingRecordTableRva +
                                static_cast<std::uintptr_t>(index) * addresses::kRacingRecordStride;
            std::uint32_t identityHash{};
            std::uint32_t rawMedal{};
            float bestTime{};
            std::uint32_t raceClass{};
            if (!SafeCopy(reinterpret_cast<const void*>(record + addresses::kRacingRecordIdentityHashOffset),
                          &identityHash, sizeof(identityHash)) ||
                !SafeCopy(reinterpret_cast<const void*>(record + addresses::kRacingRecordMedalOffset), &rawMedal,
                          sizeof(rawMedal)) ||
                !SafeCopy(reinterpret_cast<const void*>(record + addresses::kRacingRecordBestTimeOffset), &bestTime,
                          sizeof(bestTime)) ||
                !SafeCopy(reinterpret_cast<const void*>(record + addresses::kRacingRecordClassOffset), &raceClass,
                          sizeof(raceClass))) {
                snapshot.result = ReaderResult::InvalidPointer;
                snapshot.races.clear();
                return snapshot;
            }
            const auto& definition = kRaceDefinitions[index];
            if (identityHash != definition.identityHash || raceClass != definition.raceClass || rawMedal > 4 ||
                !std::isfinite(bestTime) || bestTime < 0.0F || (rawMedal == 0 && bestTime != 0.0F)) {
                snapshot.result = ReaderResult::InvalidData;
                snapshot.races.clear();
                return snapshot;
            }
            snapshot.races.push_back(
                {definition.name, index, identityHash, static_cast<RacingMedal>(rawMedal), bestTime, raceClass});
        }
        snapshot.result = ReaderResult::Success;
        return snapshot;
    }

    void LogRacingSnapshot(const RacingSnapshot& snapshot, const bool full) {
        std::ostringstream summary;
        summary << "[Manual snapshot] result=" << ToString(snapshot.result) << " races=" << snapshot.races.size();
        LogInfo("Racing", summary.str());
        if (!full || snapshot.result != ReaderResult::Success)
            return;
        for (const auto& race : snapshot.races) {
            LogInfo("Racing", std::string{race.name} + "=" + ToString(race.medal) +
                                  " best_time=" + std::to_string(race.bestTime));
        }
    }
}  // namespace sr2ap
