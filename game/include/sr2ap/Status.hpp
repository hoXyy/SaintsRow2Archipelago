#pragma once

#include "sr2ap/Activities.hpp"
#include "sr2ap/ChopShop.hpp"
#include "sr2ap/Collectibles.hpp"
#include "sr2ap/Hitman.hpp"
#include "sr2ap/Missions.hpp"
#include "sr2ap/Progression.hpp"

#include <filesystem>
#include <string>

namespace sr2ap {
    [[nodiscard]] std::string SerializeProgressionStatus(const ProgressionSnapshot& snapshot);
    [[nodiscard]] std::string SerializeProgressionStatus(const HitmanSnapshot& hitman,
                                                         const ChopShopSnapshot& chopShop,
                                                         const MissionSnapshot& missions,
                                                         const ActivitySnapshot& activities,
                                                         const CdSnapshot& cds);
    bool WriteProgressionStatus(const std::filesystem::path& path, const ProgressionSnapshot& snapshot);
    bool WriteProgressionStatus(const std::filesystem::path& path,
                                const HitmanSnapshot& hitman,
                                const ChopShopSnapshot& chopShop,
                                const MissionSnapshot& missions,
                                const ActivitySnapshot& activities,
                                const CdSnapshot& cds);
}  // namespace sr2ap
