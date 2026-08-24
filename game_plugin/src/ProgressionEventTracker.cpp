#include "sr2ap/ProgressionEventTracker.hpp"

#include "sr2ap/Collectibles.hpp"

#include <unordered_map>

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

        template <class Result, class Tracker>
        ProgressionUpdate InvalidateUnlessSuccessful(const Result result, Tracker& tracker) {
            if (result == ReaderResult::Success) {
                return {};
            }
            return {tracker.Invalidate().kind, {}};
        }

        template <class Entry>
        ProgressionUpdate ObserveBooleans(const ReaderResult result,
                                          const std::vector<Entry>& entries,
                                          std::string Entry::* key,
                                          bool Entry::* value,
                                          const ProgressionKind kind,
                                          BaselineTracker<std::string, bool>& tracker,
                                          const bool emitCompletedBaseline) {
            if (result != ReaderResult::Success) {
                return InvalidateUnlessSuccessful(result, tracker);
            }
            const auto update = tracker.Observe(MakeState(entries, key, value));
            ProgressionUpdate resultUpdate{update.kind, {}};
            if (emitCompletedBaseline &&
                (update.kind == BaselineUpdateKind::Created || update.kind == BaselineUpdateKind::IdentityChanged)) {
                for (const auto& entry : entries) {
                    if (entry.*value) {
                        resultUpdate.events.push_back({kind, entry.*key, 0, 1});
                    }
                }
            }
            for (const auto& change : update.changes) {
                resultUpdate.events.push_back({kind, change.key, change.previous ? 1U : 0U, change.current ? 1U : 0U});
            }
            return resultUpdate;
        }
    }  // namespace

    ProgressionUpdate ProgressionEventTracker::Observe(const HitmanSnapshot& snapshot) {
        return ObserveBooleans(snapshot.result, snapshot.targets, &HitmanTargetStatus::locationTag,
                               &HitmanTargetStatus::complete, ProgressionKind::Hitman, hitman_, false);
    }

    ProgressionUpdate ProgressionEventTracker::Observe(const ChopShopSnapshot& snapshot) {
        return ObserveBooleans(snapshot.result, snapshot.vehicles, &ChopShopVehicleStatus::targetTag,
                               &ChopShopVehicleStatus::retrieved, ProgressionKind::ChopShop, chopShop_, false);
    }

    ProgressionUpdate ProgressionEventTracker::Observe(const MissionSnapshot& snapshot) {
        return ObserveBooleans(snapshot.result, snapshot.missions, &MissionStatus::missionId, &MissionStatus::complete,
                               ProgressionKind::Mission, missions_, true);
    }

    ProgressionUpdate ProgressionEventTracker::Observe(const ActivitySnapshot& snapshot) {
        if (snapshot.result != ReaderResult::Success) {
            return InvalidateUnlessSuccessful(snapshot.result, activities_);
        }
        const auto update = activities_.Observe(MakeState(snapshot.instances, &ActivityInstanceStatus::instanceTag,
                                                          &ActivityInstanceStatus::completionFlags));
        ProgressionUpdate result{update.kind, {}};
        if (update.kind == BaselineUpdateKind::Created || update.kind == BaselineUpdateKind::IdentityChanged) {
            for (const auto& instance : snapshot.instances) {
                if (instance.completionFlags != 0) {
                    result.events.push_back(
                        {ProgressionKind::Activity, instance.instanceTag, 0, instance.completionFlags});
                }
            }
        }
        for (const auto& change : update.changes) {
            result.events.push_back({ProgressionKind::Activity, change.key, change.previous, change.current});
        }
        return result;
    }

    ProgressionUpdate ProgressionEventTracker::Observe(const RacingSnapshot& snapshot) {
        if (snapshot.result != ReaderResult::Success)
            return InvalidateUnlessSuccessful(snapshot.result, racing_);
        const auto update = racing_.Observe(MakeState(snapshot.races, &RaceStatus::name, &RaceStatus::medal));
        ProgressionUpdate result{update.kind, {}};
        if (update.kind == BaselineUpdateKind::Created || update.kind == BaselineUpdateKind::IdentityChanged) {
            for (const auto& race : snapshot.races) {
                if (const auto rank = RacingMedalRank(race.medal); rank != 0)
                    result.events.push_back({ProgressionKind::Racing, std::string{race.name}, 0, rank});
            }
        }
        for (const auto& change : update.changes) {
            result.events.push_back({ProgressionKind::Racing, std::string{change.key}, RacingMedalRank(change.previous),
                                     RacingMedalRank(change.current)});
        }
        return result;
    }

    ProgressionUpdate ProgressionEventTracker::Observe(const CdSnapshot& snapshot) {
        if (snapshot.result != ReaderResult::Success) {
            if (!cdsValid_) {
                return {};
            }
            cds_.clear();
            cdsValid_ = false;
            return {BaselineUpdateKind::Invalidated, {}};
        }
        const std::unordered_set<std::uint32_t> current(snapshot.collectedIds.begin(), snapshot.collectedIds.end());
        if (!cdsValid_) {
            cds_ = current;
            cdsValid_ = true;
            return {BaselineUpdateKind::Created, {}};
        }
        ProgressionUpdate update;
        for (const auto id : current) {
            if (cds_.count(id) == 0) {
                const auto* key = FindCdDistrictKey(id);
                update.events.push_back({ProgressionKind::Cd, key ? key : "unknown", 0, 1});
            }
        }
        if (current != cds_) {
            update.kind = BaselineUpdateKind::Changed;
            cds_ = current;
        }
        return update;
    }
}  // namespace sr2ap
