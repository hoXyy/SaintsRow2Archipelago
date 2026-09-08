#pragma once

#include <filesystem>
#include <string>

#include "features/Activities.hpp"
#include "features/ChopShop.hpp"
#include "features/Collectibles.hpp"
#include "features/Hitman.hpp"
#include "features/Missions.hpp"
#include "features/Racing.hpp"
#include "features/StyleLevel.hpp"
#include "progression/Progression.hpp"

namespace sr2ap {
[[nodiscard]] std::string SerializeProgressionStatus(
    const ProgressionSnapshot& snapshot);
[[nodiscard]] std::string SerializeProgressionStatus(
    const HitmanSnapshot& hitman, const ChopShopSnapshot& chopShop,
    const MissionSnapshot& missions, const ActivitySnapshot& activities,
    const RacingSnapshot& racing, const CdSnapshot& cds,
    const StyleLevelSnapshot& styleLevel);
bool WriteProgressionStatus(const std::filesystem::path& path,
                            const ProgressionSnapshot& snapshot);
bool WriteProgressionStatus(const std::filesystem::path& path,
                            const HitmanSnapshot& hitman,
                            const ChopShopSnapshot& chopShop,
                            const MissionSnapshot& missions,
                            const ActivitySnapshot& activities,
                            const RacingSnapshot& racing, const CdSnapshot& cds,
                            const StyleLevelSnapshot& styleLevel);
}  // namespace sr2ap
