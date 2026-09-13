#include "HitmanReader.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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
constexpr std::string_view kReaderId{"hitman"};

constexpr std::size_t kMaxTargetsPerList = 16;
constexpr std::size_t kLocationCapacity = 32;

struct HitmanTargetStatus {
    std::string locationTag;
    std::uint32_t listId{};
    std::uint32_t targetIndex{};
    bool complete{};
};

struct HitmanSnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::vector<HitmanTargetStatus> targets;
};

bool ValidateReaderCode(const ModuleInfo& game) {
    constexpr std::array<std::uint8_t, 12> expected{
        0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x0C, 0x01, 0x00, 0x00};

    return MatchesBytes(game.base + addresses::kHitmanCategoryHandlerRva,
                        expected);
}

HitmanSnapshot ReadSnapshot(const GameContext& context) {
    HitmanSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    if (!ValidateReaderCode(game)) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    const auto table = game.base + addresses::kHitmanListTableCandidateRva;
    for (std::uint32_t listIndex = 0; listIndex < addresses::kHitmanListCount;
         ++listIndex) {
        const auto descriptor =
            table + listIndex * addresses::kHitmanListDescriptorStride;

        std::optional<std::uint32_t> rowBase =
            ReadMemory<std::uint32_t>(descriptor);
        if (!rowBase || *rowBase == 0) {
            snapshot.result = ReaderResult::GameNotReady;
            snapshot.targets.clear();
            return snapshot;
        }

        if (!IsInsideModule(game.handle,
                            reinterpret_cast<const void*>(*rowBase))) {
            snapshot.result = ReaderResult::InvalidPointer;
            snapshot.targets.clear();
            return snapshot;
        }

        std::optional<std::uint32_t> count = ReadMemory<std::uint32_t>(
            *rowBase + addresses::kHitmanRowCountOffset);
        if (!count || *count == 0 || *count > kMaxTargetsPerList) {
            snapshot.result = ReaderResult::ManagerUnavailable;
            snapshot.targets.clear();
            return snapshot;
        }

        const auto required =
            static_cast<std::size_t>(*count - 1) * addresses::kHitmanRowStride +
            addresses::kHitmanLocationOffset + kLocationCapacity;
        if (!IsReadableAddress(reinterpret_cast<const void*>(*rowBase),
                               required)) {
            snapshot.result = ReaderResult::InvalidPointer;
            snapshot.targets.clear();
            return snapshot;
        }

        for (std::uint32_t targetIndex = 0; targetIndex < *count;
             ++targetIndex) {
            const auto row = static_cast<std::uintptr_t>(*rowBase) +
                             targetIndex * addresses::kHitmanRowStride;
            std::optional<std::uint8_t> completion = ReadMemory<std::uint8_t>(
                row + addresses::kHitmanCompletionOffset);
            auto location =
                ReadValidatedString(row + addresses::kHitmanLocationOffset,
                                    kLocationCapacity, "HITMAN_LOC_");
            if (!completion || *completion > 1 || !location) {
                snapshot.result = ReaderResult::InvalidPointer;
                snapshot.targets.clear();
                return snapshot;
            }

            snapshot.targets.push_back({std::move(*location), listIndex + 1,
                                        targetIndex + 1, *completion != 0});
        }
    }
    snapshot.result = ReaderResult::Success;
    return snapshot;
}

std::string SerializeStatus(const HitmanSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return fmt::format("[{}]\nresult={}\n", kReaderId,
                           ToString(snapshot.result));
    }

    const auto retrieved =
        std::count_if(snapshot.targets.begin(), snapshot.targets.end(),
                      [](const auto& value) { return value.complete; });

    std::string output = fmt::format(
        "[{}]\n"
        "result={}\n"
        "target_count={}\n"
        "completed_count={}\n",
        kReaderId, ToString(snapshot.result), snapshot.targets.size(),
        retrieved);

    for (const auto& target : snapshot.targets) {
        output += fmt::format("{}={}\n", target.locationTag, target.complete);
    }

    return output;
}

class HitmanReader final : public ProgressionReader {
   public:
    std::string_view Id() const noexcept override {
        return kReaderId;
    }

    ReaderUpdate Poll(const GameContext& context) override;

    std::string CaptureStatus(const GameContext& context) const override {
        return SerializeStatus(ReadSnapshot(context));
    }

   private:
    BaselineTracker<std::string, bool> baseline_;
    ReaderResult previousResult_{ReaderResult::ReaderUnavailable};
};

ReaderUpdate HitmanReader::Poll(const GameContext& context) {
    const auto snapshot = ReadSnapshot(context);
    ReaderUpdate result;
    result.statusSection = SerializeStatus(snapshot);

    if (snapshot.result != ReaderResult::Success) {
        const auto invalidation = baseline_.Invalidate();

        result.changed = invalidation.kind != BaselineUpdateKind::Unchanged ||
                         snapshot.result != previousResult_;

        previousResult_ = snapshot.result;
        return result;
    }

    std::unordered_map<std::string, bool> current;
    current.reserve(snapshot.targets.size());

    for (const auto& target : snapshot.targets) {
        current.emplace(target.locationTag, target.complete);
    }

    const auto update = baseline_.Observe(std::move(current));
    result.changed = update.kind != BaselineUpdateKind::Unchanged;

    if (update.kind == BaselineUpdateKind::Created ||
        update.kind == BaselineUpdateKind::IdentityChanged) {
        for (const auto& target : snapshot.targets) {
            if (!target.complete) {
                continue;
            }

            result.events.push_back({
                .category = std::string{kReaderId},
                .key = target.locationTag,
                .previous = 0,
                .current = 1,
            });
        }
    }

    for (const auto& change : update.changes) {
        result.events.push_back({
            .category = std::string{kReaderId},
            .key = change.key,
            .previous = change.previous ? 1U : 0U,
            .current = change.current ? 1U : 0U,
        });
    }

    previousResult_ = ReaderResult::Success;

    return result;
}
}  // namespace

ProgressionReaderPtr CreateHitmanReader() {
    return std::make_unique<HitmanReader>();
}
}  // namespace sr2ap