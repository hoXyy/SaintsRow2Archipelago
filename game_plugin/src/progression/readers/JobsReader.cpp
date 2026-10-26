#include "JobsReader.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
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
constexpr std::string_view kReaderId{"jobs"};
// Each job reports as its own category with this single key.
constexpr std::string_view kLevelsKey{"levels"};

struct JobDefinition {
    std::string_view key;
    std::uint32_t statId;
};

struct JobStatus {
    std::string_view key;
    std::uint32_t completedLevels{};
};

struct JobsSnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::vector<JobStatus> jobs;
};

constexpr std::array kJobDefinitions{
    JobDefinition{"paramedic", addresses::kParamedicLevelsStatId},
    JobDefinition{"firefighter", addresses::kFirefighterLevelsStatId},
    JobDefinition{"hoing", addresses::kHoingLevelsStatId},
    JobDefinition{"taxi", addresses::kTaxiLevelsStatId},
    JobDefinition{"towing", addresses::kTowingLevelsStatId},
};

JobsSnapshot ReadSnapshot(const GameContext& context) {
    JobsSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    if (!context.IsLoaded()) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    const auto statCount =
        ReadMemory<std::uint32_t>(game.base + addresses::kTrackedStatCountRva);
    if (!statCount) {
        snapshot.result = ReaderResult::InvalidPointer;
        return snapshot;
    }
    if (*statCount <= addresses::kHoingLevelsStatId) {
        snapshot.result = ReaderResult::ManagerUnavailable;
        return snapshot;
    }

    snapshot.jobs.reserve(kJobDefinitions.size());
    for (const auto& definition : kJobDefinitions) {
        const auto value = ReadMemory<std::uint32_t>(
            game.base + addresses::kTrackedStatTableRva +
            static_cast<std::uintptr_t>(definition.statId) *
                addresses::kTrackedStatStride +
            addresses::kTrackedStatValueOffset);
        if (!value) {
            snapshot.result = ReaderResult::InvalidPointer;
            snapshot.jobs.clear();
            return snapshot;
        }
        if (*value > addresses::kJobLevelCount) {
            snapshot.result = ReaderResult::InvalidData;
            snapshot.jobs.clear();
            return snapshot;
        }
        snapshot.jobs.push_back({definition.key, *value});
    }

    snapshot.result = ReaderResult::Success;
    return snapshot;
}

std::string SerializeStatus(const JobsSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return fmt::format("[{}]\nresult={}\n", kReaderId,
                           ToString(snapshot.result));
    }

    std::string output =
        fmt::format("[{}]\nresult={}\n", kReaderId, ToString(snapshot.result));
    for (const auto& job : snapshot.jobs) {
        output += fmt::format("{}={}\n", job.key, job.completedLevels);
    }
    return output;
}

class JobsReader final : public ProgressionReader {
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

        std::unordered_map<std::string, std::uint32_t> current;
        current.reserve(snapshot.jobs.size());

        for (const auto& job : snapshot.jobs) {
            current.emplace(job.key, job.completedLevels);
        }

        const auto update = baseline_.Observe(std::move(current));
        result.changed = update.kind != BaselineUpdateKind::Unchanged;

        if (update.kind == BaselineUpdateKind::Created ||
            update.kind == BaselineUpdateKind::IdentityChanged) {
            for (const auto& job : snapshot.jobs) {
                if (job.completedLevels != 0) {
                    result.events.push_back({.category = std::string{job.key},
                                             .key = std::string{kLevelsKey},
                                             .previous = 0,
                                             .current = job.completedLevels});
                }
            }
        }

        for (const auto& change : update.changes) {
            result.events.push_back({.category = change.key,
                                     .key = std::string{kLevelsKey},
                                     .previous = change.previous,
                                     .current = change.current});
        }

        previousResult_ = ReaderResult::Success;
        return result;
    };

    std::string CaptureStatus(const GameContext& context) const override {
        return SerializeStatus(ReadSnapshot(context));
    }

    void Reset() override {
        baseline_.Invalidate();
    }

   private:
    BaselineTracker<std::string, std::uint32_t> baseline_;
    ReaderResult previousResult_{ReaderResult::ReaderUnavailable};
};
}  // namespace

ProgressionReaderPtr CreateJobsReader() {
    return std::make_unique<JobsReader>();
}
}  // namespace sr2ap
