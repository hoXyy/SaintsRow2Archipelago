#include "MissionReader.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fmt/format.h"
#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "progression/BaselineTracker.hpp"
#include "progression/ProgressionReader.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
namespace {
constexpr std::string_view kReaderId{"mission"};

constexpr std::array<const char*, 56> kBaseGameMissions{"tss01",
                                                        "tss02",
                                                        "tss03",
                                                        "tss04",
                                                        "sh_tss_caverns",
                                                        "bh01",
                                                        "bh02",
                                                        "bh03",
                                                        "bh04",
                                                        "bh05",
                                                        "bh06",
                                                        "bh07",
                                                        "bh08",
                                                        "bh09",
                                                        "bh10",
                                                        "bh11",
                                                        "sh_bh_apartments",
                                                        "sh_bh_chinatown",
                                                        "sh_bh_docks",
                                                        "sh_bh_airport",
                                                        "rn01",
                                                        "rn02",
                                                        "rn03",
                                                        "rn04",
                                                        "rn05",
                                                        "rn06",
                                                        "rn07",
                                                        "rn08",
                                                        "rn09",
                                                        "rn10",
                                                        "rn11",
                                                        "sh_rn_stripclub",
                                                        "sh_rn_sciencemuseum",
                                                        "sh_rn_museum_pier",
                                                        "sh_rn_rec_center",
                                                        "ss01",
                                                        "ss02",
                                                        "ss03",
                                                        "ss04",
                                                        "ss05",
                                                        "ss06",
                                                        "ss07",
                                                        "ss08",
                                                        "ss09",
                                                        "ss10",
                                                        "ss11",
                                                        "sh_ss_trailerpark",
                                                        "sh_ss_crackhouse",
                                                        "sh_ss_student_union",
                                                        "sh_ss_fishingdock",
                                                        "ep01",
                                                        "ep02",
                                                        "ep03",
                                                        "ep04",
                                                        "sh_tss_ugmall",
                                                        "em01"};

using MissionCompletedFunction = bool(__thiscall*)(const char*);

struct MissionStatus {
    std::string missionId;
    bool complete{};
};

struct MissionSnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::vector<MissionStatus> missions;
};

MissionSnapshot ReadSnapshot(const GameContext& context) {
    MissionSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    if (!context.IsLoaded()) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    const auto address = game.base + addresses::kMissionCompletedQueryRva;
    if (!IsInsideModule(game.handle, reinterpret_cast<const void*>(address)) ||
        !IsExecutableAddress(reinterpret_cast<const void*>(address)) ||
        DetectDetour(reinterpret_cast<const void*>(address)) !=
            DetourKind::None) {
        snapshot.result = ReaderResult::InvalidFunction;
        return snapshot;
    }

    const auto query = reinterpret_cast<MissionCompletedFunction>(address);
    snapshot.missions.reserve(kBaseGameMissions.size());
    for (const auto* mission : kBaseGameMissions) {
        snapshot.missions.push_back({mission, query(mission)});
    }
    snapshot.result = ReaderResult::Success;
    return snapshot;
}

std::string SerializeStatus(const MissionSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return fmt::format("[{}]\nresult={}\n", kReaderId,
                           ToString(snapshot.result));
    }

    const auto missionsComplete =
        std::count_if(snapshot.missions.begin(), snapshot.missions.end(),
                      [](const auto& value) { return value.complete; });

    std::string output = fmt::format(
        "[{}]\n"
        "result={}\n"
        "mission_count={}\n"
        "complete_count={}\n",
        kReaderId, ToString(snapshot.result), snapshot.missions.size(),
        missionsComplete);

    for (const auto& mission : snapshot.missions) {
        output += fmt::format("{}={}\n", mission.missionId, mission.complete);
    }

    return output;
}

class MissionReader final : public ProgressionReader {
   public:
    std::string_view Id() const noexcept override {
        return kReaderId;
    }

    ReaderUpdate Poll(const GameContext& context) override;

    std::string CaptureStatus(const GameContext& context) const override {
        return SerializeStatus(ReadSnapshot(context));
    }

   private:
    BaselineTracker<std::string, bool> baseline_;
    ReaderResult previousResult_{ReaderResult::ReaderUnavailable};
};

ReaderUpdate MissionReader::Poll(const GameContext& context) {
    const auto snapshot = ReadSnapshot(context);
    ReaderUpdate result;
    result.statusSection = SerializeStatus(snapshot);

    if (snapshot.result != ReaderResult::Success) {
        const auto invalidation = baseline_.Invalidate();

        result.changed = invalidation.kind != BaselineUpdateKind::Unchanged ||
                         snapshot.result != previousResult_;

        previousResult_ = snapshot.result;
        return result;
    }

    std::unordered_map<std::string, bool> current;
    current.reserve(snapshot.missions.size());

    for (const auto& mission : snapshot.missions) {
        current.emplace(mission.missionId, mission.complete);
    }

    const auto update = baseline_.Observe(std::move(current));
    result.changed = update.kind != BaselineUpdateKind::Unchanged;

    if (update.kind == BaselineUpdateKind::Created ||
        update.kind == BaselineUpdateKind::IdentityChanged) {
        for (const auto& mission : snapshot.missions) {
            if (!mission.complete) {
                continue;
            }

            result.events.push_back({
                .category = std::string{kReaderId},
                .key = mission.missionId,
                .previous = 0,
                .current = 1,
            });
        }
    }

    for (const auto& change : update.changes) {
        result.events.push_back({
            .category = std::string{kReaderId},
            .key = change.key,
            .previous = change.previous ? 1U : 0U,
            .current = change.current ? 1U : 0U,
        });
    }

    previousResult_ = ReaderResult::Success;

    return result;
}
}  // namespace

ProgressionReaderPtr CreateMissionReader() {
    return std::make_unique<MissionReader>();
}
}  // namespace sr2ap