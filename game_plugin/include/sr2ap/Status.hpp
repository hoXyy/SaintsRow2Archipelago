#pragma once

#include <filesystem>
#include <string>

#include "sr2ap/Activities.hpp"
#include "sr2ap/ChopShop.hpp"
#include "sr2ap/Collectibles.hpp"
#include "sr2ap/Hitman.hpp"
#include "sr2ap/Missions.hpp"
#include "sr2ap/Progression.hpp"
#include "sr2ap/Racing.hpp"
#include "sr2ap/StyleLevel.hpp"

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
