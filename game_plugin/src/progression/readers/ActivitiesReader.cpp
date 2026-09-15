#include "ActivitiesReader.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "fmt/format.h"
#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "progression/BaselineTracker.hpp"
#include "progression/ProgressionReader.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
namespace {
constexpr std::string_view kReaderId{"activity"};

struct ActivityInstanceStatus {
    std::string instanceTag;
    std::uint32_t completedLevels{};
    std::uint32_t totalLevels{};
    std::uint8_t completionFlags{};
};

struct ActivitySnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::vector<ActivityInstanceStatus> instances;
};

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

ActivitySnapshot ReadSnapshot(const GameContext& context) {
    ActivitySnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
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
        snapshot.result = ReaderResult::ManagerUnavailable;
        return snapshot;
    }

    std::unordered_set<std::string> identities;
    for (std::uint32_t index = 0; index < *componentCount; ++index) {
        const auto component = game.base +
                               addresses::kActivityComponentTableRva +
                               static_cast<std::uintptr_t>(index) *
                                   addresses::kActivityComponentStride;

        auto kind = ReadMemory<std::uint32_t>(
            component + addresses::kActivityComponentKindOffset);
        auto activity = ReadMemory<std::uint32_t>(
            component + addresses::kActivityComponentObjectOffset);

        if (!kind || *kind != 1 || !activity || !*activity) {
            continue;
        }

        if (!IsReadableAddress(*activity, 0x180)) {
            snapshot.result = ReaderResult::InvalidPointer;
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
            snapshot.result = ReaderResult::InvalidData;
            snapshot.instances.clear();
            return snapshot;
        }

        std::uint8_t flags{};
        bool found = false;
        for (std::uint32_t row = 0; row < *progressionCount; ++row) {
            const auto entry = game.base +
                               addresses::kActivityProgressionTableRva +
                               static_cast<std::uintptr_t>(row) *
                                   addresses::kActivityProgressionEntryStride;

            auto storedKey = ReadMemory<std::uint32_t>(entry);

            if (!storedKey) {
                snapshot.result = ReaderResult::InvalidPointer;
                snapshot.instances.clear();
                return snapshot;
            }
            if (storedKey != key) {
                continue;
            }
            found = ReadMemoryIntoValue(
                entry + addresses::kActivityProgressionFlagsOffset, &flags);

            break;
        }

        const auto validMask = static_cast<std::uint8_t>(
            total.value() == 8 ? 0xFFu : (1u << total.value()) - 1u);

        if (!found || (flags & static_cast<std::uint8_t>(~validMask)) != 0) {
            snapshot.result = ReaderResult::InvalidData;
            snapshot.instances.clear();
            return snapshot;
        }

        std::uint32_t completed{};
        for (std::uint32_t level = 0; level < *total; ++level) {
            if ((flags & (1u << level)) != 0) {
                ++completed;
            }
        }
        snapshot.instances.push_back(
            {std::move(tag), completed, total.value(), flags});
    }

    if (snapshot.instances.size() != kExpectedInstanceCount) {
        snapshot.result = ReaderResult::ManagerUnavailable;
        snapshot.instances.clear();
        return snapshot;
    }

    std::sort(snapshot.instances.begin(), snapshot.instances.end(),
              [](const auto& left, const auto& right) {
                  return left.instanceTag < right.instanceTag;
              });
    snapshot.result = ReaderResult::Success;
    return snapshot;
}

std::string SerializeStatus(const ActivitySnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return fmt::format("[{}]\nresult={}\n", kReaderId,
                           ToString(snapshot.result));
    }

    const auto fullyComplete =
        std::count_if(snapshot.instances.begin(), snapshot.instances.end(),
                      [](const auto& value) {
                          return value.completedLevels == value.totalLevels;
                      });

    std::string output = fmt::format(
        "[{}]\n"
        "result={}\n"
        "instance_count={}\n"
        "fully_completed_instances={}\n",
        kReaderId, ToString(snapshot.result), snapshot.instances.size(),
        fullyComplete);

    for (const auto& instance : snapshot.instances) {
        output += fmt::format("{}={}\n", instance.instanceTag,
                              instance.completedLevels);
    }

    return output;
}

class ActivitiesReader final : public ProgressionReader {
   public:
    std::string_view Id() const noexcept override {
        return kReaderId;
    }

    ReaderUpdate Poll(const GameContext& context) override {
        const auto snapshot = ReadSnapshot(context);
        ReaderUpdate result;
        result.statusSection = SerializeStatus(snapshot);

        if (snapshot.result != ReaderResult::Success) {
            const auto invalidation = baseline_.Invalidate();

            result.changed =
                invalidation.kind != BaselineUpdateKind::Unchanged ||
                snapshot.result != previousResult_;

            previousResult_ = snapshot.result;
            return result;
        }

        std::unordered_map<std::string, std::uint8_t> current;
        current.reserve(snapshot.instances.size());

        for (const auto& instance : snapshot.instances) {
            current.emplace(instance.instanceTag, instance.completionFlags);
        }

        const auto update = baseline_.Observe(std::move(current));
        result.changed = update.kind != BaselineUpdateKind::Unchanged;

        if (update.kind == BaselineUpdateKind::Created ||
            update.kind == BaselineUpdateKind::IdentityChanged) {
            for (const auto& instance : snapshot.instances) {
                if (instance.completionFlags != 0) {
                    result.events.push_back(
                        {.category = std::string{kReaderId},
                         .key = instance.instanceTag,
                         .previous = 0,
                         .current = instance.completionFlags});
                }
            }
        }

        for (const auto& change : update.changes) {
            result.events.push_back({.category = std::string{kReaderId},
                                     .key = change.key,
                                     .previous = change.previous,
                                     .current = change.current});
        }

        previousResult_ = ReaderResult::Success;
        return result;
    };

    std::string CaptureStatus(const GameContext& context) const override {
        return SerializeStatus(ReadSnapshot(context));
    }

   private:
    BaselineTracker<std::string, std::uint8_t> baseline_;
    ReaderResult previousResult_{ReaderResult::ReaderUnavailable};
};
}  // namespace

ProgressionReaderPtr CreateActivitiesReader() {
    return std::make_unique<ActivitiesReader>();
}
}  // namespace sr2ap