#pragma once

#include <filesystem>
#include <functional>

#include "Progression.hpp"
#include "ProgressionEventSink.hpp"
#include "ProgressionEventTracker.hpp"

namespace sr2ap {
using ProgressionEventSink = std::function<void(const ProgressionEvent&)>;

class ProgressionMonitor {
   public:
    explicit ProgressionMonitor(std::filesystem::path statusPath,
                                ProgressionEventSink eventSink = {},
                                bool writeStatusFile = false);

    void CaptureManualSnapshot(bool full) const;
    void DumpCompactSnapshot() const;
    void Poll();

   private:
    bool UpdateHitman(const HitmanSnapshot& snapshot);
    bool UpdateChopShop(const ChopShopSnapshot& snapshot);
    bool UpdateMissions(const MissionSnapshot& snapshot);
    bool UpdateActivities(const ActivitySnapshot& snapshot);
    bool UpdateRacing(const RacingSnapshot& snapshot);
    bool UpdateCds(const CdSnapshot& snapshot);
    bool UpdateStyleLevel(const StyleLevelSnapshot& snapshot);
    void Emit(const ProgressionEvent& event) const;

    std::filesystem::path statusPath_;
    ProgressionEventSink eventSink_;
    bool writeStatusFile_;
    ProgressionEventTracker eventTracker_;
    HitmanReadResult lastHitmanResult_{HitmanReadResult::ReaderUnavailable};
    ChopShopReadResult lastChopShopResult_{
        ChopShopReadResult::ReaderUnavailable};
    MissionReadResult lastMissionResult_{MissionReadResult::ReaderUnavailable};
    ActivityReadResult lastActivityResult_{
        ActivityReadResult::ReaderUnavailable};
    ReaderResult lastRacingResult_{ReaderResult::ReaderUnavailable};
    CdReadResult lastCdResult_{CdReadResult::ReaderUnavailable};
    ReaderResult lastStyleLevelResult_{ReaderResult::ReaderUnavailable};
};
}  // namespace sr2ap
