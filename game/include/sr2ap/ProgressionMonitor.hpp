#pragma once

#include "sr2ap/BaselineTracker.hpp"
#include "sr2ap/Progression.hpp"
#include "sr2ap/ProgressionEventSink.hpp"
#include "sr2ap/ProgressionEventTracker.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_set>

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
        void Emit(const ProgressionEvent& event) const;

        std::filesystem::path statusPath_;
        ProgressionEventSink eventSink_;
        bool writeStatusFile_;
        ProgressionEventTracker eventTracker_;
        BaselineTracker<std::string, bool> hitman_;
        BaselineTracker<std::string, bool> chopShop_;
        BaselineTracker<std::string, bool> missions_;
        BaselineTracker<std::string, std::uint8_t> activities_;
        BaselineTracker<std::string_view, RacingMedal> racing_;
        std::unordered_set<std::uint32_t> cdBaseline_;
        bool cdBaselineValid_{};
        HitmanReadResult lastHitmanResult_{HitmanReadResult::ReaderUnavailable};
        ChopShopReadResult lastChopShopResult_{ChopShopReadResult::ReaderUnavailable};
        MissionReadResult lastMissionResult_{MissionReadResult::ReaderUnavailable};
        ActivityReadResult lastActivityResult_{ActivityReadResult::ReaderUnavailable};
        ReaderResult lastRacingResult_{ReaderResult::ReaderUnavailable};
        CdReadResult lastCdResult_{CdReadResult::ReaderUnavailable};
    };
}  // namespace sr2ap
