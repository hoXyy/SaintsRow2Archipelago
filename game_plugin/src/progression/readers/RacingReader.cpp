#include "RacingReader.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
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
constexpr std::string_view kReaderId{"racing"};

enum class RacingMedal : std::uint32_t {
    Unattempted = 0,
    Gold = 1,
    Silver = 2,
    Bronze = 3,
    NoMedal = 4
};

struct RaceDefinition {
    std::string_view name;
    std::uint32_t identityHash{};
    std::uint32_t raceClass{};
};

constexpr std::array<RaceDefinition, 27> kRaceDefinitions{{
    {"bike_air", 0x6F4ED749, 1},  {"bike_ht", 0x4C647F34, 1},
    {"bike_mu", 0x4614BBE7, 1},   {"bike_tp", 0xAD7EE670, 1},
    {"bike_un", 0x4E6AEA52, 1},   {"boat_ht", 0x3828B472, 4},
    {"boat_pr", 0x5350891E, 4},   {"car_air1", 0x07476F5E, 0},
    {"car_air2", 0x9E4E3EE4, 0},  {"car_dt", 0xDA1D3915, 0},
    {"car_ht", 0x76A87619, 0},    {"car_mu", 0x7CD8B2CA, 0},
    {"car_nu", 0x57F5E109, 0},    {"car_pj", 0x0EBCD323, 0},
    {"car_sr", 0x36FD18B6, 0},    {"car_sx", 0xD628F1A8, 0},
    {"car_tp", 0x97B2EF5D, 0},    {"heli_dt", 0xCB95CCA5, 3},
    {"heli_mu", 0x6D50477A, 3},   {"heli_sr", 0x2775ED06, 3},
    {"jetski_cv", 0xFC4D5587, 4}, {"jetski_fa", 0x02E92405, 4},
    {"jetski_nu", 0xD0EA7A70, 4}, {"jetski_pr", 0x9ACFD00C, 4},
    {"jetski_sr", 0xB1E283CF, 4}, {"plane_air", 0xE945D97A, 2},
    {"plane_un", 0x96B60D82, 2},
}};

struct RaceStatus {
    std::string_view name;
    std::uint32_t raceNumber{};
    std::uint32_t identityHash{};
    RacingMedal medal{RacingMedal::Unattempted};
    std::uint32_t raceClass{};
};

struct RacingSnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::vector<RaceStatus> races;
};

[[nodiscard]] constexpr const char* ToString(const RacingMedal medal) noexcept {
    switch (medal) {
        case RacingMedal::Unattempted:
            return "unattempted";
        case RacingMedal::Gold:
            return "gold";
        case RacingMedal::Silver:
            return "silver";
        case RacingMedal::Bronze:
            return "bronze";
        case RacingMedal::NoMedal:
            return "no_medal";
    }
    return "invalid";
}

[[nodiscard]] constexpr std::uint32_t RacingMedalRank(
    const RacingMedal medal) noexcept {
    switch (medal) {
        case RacingMedal::Gold:
            return 3;
        case RacingMedal::Silver:
            return 2;
        case RacingMedal::Bronze:
            return 1;
        case RacingMedal::Unattempted:
        case RacingMedal::NoMedal:
            return 0;
    }
    return 0;
}

RacingSnapshot ReadSnapshot(const GameContext& context) {
    RacingSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    if (!context.IsLoaded()) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    std::optional<std::uint32_t> count =
        ReadMemory<std::uint32_t>(game.base + addresses::kRacingRecordCountRva);
    if (!count || *count != kRaceDefinitions.size()) {
        snapshot.result = ReaderResult::ManagerUnavailable;
        return snapshot;
    }

    snapshot.races.reserve(kRaceDefinitions.size());
    for (std::uint32_t index = 0; index < *count; ++index) {
        const auto record =
            game.base + addresses::kRacingRecordTableRva +
            static_cast<std::uintptr_t>(index) * addresses::kRacingRecordStride;

        const auto identityHash = ReadMemory<std::uint32_t>(
            record + addresses::kRacingRecordIdentityHashOffset);
        const auto rawMedal = ReadMemory<std::uint32_t>(
            record + addresses::kRacingRecordMedalOffset);
        const auto raceClass = ReadMemory<std::uint32_t>(
            record + addresses::kRacingRecordClassOffset);

        if (!identityHash || !rawMedal || !raceClass) {
            snapshot.result = ReaderResult::InvalidPointer;
            snapshot.races.clear();
            return snapshot;
        }

        const auto& definition = kRaceDefinitions[index];
        if (*identityHash != definition.identityHash ||
            *raceClass != definition.raceClass ||
            *rawMedal > static_cast<std::uint32_t>(RacingMedal::NoMedal)) {
            snapshot.result = ReaderResult::InvalidData;
            snapshot.races.clear();
            return snapshot;
        }

        snapshot.races.push_back({definition.name, index, *identityHash,
                                  static_cast<RacingMedal>(*rawMedal),
                                  *raceClass});
    }
    snapshot.result = ReaderResult::Success;
    return snapshot;
}

std::string SerializeStatus(const RacingSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return fmt::format("[{}]\nresult={}\n", kReaderId,
                           ToString(snapshot.result));
    }

    const auto medalCount = std::count_if(
        snapshot.races.begin(), snapshot.races.end(), [](const auto& race) {
            return race.medal == RacingMedal::Gold ||
                   race.medal == RacingMedal::Silver ||
                   race.medal == RacingMedal::Bronze;
        });

    std::string output = fmt::format(
        "[{}]\n"
        "result={}\n"
        "races={}\n"
        "medal_count={}\n",
        kReaderId, ToString(snapshot.result), snapshot.races.size(),
        medalCount);

    for (const auto& race : snapshot.races) {
        output += fmt::format(
            "{}={}\n"
            "{}_medal={}\n",
            race.name, RacingMedalRank(race.medal), race.name,
            ToString(race.medal));
    }

    return output;
}

class RacingReader final : public ProgressionReader {
   public:
    std::string_view Id() const noexcept override {
        return kReaderId;
    }

    ReaderUpdate Poll(const GameContext& context) override;

    std::string CaptureStatus(const GameContext& context) const override {
        return SerializeStatus(ReadSnapshot(context));
    }

   private:
    BaselineTracker<std::string_view, RacingMedal> baseline_;
    ReaderResult previousResult_{ReaderResult::ReaderUnavailable};
};

ReaderUpdate RacingReader::Poll(const GameContext& context) {
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

    std::unordered_map<std::string_view, RacingMedal> current;
    current.reserve(snapshot.races.size());

    for (const auto& race : snapshot.races) {
        current.emplace(race.name, race.medal);
    }

    const auto update = baseline_.Observe(std::move(current));
    result.changed = update.kind != BaselineUpdateKind::Unchanged;

    if (update.kind == BaselineUpdateKind::Created ||
        update.kind == BaselineUpdateKind::IdentityChanged) {
        for (const auto& race : snapshot.races) {
            if (const auto rank = RacingMedalRank(race.medal); rank != 0) {
                result.events.push_back(
                    {std::string{kReaderId}, std::string{race.name}, 0, rank});
            }
        }
    }

    for (const auto& change : update.changes) {
        result.events.push_back({std::string{kReaderId},
                                 std::string{change.key},
                                 RacingMedalRank(change.previous),
                                 RacingMedalRank(change.current)});
    }

    previousResult_ = ReaderResult::Success;
    return result;
}
}  // namespace

ProgressionReaderPtr CreateRacingReader() {
    return std::make_unique<RacingReader>();
}
}  // namespace sr2ap