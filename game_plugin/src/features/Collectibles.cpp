#include "Collectibles.hpp"

#include <windows.h>

#include <optional>
#include <unordered_set>

#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Helpers.hpp"
#include "util/Logger.hpp"

namespace sr2ap {

CdSnapshot GetCdSnapshot(const GameContext& context) {
    CdSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = CdReadResult::UnsupportedVersion;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    std::optional<std::uint32_t> manager = ReadMemory<std::uint32_t>(
        game.base + addresses::kCollectibleManagerRva);
    if (!manager || !*manager) {
        snapshot.result = CdReadResult::ManagerUnavailable;
        return snapshot;
    }

    std::optional<std::uint32_t> count = ReadMemory<std::uint32_t>(
        *manager + addresses::kCollectedCdCountOffset);

    std::optional<std::uint32_t> target = ReadMemory<std::uint32_t>(
        *manager + addresses::kCollectedCdTargetOffset);

    if (!count || !target) {
        snapshot.result = CdReadResult::InvalidPointer;
        return snapshot;
    }

    snapshot.target = *target;

    if (snapshot.target == 0) {
        snapshot.result = CdReadResult::GameNotReady;
        return snapshot;
    }

    if (snapshot.target != 50 || count > addresses::kCollectedCdCapacity ||
        count > snapshot.target) {
        snapshot.result = CdReadResult::InvalidData;
        return snapshot;
    }

    std::unordered_set<std::uint32_t> identities;
    snapshot.collectedIds.reserve(*count);

    for (std::uint32_t index = 0; index < count; ++index) {
        std::optional<std::uint32_t> id = ReadMemory<std::uint32_t>(
            *manager + addresses::kCollectedCdIdsOffset + index * 4);

        if (!id) {
            snapshot.result = CdReadResult::InvalidPointer;
            snapshot.collectedIds.clear();
            return snapshot;
        }

        if (!*id || !identities.emplace(*id).second) {
            snapshot.result = CdReadResult::InvalidData;
            snapshot.collectedIds.clear();
            return snapshot;
        }

        snapshot.collectedIds.push_back(*id);
    }

    snapshot.result = CdReadResult::Success;
    return snapshot;
}

void LogCdSnapshot(const CdSnapshot& snapshot, bool full) {
    LogInfo(
        "CDs",
        "[Manual snapshot] result=" + std::string(ToString(snapshot.result)) +
            " collected=" + std::to_string(snapshot.collectedIds.size()) + "/" +
            std::to_string(snapshot.target));
    if (full && snapshot.result == CdReadResult::Success) {
        for (const auto id : snapshot.collectedIds) {
            const auto key = FindCdDistrictKey(id);
            LogInfo("CDs", std::string(key ? key : "unknown") +
                               " id=" + Hex(id) + " complete=1");
        }
    }
}
}  // namespace sr2ap
