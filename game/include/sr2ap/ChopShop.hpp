#pragma once

#include "sr2ap/ReaderResult.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sr2ap {
    using ChopShopReadResult = ReaderResult;

    struct ChopShopVehicleStatus {
        std::string targetTag;
        std::uint32_t listId{};
        std::uint32_t vehicleIndex{};
        bool retrieved{};
        std::uint32_t cash{};
        std::uint32_t respect{};
    };

    struct ChopShopSnapshot {
        ChopShopReadResult result{ChopShopReadResult::ReaderUnavailable};
        std::vector<ChopShopVehicleStatus> vehicles;
    };

    ChopShopSnapshot GetChopShopSnapshot();
    void LogChopShopSnapshot(const ChopShopSnapshot& snapshot, bool full);
}  // namespace sr2ap
