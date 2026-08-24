#pragma once

#include "sr2ap/ReaderResult.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sr2ap {
    using CdReadResult = ReaderResult;

    struct CdSnapshot {
        CdReadResult result{CdReadResult::ReaderUnavailable};
        std::uint32_t target{};
        std::vector<std::uint32_t> collectedIds;
    };

    struct CdDefinition {
        std::uint32_t id;
        const char* districtKey;
    };

    CdSnapshot GetCdSnapshot();
    const std::vector<CdDefinition>& GetCdDefinitions();
    const char* FindCdDistrictKey(std::uint32_t id);
    void LogCdSnapshot(const CdSnapshot& snapshot, bool full);
}  // namespace sr2ap
