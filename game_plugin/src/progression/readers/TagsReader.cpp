#include "TagsReader.hpp"

#include <algorithm>
#include <array>
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
#include "game/Memory.hpp"
#include "progression/BaselineTracker.hpp"
#include "progression/ProgressionReader.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
namespace {
constexpr std::string_view kReaderId{"tags"};
constexpr std::uint8_t kCompletedFlag{0x02};

struct TagStatus {
    std::string_view key;
    bool completed{};
};

struct TagDefinition {
    std::uint32_t hash;
    std::string_view key;
};

struct TagSnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::vector<TagStatus> tags;
};

constexpr std::array<TagDefinition, addresses::kExpectedTagCount>
    kTagDefinitions{{
        {0x8E1F27CEu, "tag_01"},  // tagging_spots_spot00
        {0xF9181758u, "tag_02"},  // tagging_spots_spot01
        {0x601146E2u, "tag_03"},  // tagging_spots_spot02
        {0x17167674u, "tag_04"},  // tagging_spots_spot03
        {0x8972E3D7u, "tag_05"},  // tagging_spots_spot04
        {0xFE75D341u, "tag_06"},  // tagging_spots_spot05
        {0x677C82FBu, "tag_07"},  // tagging_spots_spot06
        {0x107BB26Du, "tag_08"},  // tagging_spots_spot07
        {0x80C4AFFCu, "tag_09"},  // tagging_spots_spot08
        {0xF7C39F6Au, "tag_10"},  // tagging_spots_spot09
        {0x9704168Fu, "tag_11"},  // tagging_spots_spot10
        {0xE0032619u, "tag_12"},  // tagging_spots_spot11
        {0x790A77A3u, "tag_13"},  // tagging_spots_spot12
        {0x0E0D4735u, "tag_14"},  // tagging_spots_spot13
        {0x9069D296u, "tag_15"},  // tagging_spots_spot14
        {0xE76EE200u, "tag_16"},  // tagging_spots_spot15
        {0x7E67B3BAu, "tag_17"},  // tagging_spots_spot16
        {0x0960832Cu, "tag_18"},  // tagging_spots_spot17
        {0x99DF9EBDu, "tag_19"},  // tagging_spots_spot18
        {0xEED8AE2Bu, "tag_20"},  // tagging_spots_spot19
        {0xBC29454Cu, "tag_21"},  // tagging_spots_spot20
        {0xCB2E75DAu, "tag_22"},  // tagging_spots_spot21
        {0x52272460u, "tag_23"},  // tagging_spots_spot22
        {0x252014F6u, "tag_24"},  // tagging_spots_spot23
        {0xBB448155u, "tag_25"},  // tagging_spots_spot24
        {0xCC43B1C3u, "tag_26"},  // tagging_spots_spot25
        {0x554AE079u, "tag_27"},  // tagging_spots_spot26
        {0x224DD0EFu, "tag_28"},  // tagging_spots_spot27
        {0xB2F2CD7Eu, "tag_29"},  // tagging_spots_spot28
        {0xC5F5FDE8u, "tag_30"},  // tagging_spots_spot29
        {0xA532740Du, "tag_31"},  // tagging_spots_spot30
        {0xD235449Bu, "tag_32"},  // tagging_spots_spot31
        {0x4B3C1521u, "tag_33"},  // tagging_spots_spot32
        {0x3C3B25B7u, "tag_34"},  // tagging_spots_spot33
        {0xA25FB014u, "tag_35"},  // tagging_spots_spot34
        {0xD5588082u, "tag_36"},  // tagging_spots_spot35
        {0x4C51D138u, "tag_37"},  // tagging_spots_spot36
        {0x3B56E1AEu, "tag_38"},  // tagging_spots_spot37
        {0xABE9FC3Fu, "tag_39"},  // tagging_spots_spot38
        {0xDCEECCA9u, "tag_40"},  // tagging_spots_spot39
        {0xEA73E2CAu, "tag_41"},  // tagging_spots_spot40
        {0x9D74D25Cu, "tag_42"},  // tagging_spots_spot41
        {0x047D83E6u, "tag_43"},  // tagging_spots_spot42
        {0x737AB370u, "tag_44"},  // tagging_spots_spot43
        {0xED1E26D3u, "tag_45"},  // tagging_spots_spot44
        {0x9A191645u, "tag_46"},  // tagging_spots_spot45
        {0x031047FFu, "tag_47"},  // tagging_spots_spot46
        {0x74177769u, "tag_48"},  // tagging_spots_spot47
        {0xE4A86AF8u, "tag_49"},  // tagging_spots_spot48
        {0x93AF5A6Eu, "tag_50"},  // tagging_spots_spot49
    }};

const TagDefinition* FindTag(const std::uint32_t hash) noexcept {
    const auto found = std::ranges::find_if(
        kTagDefinitions,
        [hash](const auto& definition) { return definition.hash == hash; });

    return found == kTagDefinitions.end() ? nullptr : &*found;
}

TagSnapshot ReadTagSnapshot(const GameContext& context) {
    TagSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    if (!context.IsLoaded()) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    const auto head = ReadMemory<std::uintptr_t>(context.module->base +
                                                 addresses::kTagSpotHeadRva);

    if (!head || !*head) {
        snapshot.result = ReaderResult::ManagerUnavailable;
        return snapshot;
    }

    std::unordered_set<std::uint32_t> identities;
    snapshot.tags.reserve(kTagDefinitions.size());

    auto current = *head;

    for (std::size_t visited = 0; visited < kTagDefinitions.size(); ++visited) {
        const auto flags =
            ReadMemory<std::uint8_t>(current + addresses::kTagSpotFlagsOffset);

        const auto metadata = ReadMemory<std::uintptr_t>(
            current + addresses::kTagSpotMetadataOffset);

        const auto next =
            ReadMemory<std::uintptr_t>(current + addresses::kTagSpotNextOffset);

        if (!flags || !metadata || !*metadata || !next || !*next) {
            snapshot.result = ReaderResult::InvalidPointer;
            snapshot.tags.clear();
            return snapshot;
        }

        const auto identity = ReadMemory<std::uint32_t>(
            *metadata + addresses::kTagIdentityHashOffset);

        if (!identity) {
            snapshot.result = ReaderResult::InvalidPointer;
            snapshot.tags.clear();
            return snapshot;
        }

        const auto* definition = FindTag(*identity);

        if (!definition || !identities.emplace(*identity).second) {
            snapshot.result = ReaderResult::InvalidData;
            snapshot.tags.clear();
            return snapshot;
        }

        snapshot.tags.push_back({
            .key = definition->key,
            .completed = (*flags & kCompletedFlag) != 0,
        });

        current = *next;
    }

    if (current != *head || identities.size() != kTagDefinitions.size()) {
        snapshot.result = ReaderResult::InvalidData;
        snapshot.tags.clear();
        return snapshot;
    }

    snapshot.result = ReaderResult::Success;
    return snapshot;
}

std::string SerializeStatus(const TagSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return fmt::format("[{}]\nresult={}\n", kReaderId,
                           ToString(snapshot.result));
    }

    const auto completed =
        std::count_if(snapshot.tags.begin(), snapshot.tags.end(),
                      [](const auto& tag) { return tag.completed; });

    std::string output = fmt::format(
        "[{}]\n"
        "result={}\n"
        "tag_count={}\n"
        "completed_count={}\n",
        kReaderId, ToString(snapshot.result), snapshot.tags.size(), completed);

    for (const auto& tag : snapshot.tags) {
        output += fmt::format("{}={}\n", tag.key, tag.completed ? 1 : 0);
    }

    return output;
}

class TagsReader final : public ProgressionReader {
   public:
    std::string_view Id() const noexcept override {
        return kReaderId;
    }

    ReaderUpdate Poll(const GameContext& context) override {
        const auto snapshot = ReadTagSnapshot(context);

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

        std::unordered_map<std::string_view, bool> current;
        current.reserve(snapshot.tags.size());
        for (const auto& tag : snapshot.tags) {
            current.emplace(tag.key, tag.completed);
        }

        const auto update = baseline_.Observe(std::move(current));
        result.changed = update.kind != BaselineUpdateKind::Unchanged;

        if (update.kind == BaselineUpdateKind::Created ||
            update.kind == BaselineUpdateKind::IdentityChanged) {
            for (const auto& tag : snapshot.tags) {
                if (tag.completed) {
                    result.events.push_back({
                        .category = std::string{kReaderId},
                        .key = std::string{tag.key},
                        .previous = 0,
                        .current = 1,
                    });
                }
            }
        }

        for (const auto& change : update.changes) {
            result.events.push_back({
                .category = std::string{kReaderId},
                .key = std::string{change.key},
                .previous = change.previous ? 1U : 0U,
                .current = change.current ? 1U : 0U,
            });
        }

        previousResult_ = ReaderResult::Success;
        return result;
    }

    std::string CaptureStatus(const GameContext& context) const override {
        return SerializeStatus(ReadTagSnapshot(context));
    }

   private:
    BaselineTracker<std::string_view, bool> baseline_;
    ReaderResult previousResult_{ReaderResult::ReaderUnavailable};
};
}  // namespace

ProgressionReaderPtr CreateTagsReader() {
    return std::make_unique<TagsReader>();
}
}  // namespace sr2ap
