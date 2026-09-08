#include "ProgressionEventTracker.hpp"

#include <unordered_map>

#include "features/Collectibles.hpp"

namespace sr2ap {
namespace {
template <class Entry, class Key, class Value>
std::unordered_map<Key, Value> MakeState(const std::vector<Entry>& entries,
                                         Key Entry::* key,
                                         Value Entry::* value) {
    std::unordered_map<Key, Value> state;
    for (const auto& entry : entries) {
        state.emplace(entry.*key, entry.*value);
    }
    return state;
}

template <class Result, class Key, class Value>
ProgressionUpdate<Key, Value> InvalidateUnlessSuccessful(
    Result result, BaselineTracker<Key, Value>& tracker) {
    if (result == ReaderResult::Success) {
        return {};
    }

    return {tracker.Invalidate(), {}};
}

template <class Entry>
BooleanProgressionUpdate ObserveBooleans(
    ReaderResult result, const std::vector<Entry>& entries,
    std::string Entry::* key, bool Entry::* value, ProgressionKind kind,
    BaselineTracker<std::string, bool>& tracker, bool emitCompletedBaseline) {
    if (result != ReaderResult::Success) {
        return InvalidateUnlessSuccessful(result, tracker);
    }

    auto baseline = tracker.Observe(MakeState(entries, key, value));
    std::vector<ProgressionEvent> events;

    if (emitCompletedBaseline &&
        (baseline.kind == BaselineUpdateKind::Created ||
         baseline.kind == BaselineUpdateKind::IdentityChanged)) {
        for (const auto& entry : entries) {
            if (entry.*value) {
                events.push_back({kind, entry.*key, 0, 1});
            }
        }
    }

    for (const auto& change : baseline.changes) {
        events.push_back({
            kind,
            change.key,
            change.previous ? 1U : 0U,
            change.current ? 1U : 0U,
        });
    }

    return {std::move(baseline), std::move(events)};
}
}  // namespace

BooleanProgressionUpdate ProgressionEventTracker::Observe(
    const HitmanSnapshot& snapshot) {
    return ObserveBooleans(
        snapshot.result, snapshot.targets, &HitmanTargetStatus::locationTag,
        &HitmanTargetStatus::complete, ProgressionKind::Hitman, hitman_, false);
}

BooleanProgressionUpdate ProgressionEventTracker::Observe(
    const ChopShopSnapshot& snapshot) {
    return ObserveBooleans(snapshot.result, snapshot.vehicles,
                           &ChopShopVehicleStatus::targetTag,
                           &ChopShopVehicleStatus::retrieved,
                           ProgressionKind::ChopShop, chopShop_, false);
}

BooleanProgressionUpdate ProgressionEventTracker::Observe(
    const MissionSnapshot& snapshot) {
    return ObserveBooleans(snapshot.result, snapshot.missions,
                           &MissionStatus::missionId, &MissionStatus::complete,
                           ProgressionKind::Mission, missions_, true);
}

ActivityProgressionUpdate ProgressionEventTracker::Observe(
    const ActivitySnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return InvalidateUnlessSuccessful(snapshot.result, activities_);
    }
    const auto update = activities_.Observe(
        MakeState(snapshot.instances, &ActivityInstanceStatus::instanceTag,
                  &ActivityInstanceStatus::completionFlags));
    ActivityProgressionUpdate result{update, {}};
    if (update.kind == BaselineUpdateKind::Created ||
        update.kind == BaselineUpdateKind::IdentityChanged) {
        for (const auto& instance : snapshot.instances) {
            if (instance.completionFlags != 0) {
                result.events.push_back({ProgressionKind::Activity,
                                         instance.instanceTag, 0,
                                         instance.completionFlags});
            }
        }
    }
    for (const auto& change : update.changes) {
        result.events.push_back({ProgressionKind::Activity, change.key,
                                 change.previous, change.current});
    }
    return result;
}

RacingProgressionUpdate ProgressionEventTracker::Observe(
    const RacingSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return InvalidateUnlessSuccessful(snapshot.result, racing_);
    }

    const auto update = racing_.Observe(
        MakeState(snapshot.races, &RaceStatus::name, &RaceStatus::medal));
    RacingProgressionUpdate result{update, {}};
    if (update.kind == BaselineUpdateKind::Created ||
        update.kind == BaselineUpdateKind::IdentityChanged) {
        for (const auto& race : snapshot.races) {
            if (const auto rank = RacingMedalRank(race.medal); rank != 0) {
                result.events.push_back(
                    {ProgressionKind::Racing, std::string{race.name}, 0, rank});
            }
        }
    }
    for (const auto& change : update.changes) {
        result.events.push_back({ProgressionKind::Racing,
                                 std::string{change.key},
                                 RacingMedalRank(change.previous),
                                 RacingMedalRank(change.current)});
    }
    return result;
}

CdProgressionUpdate ProgressionEventTracker::Observe(
    const CdSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        if (!cdsValid_) {
            return {};
        }

        cds_.clear();
        cdsValid_ = false;

        CdProgressionUpdate result;
        result.baseline.kind = BaselineUpdateKind::Invalidated;
        return result;
    }

    std::unordered_set<std::uint32_t> current{
        snapshot.collectedIds.begin(),
        snapshot.collectedIds.end(),
    };

    if (!cdsValid_) {
        cds_ = std::move(current);
        cdsValid_ = true;

        CdProgressionUpdate result;
        result.baseline.kind = BaselineUpdateKind::Created;
        return result;
    }

    CdProgressionUpdate result;

    // Newly collected CDs
    for (const auto id : current) {
        if (cds_.count(id) != 0) {
            continue;
        }

        result.baseline.changes.push_back({
            id,
            false,
            true,
        });

        const auto* key = FindCdDistrictKey(id);
        result.events.push_back({
            ProgressionKind::Cd,
            key ? key : "unknown",
            0,
            1,
        });
    }

    // CDs removed by loading an older save or other state rollback
    for (const auto id : cds_) {
        if (current.count(id) != 0) {
            continue;
        }

        result.baseline.changes.push_back({
            id,
            true,
            false,
        });
    }

    if (!result.baseline.changes.empty()) {
        result.baseline.kind = BaselineUpdateKind::Changed;
    }

    cds_ = std::move(current);
    return result;
}

StyleLevelProgressionUpdate ProgressionEventTracker::Observe(
    const StyleLevelSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return InvalidateUnlessSuccessful(snapshot.result, styleLevel_);
    }

    constexpr auto key = "player";
    const auto update = styleLevel_.Observe({{key, snapshot.displayedLevel}});
    StyleLevelProgressionUpdate result{update, {}};
    if (update.kind == BaselineUpdateKind::Created ||
        update.kind == BaselineUpdateKind::IdentityChanged) {
        result.events.push_back(
            {ProgressionKind::StyleLevel, key, 0, snapshot.displayedLevel});
    }
    for (const auto& change : update.changes) {
        result.events.push_back({ProgressionKind::StyleLevel, change.key,
                                 change.previous, change.current});
    }
    return result;
}
}  // namespace sr2ap
