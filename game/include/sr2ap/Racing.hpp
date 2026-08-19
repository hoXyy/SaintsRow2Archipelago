#pragma once

#include "sr2ap/ReaderResult.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sr2ap {
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

    inline constexpr std::array<RaceDefinition, 27> kRaceDefinitions{{
        {"bike_air", 0x6F4ED749, 1},  {"bike_ht", 0x4C647F34, 1},   {"bike_mu", 0x4614BBE7, 1},
        {"bike_tp", 0xAD7EE670, 1},   {"bike_un", 0x4E6AEA52, 1},   {"boat_ht", 0x3828B472, 4},
        {"boat_pr", 0x5350891E, 4},   {"car_air1", 0x07476F5E, 0},  {"car_air2", 0x9E4E3EE4, 0},
        {"car_dt", 0xDA1D3915, 0},    {"car_ht", 0x76A87619, 0},    {"car_mu", 0x7CD8B2CA, 0},
        {"car_nu", 0x57F5E109, 0},    {"car_pj", 0x0EBCD323, 0},    {"car_sr", 0x36FD18B6, 0},
        {"car_sx", 0xD628F1A8, 0},    {"car_tp", 0x97B2EF5D, 0},    {"heli_dt", 0xCB95CCA5, 3},
        {"heli_mu", 0x6D50477A, 3},   {"heli_sr", 0x2775ED06, 3},   {"jetski_cv", 0xFC4D5587, 4},
        {"jetski_fa", 0x02E92405, 4}, {"jetski_nu", 0xD0EA7A70, 4}, {"jetski_pr", 0x9ACFD00C, 4},
        {"jetski_sr", 0xB1E283CF, 4}, {"plane_air", 0xE945D97A, 2}, {"plane_un", 0x96B60D82, 2},
    }};

    struct RaceStatus {
        std::string_view name;
        std::uint32_t raceNumber{};
        std::uint32_t identityHash{};
        RacingMedal medal{RacingMedal::Unattempted};
        float bestTime{};
        std::uint32_t raceClass{};
    };

    struct RacingSnapshot {
        ReaderResult result{ReaderResult::ReaderUnavailable};
        std::vector<RaceStatus> races;
    };

    [[nodiscard]] inline constexpr const char* ToString(const RacingMedal medal) noexcept {
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

    [[nodiscard]] inline constexpr std::uint32_t RacingMedalRank(const RacingMedal medal) noexcept {
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

    RacingSnapshot GetRacingSnapshot();
    void LogRacingSnapshot(const RacingSnapshot& snapshot, bool full);
}  // namespace sr2ap
