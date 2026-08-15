#include "sr2ap/Missions.hpp"
#include "sr2ap/Addresses.hpp"
#include "sr2ap/Hitman.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <windows.h>
#include <array>
#include <sstream>

namespace sr2ap {
    namespace {

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
    }  // namespace

    MissionSnapshot GetMissionSnapshot() {
        MissionSnapshot snapshot;
        const auto game = InspectSupportedGameModule();
        if (!game) {
            snapshot.result = MissionReadResult::UnsupportedVersion;
            return snapshot;
        }

        // The hitman stuff is the "Game is ready" gate since that was the first thing implemented lol
        if (GetHitmanSnapshot().result != HitmanReadResult::Success) {
            snapshot.result = MissionReadResult::GameNotReady;
            return snapshot;
        }
        const auto address = game->base + addresses::kMissionCompletedQueryRva;
        if (!IsInsideModule(game->handle, reinterpret_cast<const void*>(address)) ||
            !IsExecutableAddress(reinterpret_cast<const void*>(address)) ||
            DetectDetour(reinterpret_cast<const void*>(address)) != DetourKind::None) {
            snapshot.result = MissionReadResult::InvalidFunction;
            return snapshot;
        }
        const auto query = reinterpret_cast<MissionCompletedFunction>(address);
        snapshot.missions.reserve(kBaseGameMissions.size());
        for (const auto* mission : kBaseGameMissions) {
            snapshot.missions.push_back({mission, query(mission)});
        }
        snapshot.result = MissionReadResult::Success;
        return snapshot;
    }

    void LogMissionSnapshot(const MissionSnapshot& snapshot, bool full) {
        std::size_t complete = 0;
        for (const auto& mission : snapshot.missions) {
            if (mission.complete) {
                ++complete;
            }
        }
        std::ostringstream summary;
        summary << "[Manual snapshot] result=" << ToString(snapshot.result) << " missions=" << snapshot.missions.size()
                << " complete=" << complete;
        LogInfo("Missions", summary.str());
        if (full && snapshot.result == MissionReadResult::Success) {
            for (const auto& mission : snapshot.missions) {
                LogInfo("Missions", mission.missionId + "=" + (mission.complete ? "1" : "0"));
            }
        }
    }
}  // namespace sr2ap
