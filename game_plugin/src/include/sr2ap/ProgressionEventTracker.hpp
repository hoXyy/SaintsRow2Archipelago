#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "sr2ap/BaselineTracker.hpp"
#include "sr2ap/Progression.hpp"
#include "sr2ap/ProgressionEventSink.hpp"

namespace sr2ap {
template <class Key, class Value>
struct ProgressionUpdate {
    BaselineUpdate<Key, Value> baseline;
    std::vector<ProgressionEvent> events;
};

using BooleanProgressionUpdate = ProgressionUpdate<std::string, bool>;
using ActivityProgressionUpdate = ProgressionUpdate<std::string, std::uint8_t>;
using RacingProgressionUpdate =
    ProgressionUpdate<std::string_view, RacingMedal>;
using CdProgressionUpdate = ProgressionUpdate<std::uint32_t, bool>;
using StyleLevelProgressionUpdate =
    ProgressionUpdate<std::string, std::uint32_t>;

class ProgressionEventTracker {
   public:
    BooleanProgressionUpdate Observe(const HitmanSnapshot& snapshot);
    BooleanProgressionUpdate Observe(const ChopShopSnapshot& snapshot);
    BooleanProgressionUpdate Observe(const MissionSnapshot& snapshot);
    ActivityProgressionUpdate Observe(const ActivitySnapshot& snapshot);
    RacingProgressionUpdate Observe(const RacingSnapshot& snapshot);
    CdProgressionUpdate Observe(const CdSnapshot& snapshot);
    StyleLevelProgressionUpdate Observe(const StyleLevelSnapshot& snapshot);

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
