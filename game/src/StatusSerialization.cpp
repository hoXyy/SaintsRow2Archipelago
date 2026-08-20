#include "sr2ap/Status.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace sr2ap {
    std::string SerializeProgressionStatus(const ProgressionSnapshot& snapshot) {
        return SerializeProgressionStatus(snapshot.hitman, snapshot.chopShop, snapshot.missions, snapshot.activities,
                                          snapshot.racing, snapshot.cds);
    }

    std::string SerializeProgressionStatus(const HitmanSnapshot& hitman,
                                           const ChopShopSnapshot& chopShop,
                                           const MissionSnapshot& missions,
                                           const ActivitySnapshot& activities,
                                           const RacingSnapshot& racing,
                                           const CdSnapshot& cds) {
        std::ostringstream output;
        const auto hitmanComplete = std::count_if(hitman.targets.begin(), hitman.targets.end(),
                                                  [](const auto& value) { return value.complete; });
        const auto retrieved = std::count_if(chopShop.vehicles.begin(), chopShop.vehicles.end(),
                                             [](const auto& value) { return value.retrieved; });
        const auto missionsComplete = std::count_if(missions.missions.begin(), missions.missions.end(),
                                                    [](const auto& value) { return value.complete; });

        output << "game_ready=" << (hitman.result == HitmanReadResult::Success ? 1 : 0) << '\n'
               << "reader_ready=" << (hitman.result == HitmanReadResult::Success ? 1 : 0) << '\n'
               << "result=" << ToString(hitman.result) << '\n'
               << "target_count=" << hitman.targets.size() << '\n'
               << "complete_count=" << hitmanComplete << "\n\n[hitman]\n";
        if (hitman.result == HitmanReadResult::Success) {
            for (const auto& target : hitman.targets) {
                output << target.locationTag << '=' << (target.complete ? 1 : 0) << '\n';
            }
        }

        output << "\n[chop_shop]\nresult=" << ToString(chopShop.result)
               << "\nvehicle_count=" << chopShop.vehicles.size() << "\nretrieved_count=" << retrieved << '\n';
        if (chopShop.result == ChopShopReadResult::Success) {
            for (const auto& vehicle : chopShop.vehicles) {
                output << vehicle.targetTag << '=' << (vehicle.retrieved ? 1 : 0) << '\n';
            }
        }

        output << "\n[missions]\nresult=" << ToString(missions.result) << "\nmission_count=" << missions.missions.size()
               << "\ncomplete_count=" << missionsComplete << '\n';
        if (missions.result == MissionReadResult::Success) {
            for (const auto& mission : missions.missions) {
                output << mission.missionId << '=' << (mission.complete ? 1 : 0) << '\n';
            }
        }

        const auto fullyComplete =
            std::count_if(activities.instances.begin(), activities.instances.end(),
                          [](const auto& value) { return value.completedLevels == value.totalLevels; });
        output << "\n[activities]\nresult=" << ToString(activities.result)
               << "\ninstance_count=" << activities.instances.size() << "\nfully_complete_count=" << fullyComplete
               << '\n';
        if (activities.result == ActivityReadResult::Success) {
            for (const auto& instance : activities.instances) {
                output << instance.instanceTag << '=' << instance.completedLevels << '\n'
                       << instance.instanceTag << "_flags=0x" << std::uppercase << std::hex << std::setw(2)
                       << std::setfill('0') << static_cast<unsigned>(instance.completionFlags) << std::dec
                       << std::setfill(' ') << '\n';
            }
        }

        const auto medalCount = std::count_if(racing.races.begin(), racing.races.end(), [](const auto& race) {
            return race.medal == RacingMedal::Gold || race.medal == RacingMedal::Silver ||
                   race.medal == RacingMedal::Bronze;
        });
        output << "\n[racing]\nresult=" << ToString(racing.result) << "\nrace_count=" << racing.races.size()
               << "\nmedal_count=" << medalCount << '\n';
        if (racing.result == ReaderResult::Success) {
            for (const auto& race : racing.races) {
                output << race.name << '=' << RacingMedalRank(race.medal) << '\n'
                       << race.name << "_medal=" << ToString(race.medal) << '\n'
                       << race.name << "_best_time=" << race.bestTime << '\n';
            }
        }

        output << "\n[cds]\nresult=" << ToString(cds.result) << "\ntarget_count=" << cds.target
               << "\ncollected_count=" << cds.collectedIds.size() << '\n';
        if (cds.result == CdReadResult::Success) {
            const std::unordered_set<std::uint32_t> collected(cds.collectedIds.begin(), cds.collectedIds.end());
            std::vector<const CdDefinition*> definitions;
            for (const auto& definition : GetCdDefinitions()) {
                definitions.push_back(&definition);
            }
            std::sort(definitions.begin(), definitions.end(), [](const auto* left, const auto* right) {
                return std::string(left->districtKey) < std::string(right->districtKey);
            });
            for (const auto* definition : definitions) {
                output << definition->districtKey << '=' << (collected.count(definition->id) ? 1 : 0) << '\n';
            }
        }
        return output.str();
    }
}  // namespace sr2ap
