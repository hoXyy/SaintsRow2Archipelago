#include "StyleLevelReader.hpp"

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
#include "util/Logger.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
namespace {
constexpr std::string_view kReaderId{"style_level"};
constexpr std::string_view kPlayerKey{"player"};

struct StyleLevelSnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::uint32_t level{};
    std::uint32_t points{};
};

StyleLevelSnapshot ReadSnapshot(const GameContext& context) {
    StyleLevelSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;
    const auto gameBase = game.base;

    const auto player =
        ReadMemory<std::uint32_t>(gameBase + addresses::kPlayerGlobalRva);
    const auto table =
        ReadMemory<std::uint32_t>(gameBase + addresses::kStyleLevelTableRva);
    const auto count =
        ReadMemory<std::uint32_t>(gameBase + addresses::kStyleLevelCountRva);

    if (!player || !table || !count) {
        snapshot.result = ReaderResult::InvalidPointer;
        return snapshot;
    }

    if (*player == 0 || *table == 0 || *count == 0) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    if (*count != addresses::kExpectedStyleLevelCount) {
        snapshot.result = ReaderResult::InvalidData;
        return snapshot;
    }

    if (!ReadMemoryIntoValue((static_cast<std::uintptr_t>(*player) +
                              addresses::kPlayerStyleLevelOffset),
                             &snapshot.level) ||
        !ReadMemoryIntoValue((static_cast<std::uintptr_t>(*player) +
                              addresses::kPlayerStylePointsOffset),
                             &snapshot.points)) {
        snapshot.result = ReaderResult::InvalidPointer;
        return snapshot;
    }

    if (snapshot.level >= *count) {
        snapshot.result = ReaderResult::InvalidData;
        return snapshot;
    }

    snapshot.result = ReaderResult::Success;
    return snapshot;
};

std::string SerializeStatus(const StyleLevelSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return fmt::format("[{}]\nresult={}\n", kReaderId,
                           ToString(snapshot.result));
    }

    return fmt::format(
        "[{}]\n"
        "result=success\n"
        "level={}\n"
        "points={}\n",
        kReaderId, snapshot.level, snapshot.points);
}

class StyleLevelReader final : public ProgressionReader {
   public:
    std::string_view Id() const noexcept override {
        return kReaderId;
    }

    ReaderUpdate Poll(const GameContext& context) override;

    std::string CaptureStatus(const GameContext& context) const override {
        return SerializeStatus(ReadSnapshot(context));
    };

   private:
    BaselineTracker<std::string, std::uint32_t> baseline_;
    ReaderResult previousResult_{ReaderResult::ReaderUnavailable};
};

ReaderUpdate StyleLevelReader::Poll(const GameContext& context) {
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

    const auto update =
        baseline_.Observe({{std::string{kPlayerKey}, snapshot.level}});

    result.changed = update.kind != BaselineUpdateKind::Unchanged;

    if (update.kind == BaselineUpdateKind::Created ||
        update.kind == BaselineUpdateKind::IdentityChanged) {
        result.events.push_back({.category = std::string{kReaderId},
                                 .key = std::string{kPlayerKey},
                                 .previous = 0,
                                 .current = snapshot.level});
    }

    for (const auto& change : update.changes) {
        result.events.push_back({.category = std::string{kReaderId},
                                 .key = change.key,
                                 .previous = change.previous,
                                 .current = change.current});

        const auto message =
            fmt::format("Level changed: {} -> {} points={}", change.previous,
                        change.current, snapshot.points);

        if (change.current > change.previous) {
            LogInfo("StyleLevel", message);
        } else {
            LogWarning("StyleLevel", message);
        }
    }

    previousResult_ = ReaderResult::Success;
    return result;
}
}  // namespace

ProgressionReaderPtr CreateStyleLevelReader() {
    return std::make_unique<StyleLevelReader>();
}
}  // namespace sr2ap