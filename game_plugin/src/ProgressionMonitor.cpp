#include "sr2ap/ProgressionMonitor.hpp"

#include <algorithm>

#include "sr2ap/Collectibles.hpp"
#include "sr2ap/Helpers.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Status.hpp"

namespace sr2ap {
namespace {

template <class Result>
bool HandleUnavailable(BaselineUpdateKind kind, Result result, Result& previous,
                       const char* subsystem) {
    const bool invalidated = kind == BaselineUpdateKind::Invalidated;

    if (invalidated) {
        LogInfo(subsystem,
                std::string(
                    "Progression unavailable; baseline invalidated result=") +
                    ToString(result));
    } else if (result != previous) {
        LogDebug(subsystem,
                 std::string("Polling waiting result=") + ToString(result));
    }

    previous = result;
    return invalidated;
}

void LogBooleanChanges(
    const char* subsystem,
    const std::vector<BaselineChange<std::string, bool>>& changes) {
    for (const auto& change : changes) {
        const auto message = "Completion changed: " + change.key +
                             (change.previous ? " 1 -> 0" : " 0 -> 1");
        if (change.current) {
            LogInfo(subsystem, message);
        } else {
            LogWarning(subsystem, message);
        }
    }
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
    const auto observation = eventTracker_.Observe(snapshot);
    const auto& update = observation.baseline;

    if (snapshot.result != HitmanReadResult::Success) {
        return HandleUnavailable(update.kind, snapshot.result,
                                 lastHitmanResult_, "Hitman");
    }

    if (update.kind == BaselineUpdateKind::Created) {
        const auto complete =
            std::count_if(snapshot.targets.begin(), snapshot.targets.end(),
                          [](const auto& target) { return target.complete; });

        LogInfo("Hitman", "Baseline created: targets=" +
                              std::to_string(snapshot.targets.size()) +
                              " complete=" + std::to_string(complete));
    } else if (update.kind == BaselineUpdateKind::IdentityChanged) {
        LogInfo("Hitman", "Target identity changed; baseline recreated");
    }

    for (const auto& event : observation.events) {
        Emit(event);
    }

    LogBooleanChanges("Hitman", update.changes);
    lastHitmanResult_ = HitmanReadResult::Success;
    return update.kind != BaselineUpdateKind::Unchanged;
}

bool ProgressionMonitor::UpdateChopShop(const ChopShopSnapshot& snapshot) {
    const auto observation = eventTracker_.Observe(snapshot);
    const auto& update = observation.baseline;

    if (snapshot.result != ChopShopReadResult::Success) {
        return HandleUnavailable(update.kind, snapshot.result,
                                 lastChopShopResult_, "ChopShop");
    }

    if (update.kind == BaselineUpdateKind::Created) {
        const auto complete = std::count_if(
            snapshot.vehicles.begin(), snapshot.vehicles.end(),
            [](const auto& vehicle) { return vehicle.retrieved; });

        LogInfo("ChopShop", "Baseline created: vehicles=" +
                                std::to_string(snapshot.vehicles.size()) +
                                " retrieved=" + std::to_string(complete));
    } else if (update.kind == BaselineUpdateKind::IdentityChanged) {
        LogInfo("ChopShop", "Vehicle identity changed; baseline recreated");
    }

    for (const auto& event : observation.events) {
        Emit(event);
    }

    LogBooleanChanges("ChopShop", update.changes);
    lastChopShopResult_ = ChopShopReadResult::Success;
    return update.kind != BaselineUpdateKind::Unchanged;
}

bool ProgressionMonitor::UpdateMissions(const MissionSnapshot& snapshot) {
    const auto observation = eventTracker_.Observe(snapshot);
    const auto& update = observation.baseline;

    if (snapshot.result != MissionReadResult::Success) {
        return HandleUnavailable(update.kind, snapshot.result,
                                 lastMissionResult_, "Missions");
    }

    if (update.kind == BaselineUpdateKind::Created) {
        LogInfo("Missions", "Baseline created: missions=" +
                                std::to_string(snapshot.missions.size()));
    } else if (update.kind == BaselineUpdateKind::IdentityChanged) {
        LogInfo("Missions", "Mission identity changed; baseline recreated");
    }

    for (const auto& event : observation.events) {
        Emit(event);
    }

    LogBooleanChanges("Missions", update.changes);
    lastMissionResult_ = MissionReadResult::Success;
    return update.kind != BaselineUpdateKind::Unchanged;
}

bool ProgressionMonitor::UpdateActivities(const ActivitySnapshot& snapshot) {
    const auto observation = eventTracker_.Observe(snapshot);
    const auto& update = observation.baseline;

    if (snapshot.result != ActivityReadResult::Success) {
        return HandleUnavailable(update.kind, snapshot.result,
                                 lastActivityResult_, "Activities");
    }

    if (update.kind == BaselineUpdateKind::Created) {
        LogInfo("Activities", "Baseline created: instances=" +
                                  std::to_string(snapshot.instances.size()));
    } else if (update.kind == BaselineUpdateKind::IdentityChanged) {
        LogInfo("Activities", "Activity identity changed; baseline recreated");
    }

    for (const auto& change : update.changes) {
        const auto instance = std::find_if(
            snapshot.instances.begin(), snapshot.instances.end(),
            [&](const auto& item) { return item.instanceTag == change.key; });
        std::uint32_t previousCompleted{};
        for (std::uint32_t level = 0; level < instance->totalLevels; ++level) {
            previousCompleted +=
                (change.previous & (1u << level)) != 0 ? 1u : 0u;
        }
        const auto message = "Progress changed: " + change.key + " " +
                             std::to_string(previousCompleted) + " -> " +
                             std::to_string(instance->completedLevels) +
                             " of " + std::to_string(instance->totalLevels);
        if (instance->completedLevels > previousCompleted) {
            LogInfo("Activities", message);
        } else {
            LogWarning("Activities", message);
        }
    }

    for (const auto& event : observation.events) {
        Emit(event);
    }

    lastActivityResult_ = ActivityReadResult::Success;
    return update.kind != BaselineUpdateKind::Unchanged;
}

bool ProgressionMonitor::UpdateRacing(const RacingSnapshot& snapshot) {
    const auto observation = eventTracker_.Observe(snapshot);
    const auto& update = observation.baseline;

    if (snapshot.result != ReaderResult::Success)
        return HandleUnavailable(update.kind, snapshot.result,
                                 lastRacingResult_, "Racing");

    if (update.kind == BaselineUpdateKind::Created) {
        LogInfo("Racing", "Baseline created: races=" +
                              std::to_string(snapshot.races.size()));
    } else if (update.kind == BaselineUpdateKind::IdentityChanged) {
        LogInfo("Racing", "Race identity changed; baseline recreated");
    }

    for (const auto& change : update.changes) {
        const auto message = "Medal changed: " + std::string{change.key} + " " +
                             ToString(change.previous) + " -> " +
                             ToString(change.current);
        const bool improved = change.current != RacingMedal::Unattempted &&
                              (change.previous == RacingMedal::Unattempted ||
                               change.current < change.previous);
        if (improved)
            LogInfo("Racing", message);
        else
            LogWarning("Racing", message);
    }

    for (const auto& event : observation.events) {
        Emit(event);
    }

    lastRacingResult_ = ReaderResult::Success;
    return update.kind != BaselineUpdateKind::Unchanged;
}

bool ProgressionMonitor::UpdateCds(const CdSnapshot& snapshot) {
    const auto observation = eventTracker_.Observe(snapshot);
    const auto& update = observation.baseline;

    if (snapshot.result != CdReadResult::Success) {
        return HandleUnavailable(update.kind, snapshot.result, lastCdResult_,
                                 "CDs");
    }

    if (update.kind == BaselineUpdateKind::Created) {
        LogInfo("CDs", "Baseline created: collected=" +
                           std::to_string(snapshot.collectedIds.size()) + "/" +
                           std::to_string(snapshot.target));
    }

    for (const auto& change : update.changes) {
        const auto* key = FindCdDistrictKey(change.key);

        if (change.current) {
            LogInfo("CDs", "Collected: " + std::string(key ? key : "unknown") +
                               " id=" + Hex(change.key));
        } else {
            LogWarning("CDs", "Collection removed: " + Hex(change.key));
        }
    }

    for (const auto& event : observation.events) {
        Emit(event);
    }

    lastCdResult_ = CdReadResult::Success;
    return update.kind != BaselineUpdateKind::Unchanged;
}

bool ProgressionMonitor::UpdateStyleLevel(const StyleLevelSnapshot& snapshot) {
    const auto observation = eventTracker_.Observe(snapshot);
    const auto& update = observation.baseline;

    if (snapshot.result != ReaderResult::Success) {
        return HandleUnavailable(update.kind, snapshot.result,
                                 lastStyleLevelResult_, "StyleLevel");
    }

    if (update.kind == BaselineUpdateKind::Created) {
        LogInfo("StyleLevel", "Baseline created: stored_level=" +
                                  std::to_string(snapshot.storedLevel) +
                                  " displayed_level=" +
                                  std::to_string(snapshot.displayedLevel) +
                                  " points=" + std::to_string(snapshot.points));
    }

    for (const auto& change : update.changes) {
        const auto message =
            "Level changed: " + std::to_string(change.previous) + " -> " +
            std::to_string(change.current) +
            " points=" + std::to_string(snapshot.points);
        if (change.current > change.previous) {
            LogInfo("StyleLevel", message);
        } else {
            LogWarning("StyleLevel", message);
        }
    }

    for (const auto& event : observation.events) {
        Emit(event);
    }

    lastStyleLevelResult_ = ReaderResult::Success;
    return update.kind != BaselineUpdateKind::Unchanged;
}

void ProgressionMonitor::Poll() {
    const auto snapshot = GetProgressionSnapshot();
    bool changed = false;
    changed |= UpdateHitman(snapshot.hitman);
    changed |= UpdateChopShop(snapshot.chopShop);
    changed |= UpdateMissions(snapshot.missions);
    changed |= UpdateActivities(snapshot.activities);
    changed |= UpdateRacing(snapshot.racing);
    changed |= UpdateCds(snapshot.cds);
    changed |= UpdateStyleLevel(snapshot.styleLevel);

    if (changed && writeStatusFile_ &&
        !WriteProgressionStatus(statusPath_, snapshot)) {
        LogWarning("Status", "Unable to replace diagnostic status file");
    }
}
}  // namespace sr2ap
