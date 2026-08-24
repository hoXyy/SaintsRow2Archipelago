#include "sr2ap/Activities.hpp"
#include "sr2ap/Addresses.hpp"
#include "sr2ap/Hitman.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace sr2ap {
    namespace {
        constexpr std::size_t kExpectedInstanceCount = 24;
        constexpr std::uint32_t kMaximumTableRows = 4096;
        constexpr std::array<const char*, 12> kPrefixes{"fuzz_",      "fraud_",  "snatch_", "escort_",
                                                        "demoderby_", "drug_",   "crowd_",  "sewage_",
                                                        "fight_",     "mayhem_", "heli_",   "torch_"};

        bool ReadTag(std::uintptr_t address, std::string& result) {
            const auto value = ReadFixedString(address, 32);
            if (!value) {
                return false;
            }
            result = *value;
            if (result.empty() || !std::all_of(result.begin(), result.end(), [](unsigned char character) {
                    return std::islower(character) || std::isdigit(character) || character == '_';
                })) {
                return false;
            }
            return std::any_of(kPrefixes.begin(), kPrefixes.end(),
                               [&](const char* prefix) { return result.rfind(prefix, 0) == 0; });
        }
    }  // namespace

    ActivitySnapshot GetActivitySnapshot() {
        ActivitySnapshot snapshot;
        const auto game = InspectSupportedGameModule();
        if (!game) {
            snapshot.result = ActivityReadResult::UnsupportedVersion;
            return snapshot;
        }
        if (GetHitmanSnapshot().result != HitmanReadResult::Success) {
            snapshot.result = ActivityReadResult::GameNotReady;
            return snapshot;
        }
        std::uint32_t componentCount{}, progressionCount{};
        if (!SafeCopy(reinterpret_cast<const void*>(game->base + addresses::kActivityComponentCountRva),
                      &componentCount, sizeof(componentCount)) ||
            componentCount == 0 || componentCount > kMaximumTableRows ||
            !SafeCopy(reinterpret_cast<const void*>(game->base + addresses::kActivityProgressionCountRva),
                      &progressionCount, sizeof(progressionCount)) ||
            progressionCount == 0 || progressionCount > kMaximumTableRows) {
            snapshot.result = ActivityReadResult::ManagerUnavailable;
            return snapshot;
        }
        std::unordered_set<std::string> identities;
        for (std::uint32_t index = 0; index < componentCount; ++index) {
            const auto component = game->base + addresses::kActivityComponentTableRva +
                                   static_cast<std::uintptr_t>(index) * addresses::kActivityComponentStride;
            std::uint32_t kind{}, activity{};
            if (!SafeCopy(reinterpret_cast<const void*>(component + addresses::kActivityComponentKindOffset), &kind,
                          sizeof(kind)) ||
                kind != 1 ||
                !SafeCopy(reinterpret_cast<const void*>(component + addresses::kActivityComponentObjectOffset),
                          &activity, sizeof(activity)) ||
                !activity) {
                continue;
            }
            if (!IsReadableAddress(reinterpret_cast<const void*>(activity), 0x180)) {
                snapshot.result = ActivityReadResult::InvalidPointer;
                snapshot.instances.clear();
                return snapshot;
            }
            std::string tag;
            if (!ReadTag(activity + addresses::kActivityInstanceTagOffset, tag)) {
                continue;
            }
            std::uint32_t total{}, key{};
            if (!SafeCopy(reinterpret_cast<const void*>(activity + addresses::kActivityTotalLevelsOffset), &total,
                          sizeof(total)) ||
                !SafeCopy(reinterpret_cast<const void*>(activity + addresses::kActivityProgressionKeyOffset), &key,
                          sizeof(key)) ||
                total == 0 || total > 8 || !key || !identities.emplace(tag).second) {
                snapshot.result = ActivityReadResult::InvalidData;
                snapshot.instances.clear();
                return snapshot;
            }
            std::uint8_t flags{};
            bool found = false;
            for (std::uint32_t row = 0; row < progressionCount; ++row) {
                const auto entry = game->base + addresses::kActivityProgressionTableRva +
                                   static_cast<std::uintptr_t>(row) * addresses::kActivityProgressionEntryStride;
                std::uint32_t storedKey{};
                if (!SafeCopy(reinterpret_cast<const void*>(entry), &storedKey, sizeof(storedKey))) {
                    snapshot.result = ActivityReadResult::InvalidPointer;
                    snapshot.instances.clear();
                    return snapshot;
                }
                if (storedKey != key) {
                    continue;
                }
                found = SafeCopy(reinterpret_cast<const void*>(entry + addresses::kActivityProgressionFlagsOffset),
                                 &flags, sizeof(flags));
                break;
            }
            const auto validMask = static_cast<std::uint8_t>(total == 8 ? 0xFFu : (1u << total) - 1u);
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
            snapshot.instances.push_back({std::move(tag), completed, total, flags});
        }
        if (snapshot.instances.size() != kExpectedInstanceCount) {
            snapshot.result = ActivityReadResult::ManagerUnavailable;
            snapshot.instances.clear();
            return snapshot;
        }
        std::sort(snapshot.instances.begin(), snapshot.instances.end(),
                  [](const auto& left, const auto& right) { return left.instanceTag < right.instanceTag; });
        snapshot.result = ActivityReadResult::Success;
        return snapshot;
    }

    void LogActivitySnapshot(const ActivitySnapshot& snapshot, bool full) {
        const auto completedInstances =
            std::count_if(snapshot.instances.begin(), snapshot.instances.end(),
                          [](const auto& instance) { return instance.completedLevels == instance.totalLevels; });
        std::ostringstream summary;
        summary << "[Manual snapshot] result=" << ToString(snapshot.result)
                << " instances=" << snapshot.instances.size() << " fully_complete=" << completedInstances;
        LogInfo("Activities", summary.str());
        if (full && snapshot.result == ActivityReadResult::Success) {
            for (const auto& instance : snapshot.instances) {
                LogInfo("Activities", instance.instanceTag + "=" + std::to_string(instance.completedLevels));
            }
        }
    }
}  // namespace sr2ap
