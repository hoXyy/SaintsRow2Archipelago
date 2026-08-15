#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "sr2ap/ReaderResult.hpp"

namespace sr2ap {
    using HitmanReadResult = ReaderResult;

    struct HitmanTargetStatus {
        std::string locationTag;
        std::uint32_t listId{};
        std::uint32_t targetIndex{};
        bool complete{};
    };

    struct HitmanSnapshot {
        HitmanReadResult result{HitmanReadResult::ReaderUnavailable};
        std::vector<HitmanTargetStatus> targets;
    };

    HitmanSnapshot GetHitmanSnapshot();
    void LogHitmanSnapshot(const HitmanSnapshot& snapshot, bool full);
    bool WriteHitmanStatus(const std::filesystem::path& path, const HitmanSnapshot& snapshot);
}  // namespace sr2ap
