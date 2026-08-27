#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "sr2ap/BaselineTracker.hpp"
#include "sr2ap/Progression.hpp"
#include "sr2ap/ProgressionEventSink.hpp"

namespace sr2ap {
struct ProgressionUpdate {
    BaselineUpdateKind kind{BaselineUpdateKind::Unchanged};
    std::vector<ProgressionEvent> events;
};

class ProgressionEventTracker {
   public:
    ProgressionUpdate Observe(const HitmanSnapshot& snapshot);
    ProgressionUpdate Observe(const ChopShopSnapshot& snapshot);
    ProgressionUpdate Observe(const MissionSnapshot& snapshot);
    ProgressionUpdate Observe(const ActivitySnapshot& snapshot);
    ProgressionUpdate Observe(const RacingSnapshot& snapshot);
    ProgressionUpdate Observe(const CdSnapshot& snapshot);
    ProgressionUpdate Observe(const StyleLevelSnapshot& snapshot);

   private:
    BaselineTracker<std::string, bool> hitman_;
    BaselineTracker<std::string, bool> chopShop_;
    BaselineTracker<std::string, bool> missions_;
    BaselineTracker<std::string, std::uint8_t> activities_;
    BaselineTracker<std::string_view, RacingMedal> racing_;
    std::unordered_set<std::uint32_t> cds_;
    bool cdsValid_{};
    BaselineTracker<std::string, std::uint32_t> styleLevel_;
};
}  // namespace sr2ap
