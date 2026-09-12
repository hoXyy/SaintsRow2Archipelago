#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "game/GameState.hpp"
#include "util/ReaderResult.hpp"

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

HitmanSnapshot GetHitmanSnapshot(const GameContext& context);
}  // namespace sr2ap
