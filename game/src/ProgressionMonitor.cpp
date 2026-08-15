#include "sr2ap/ProgressionMonitor.hpp"

#include "sr2ap/Collectibles.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Status.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
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

        template <class Tracker, class Result>
        bool HandleUnavailable(Tracker& tracker, Result result, Result& previous, const char* subsystem) {
            const bool invalidated = tracker.Invalidate().kind == BaselineUpdateKind::Invalidated;
            if (invalidated) {
                LogInfo(subsystem,
                        std::string("Progression unavailable; baseline invalidated result=") + ToString(result));
            } else if (result != previous) {
                LogDebug(subsystem, std::string("Polling waiting result=") + ToString(result));
            }
            previous = result;
            return invalidated;
        }

        void LogBooleanChanges(const char* subsystem, const std::vector<BaselineChange<std::string, bool>>& changes) {
            for (const auto& change : changes) {
                const auto message = "Completion changed: " + change.key + (change.previous ? " 1 -> 0" : " 0 -> 1");
                if (change.current) {
                    LogInfo(subsystem, message);
                } else {
                    LogWarning(subsystem, message);
                }
            }
        }

        std::string Hex(std::uint32_t value) {
            std::ostringstream stream;
            stream << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
            return stream.str();
        }
    }  // namespace

    ProgressionMonitor::ProgressionMonitor(std::filesystem::path statusPath,
                                           ProgressionEventSink eventSink,
                                           bool writeStatusFile)
        : statusPath_{std::move(statusPath)},
          eventSink_{std::move(eventSink)},
          writeStatusFile_{std::move(writeStatusFile)} {
    }

    void ProgressionMonitor::CaptureManualSnapshot(bool full) const {
        const auto snapshot = GetProgressionSnapshot();
        LogProgressionSnapshot(snapshot, full);
        if (!WriteProgressionStatus(statusPath_, snapshot)) {
            LogWarning("Status", "Unable to replace diagnostic status file");
        }
    }

    void ProgressionMonitor::DumpCompactSnapshot() const {
        LogInfo("Diagnostics", "F9 compact progression status; installed_hooks=0");
        LogProgressionSnapshot(GetProgressionSnapshot(), false);
    }

    void ProgressionMonitor::Emit(const ProgressionEvent& event) const {
        if (eventSink_) {
            eventSink_(event);
        }
    }

    bool ProgressionMonitor::UpdateHitman(const HitmanSnapshot& snapshot) {
        const auto eventUpdate = eventTracker_.Observe(snapshot);
        if (snapshot.result != HitmanReadResult::Success) {
            return HandleUnavailable(hitman_, snapshot.result, lastHitmanResult_, "Hitman");
        }
        const auto update = hitman_.Observe(
            MakeState(snapshot.targets, &HitmanTargetStatus::locationTag, &HitmanTargetStatus::complete));
        if (update.kind == BaselineUpdateKind::Created) {
            const auto complete = std::count_if(snapshot.targets.begin(), snapshot.targets.end(),
                                                [](const auto& target) { return target.complete; });
            LogInfo("Hitman", "Baseline created: targets=" + std::to_string(snapshot.targets.size()) +
                                  " complete=" + std::to_string(complete));
        } else if (update.kind == BaselineUpdateKind::IdentityChanged) {
            LogInfo("Hitman", "Target identity changed; baseline recreated");
        }

        for (const auto& event : eventUpdate.events) {
            Emit(event);
        }

        LogBooleanChanges("Hitman", update.changes);
        lastHitmanResult_ = HitmanReadResult::Success;
        return update.kind != BaselineUpdateKind::Unchanged;
    }

    bool ProgressionMonitor::UpdateChopShop(const ChopShopSnapshot& snapshot) {
        const auto eventUpdate = eventTracker_.Observe(snapshot);
        if (snapshot.result != ChopShopReadResult::Success) {
            return HandleUnavailable(chopShop_, snapshot.result, lastChopShopResult_, "ChopShop");
        }
        const auto update = chopShop_.Observe(
            MakeState(snapshot.vehicles, &ChopShopVehicleStatus::targetTag, &ChopShopVehicleStatus::retrieved));
        if (update.kind == BaselineUpdateKind::Created) {
            const auto retrieved = std::count_if(snapshot.vehicles.begin(), snapshot.vehicles.end(),
                                                 [](const auto& vehicle) { return vehicle.retrieved; });
            LogInfo("ChopShop", "Baseline created: vehicles=" + std::to_string(snapshot.vehicles.size()) +
                                    " retrieved=" + std::to_string(retrieved));
        } else if (update.kind == BaselineUpdateKind::IdentityChanged) {
            LogInfo("ChopShop", "Vehicle identity changed; baseline recreated");
        }
        for (const auto& event : eventUpdate.events) {
            Emit(event);
        }
        LogBooleanChanges("ChopShop", update.changes);
        lastChopShopResult_ = ChopShopReadResult::Success;
        return update.kind != BaselineUpdateKind::Unchanged;
    }

    bool ProgressionMonitor::UpdateMissions(const MissionSnapshot& snapshot) {
        const auto eventUpdate = eventTracker_.Observe(snapshot);
        if (snapshot.result != MissionReadResult::Success) {
            return HandleUnavailable(missions_, snapshot.result, lastMissionResult_, "Missions");
        }

        const auto update =
            missions_.Observe(MakeState(snapshot.missions, &MissionStatus::missionId, &MissionStatus::complete));
        if (update.kind == BaselineUpdateKind::Created) {
            LogInfo("Missions", "Baseline created: missions=" + std::to_string(snapshot.missions.size()));
        } else if (update.kind == BaselineUpdateKind::IdentityChanged) {
            LogInfo("Missions", "Mission identity changed; baseline recreated");
        }

        for (const auto& event : eventUpdate.events) {
            Emit(event);
        }

        LogBooleanChanges("Missions", update.changes);
        lastMissionResult_ = MissionReadResult::Success;
        return update.kind != BaselineUpdateKind::Unchanged;
    }

    bool ProgressionMonitor::UpdateActivities(const ActivitySnapshot& snapshot) {
        const auto eventUpdate = eventTracker_.Observe(snapshot);
        if (snapshot.result != ActivityReadResult::Success) {
            return HandleUnavailable(activities_, snapshot.result, lastActivityResult_, "Activities");
        }
        const auto update = activities_.Observe(MakeState(snapshot.instances, &ActivityInstanceStatus::instanceTag,
                                                          &ActivityInstanceStatus::completionFlags));
        if (update.kind == BaselineUpdateKind::Created) {
            LogInfo("Activities", "Baseline created: instances=" + std::to_string(snapshot.instances.size()));
        } else if (update.kind == BaselineUpdateKind::IdentityChanged) {
            LogInfo("Activities", "Activity identity changed; baseline recreated");
        }

        for (const auto& change : update.changes) {
            const auto instance = std::find_if(snapshot.instances.begin(), snapshot.instances.end(),
                                               [&](const auto& item) { return item.instanceTag == change.key; });
            std::uint32_t previousCompleted{};
            for (std::uint32_t level = 0; level < instance->totalLevels; ++level) {
                previousCompleted += (change.previous & (1u << level)) != 0 ? 1u : 0u;
            }
            const auto message = "Progress changed: " + change.key + " " + std::to_string(previousCompleted) + " -> " +
                                 std::to_string(instance->completedLevels) + " of " +
                                 std::to_string(instance->totalLevels);
            if (instance->completedLevels > previousCompleted) {
                LogInfo("Activities", message);
            } else {
                LogWarning("Activities", message);
            }
        }
        for (const auto& event : eventUpdate.events) {
            Emit(event);
        }
        lastActivityResult_ = ActivityReadResult::Success;
        return update.kind != BaselineUpdateKind::Unchanged;
    }

    bool ProgressionMonitor::UpdateCds(const CdSnapshot& snapshot) {
        const auto eventUpdate = eventTracker_.Observe(snapshot);
        if (snapshot.result != CdReadResult::Success) {
            const bool invalidated = cdBaselineValid_;
            if (invalidated) {
                LogInfo("CDs", std::string("Progression unavailable; baseline invalidated result=") +
                                   ToString(snapshot.result));
                cdBaseline_.clear();
                cdBaselineValid_ = false;
            } else if (snapshot.result != lastCdResult_) {
                LogDebug("CDs", std::string("Polling waiting result=") + ToString(snapshot.result));
            }
            lastCdResult_ = snapshot.result;
            return invalidated;
        }
        const std::unordered_set<std::uint32_t> current(snapshot.collectedIds.begin(), snapshot.collectedIds.end());
        if (!cdBaselineValid_) {
            cdBaseline_ = current;
            cdBaselineValid_ = true;
            LogInfo("CDs", "Baseline created: collected=" + std::to_string(snapshot.collectedIds.size()) + "/" +
                               std::to_string(snapshot.target));
            lastCdResult_ = CdReadResult::Success;
            return true;
        }
        if (current == cdBaseline_) {
            lastCdResult_ = CdReadResult::Success;
            return false;
        }
        for (const auto id : current) {
            if (cdBaseline_.find(id) == cdBaseline_.end()) {
                const auto key = FindCdDistrictKey(id);
                LogInfo("CDs", "Collected: " + std::string(key ? key : "unknown") + " id=" + Hex(id));
            }
        }
        for (const auto id : cdBaseline_) {
            if (current.find(id) == current.end()) {
                LogWarning("CDs", "Collection removed: " + Hex(id));
            }
        }
        cdBaseline_ = current;
        for (const auto& event : eventUpdate.events) {
            Emit(event);
        }
        lastCdResult_ = CdReadResult::Success;
        return true;
    }

    void ProgressionMonitor::Poll() {
        const auto snapshot = GetProgressionSnapshot();
        const bool changed = UpdateHitman(snapshot.hitman) | UpdateChopShop(snapshot.chopShop) |
                             UpdateMissions(snapshot.missions) | UpdateActivities(snapshot.activities) |
                             UpdateCds(snapshot.cds);
        if (changed && writeStatusFile_ && !WriteProgressionStatus(statusPath_, snapshot)) {
            LogWarning("Status", "Unable to replace diagnostic status file");
        }
    }
}  // namespace sr2ap
