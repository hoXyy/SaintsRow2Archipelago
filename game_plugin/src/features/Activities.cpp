#include "Activities.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <sstream>
#include <unordered_set>

#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
constexpr std::size_t kExpectedInstanceCount = 24;
constexpr std::uint32_t kMaximumTableRows = 4096;
constexpr std::array<const char*, 12> kPrefixes{
    "fuzz_",  "fraud_",  "snatch_", "escort_", "demoderby_", "drug_",
    "crowd_", "sewage_", "fight_",  "mayhem_", "heli_",      "torch_"};

bool ReadTag(std::uintptr_t address, std::string& result) {
    const auto value = ReadFixedString(address, 32);
    if (!value) {
        return false;
    }
    result = *value;
    if (result.empty() ||
        !std::all_of(result.begin(), result.end(), [](unsigned char character) {
            return std::islower(character) || std::isdigit(character) ||
                   character == '_';
        })) {
        return false;
    }
    return std::any_of(
        kPrefixes.begin(), kPrefixes.end(),
        [&](const char* prefix) { return result.rfind(prefix, 0) == 0; });
}
}  // namespace

ActivitySnapshot GetActivitySnapshot(const GameContext& context) {
    ActivitySnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ActivityReadResult::UnsupportedVersion;
        return snapshot;
    }

    if (!context.IsLoaded()) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    const auto componentCount = ReadMemory<std::uint32_t>(
        game.base + addresses::kActivityComponentCountRva);

    const auto progressionCount = ReadMemory<std::uint32_t>(
        game.base + addresses::kActivityProgressionCountRva);

    if (!componentCount || componentCount == 0 ||
        componentCount > kMaximumTableRows || !progressionCount ||
        progressionCount == 0 || progressionCount > kMaximumTableRows) {
        snapshot.result = ActivityReadResult::ManagerUnavailable;
        return snapshot;
    }

    std::unordered_set<std::string> identities;
    for (std::uint32_t index = 0; index < componentCount; ++index) {
        const auto component = game.base +
                               addresses::kActivityComponentTableRva +
                               static_cast<std::uintptr_t>(index) *
                                   addresses::kActivityComponentStride;

        auto kind = ReadMemory<std::uint32_t>(
            component + addresses::kActivityComponentKindOffset);
        auto activity = ReadMemory<std::uint32_t>(
            component + addresses::kActivityComponentObjectOffset);

        if (!kind || kind.value() != 1 || !activity || !*activity) {
            continue;
        }

        if (!IsReadableAddress(reinterpret_cast<const void*>(*activity),
                               0x180)) {
            snapshot.result = ActivityReadResult::InvalidPointer;
            snapshot.instances.clear();
            return snapshot;
        }

        std::string tag;
        if (!ReadTag(*activity + addresses::kActivityInstanceTagOffset, tag)) {
            continue;
        }

        auto total = ReadMemory<std::uint32_t>(
            *activity + addresses::kActivityTotalLevelsOffset);
        auto key = ReadMemory<std::uint32_t>(
            *activity + addresses::kActivityProgressionKeyOffset);

        if (!total || !key || total.value() == 0 || total.value() > 8 ||
            !*key || !identities.emplace(tag).second) {
            snapshot.result = ActivityReadResult::InvalidData;
            snapshot.instances.clear();
            return snapshot;
        }

        std::uint8_t flags{};
        bool found = false;
        for (std::uint32_t row = 0; row < progressionCount; ++row) {
            const auto entry = game.base +
                               addresses::kActivityProgressionTableRva +
                               static_cast<std::uintptr_t>(row) *
                                   addresses::kActivityProgressionEntryStride;

            auto storedKey = ReadMemory<std::uint32_t>(entry);

            if (!storedKey) {
                snapshot.result = ActivityReadResult::InvalidPointer;
                snapshot.instances.clear();
                return snapshot;
            }
            if (storedKey != key) {
                continue;
            }
            found = SafeCopy(
                reinterpret_cast<const void*>(
                    entry + addresses::kActivityProgressionFlagsOffset),
                &flags, sizeof(flags));
            break;
        }

        const auto validMask = static_cast<std::uint8_t>(
            total.value() == 8 ? 0xFFu : (1u << total.value()) - 1u);

        if (!found || (flags & static_cast<std::uint8_t>(~validMask)) != 0) {
            snapshot.result = ActivityReadResult::InvalidData;
            snapshot.instances.clear();
            return snapshot;
        }

        std::uint32_t completed{};
        for (std::uint32_t level = 0; level < total; ++level) {
            if ((flags & (1u << level)) != 0) {
                ++completed;
            }
        }
        snapshot.instances.push_back(
            {std::move(tag), completed, total.value(), flags});
    }

    if (snapshot.instances.size() != kExpectedInstanceCount) {
        snapshot.result = ActivityReadResult::ManagerUnavailable;
        snapshot.instances.clear();
        return snapshot;
    }

    std::sort(snapshot.instances.begin(), snapshot.instances.end(),
              [](const auto& left, const auto& right) {
                  return left.instanceTag < right.instanceTag;
              });
    snapshot.result = ActivityReadResult::Success;
    return snapshot;
}

void LogActivitySnapshot(const ActivitySnapshot& snapshot, bool full) {
    const auto completedInstances = std::count_if(
        snapshot.instances.begin(), snapshot.instances.end(),
        [](const auto& instance) {
            return instance.completedLevels == instance.totalLevels;
        });
    std::ostringstream summary;
    summary << "[Manual snapshot] result=" << ToString(snapshot.result)
            << " instances=" << snapshot.instances.size()
            << " fully_complete=" << completedInstances;
    LogInfo("Activities", summary.str());
    if (full && snapshot.result == ActivityReadResult::Success) {
        for (const auto& instance : snapshot.instances) {
            LogInfo("Activities", instance.instanceTag + "=" +
                                      std::to_string(instance.completedLevels));
        }
    }
}
}  // namespace sr2ap
