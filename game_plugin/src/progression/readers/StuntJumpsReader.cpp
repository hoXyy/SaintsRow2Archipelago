#include "StuntJumpsReader.hpp"

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
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "progression/BaselineTracker.hpp"
#include "progression/ProgressionReader.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
namespace {
constexpr std::string_view kReaderId{"stunt_jumps"};
// Upper bound on the active trigger list walk, as in MapReveal.
constexpr std::size_t kMaximumTriggers = 16384;

struct StuntJumpStatus {
    std::string_view key;
    bool completed{};
};

struct StuntJumpDefinition {
    std::uint32_t hash;
    std::string_view key;
};

struct StuntJumpSnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::uint32_t gameCompletedCount{};
    std::vector<StuntJumpStatus> jumps;
};

constexpr std::array<StuntJumpDefinition, addresses::kExpectedStuntJumpCount>
    kStuntJumpDefinitions{{
        {0xF4915DDFu, "jump_01"},  // stunt_jumps_$t000
        {0x83966D49u, "jump_02"},  // stunt_jumps_$t001
        {0x1A9F3CF3u, "jump_03"},  // stunt_jumps_$t002
        {0x6D980C65u, "jump_04"},  // stunt_jumps_$t003
        {0xF3FC99C6u, "jump_05"},  // stunt_jumps_$t004
        {0x84FBA950u, "jump_06"},  // stunt_jumps_$t005
        {0x1DF2F8EAu, "jump_07"},  // stunt_jumps_$t006
        {0x6AF5C87Cu, "jump_08"},  // stunt_jumps_$t007
        {0xFA4AD5EDu, "jump_09"},  // stunt_jumps_$t008
        {0x8D4DE57Bu, "jump_10"},  // stunt_jumps_$t009
        {0xED8A6C9Eu, "jump_11"},  // stunt_jumps_$t010
        {0x9A8D5C08u, "jump_12"},  // stunt_jumps_$t011
        {0x03840DB2u, "jump_13"},  // stunt_jumps_$t012
        {0x74833D24u, "jump_14"},  // stunt_jumps_$t013
        {0xEAE7A887u, "jump_15"},  // stunt_jumps_$t014
        {0x9DE09811u, "jump_16"},  // stunt_jumps_$t015
        {0x04E9C9ABu, "jump_17"},  // stunt_jumps_$t016
        {0x73EEF93Du, "jump_18"},  // stunt_jumps_$t017
        {0xE351E4ACu, "jump_19"},  // stunt_jumps_$t018
        {0x9456D43Au, "jump_20"},  // stunt_jumps_$t019
        {0xC6A73F5Du, "jump_21"},  // stunt_jumps_$t020
        {0xB1A00FCBu, "jump_22"},  // stunt_jumps_$t021
        {0x28A95E71u, "jump_23"},  // stunt_jumps_$t022
        {0x5FAE6EE7u, "jump_24"},  // stunt_jumps_$t023
        {0xC1CAFB44u, "jump_25"},  // stunt_jumps_$t024
        {0xB6CDCBD2u, "jump_26"},  // stunt_jumps_$t025
        {0x2FC49A68u, "jump_27"},  // stunt_jumps_$t026
        {0x58C3AAFEu, "jump_28"},  // stunt_jumps_$t027
        {0xC87CB76Fu, "jump_29"},  // stunt_jumps_$t028
        {0xBF7B87F9u, "jump_30"},  // stunt_jumps_$t029
        {0xDFBC0E1Cu, "jump_31"},  // stunt_jumps_$t030
        {0xA8BB3E8Au, "jump_32"},  // stunt_jumps_$t031
        {0x31B26F30u, "jump_33"},  // stunt_jumps_$t032
        {0x46B55FA6u, "jump_34"},  // stunt_jumps_$t033
        {0xD8D1CA05u, "jump_35"},  // stunt_jumps_$t034
        {0xAFD6FA93u, "jump_36"},  // stunt_jumps_$t035
        {0x36DFAB29u, "jump_37"},  // stunt_jumps_$t036
        {0x41D89BBFu, "jump_38"},  // stunt_jumps_$t037
        {0xD167862Eu, "jump_39"},  // stunt_jumps_$t038
        {0xA660B6B8u, "jump_40"},  // stunt_jumps_$t039
        {0x90FD98DBu, "jump_41"},  // stunt_jumps_$t040
        {0xE7FAA84Du, "jump_42"},  // stunt_jumps_$t041
        {0x7EF3F9F7u, "jump_43"},  // stunt_jumps_$t042
        {0x09F4C961u, "jump_44"},  // stunt_jumps_$t043
        {0x97905CC2u, "jump_45"},  // stunt_jumps_$t044
        {0xE0976C54u, "jump_46"},  // stunt_jumps_$t045
        {0x799E3DEEu, "jump_47"},  // stunt_jumps_$t046
        {0x0E990D78u, "jump_48"},  // stunt_jumps_$t047
        {0x9E2610E9u, "jump_49"},  // stunt_jumps_$t048
        {0xE921207Fu, "jump_50"},  // stunt_jumps_$t049
        {0x89E6A99Au, "jump_51"},  // stunt_jumps_$t050
        {0xFEE1990Cu, "jump_52"},  // stunt_jumps_$t051
        {0x67E8C8B6u, "jump_53"},  // stunt_jumps_$t052
        {0x10EFF820u, "jump_54"},  // stunt_jumps_$t053
        {0x8E8B6D83u, "jump_55"},  // stunt_jumps_$t054
        {0xF98C5D15u, "jump_56"},  // stunt_jumps_$t055
        {0x60850CAFu, "jump_57"},  // stunt_jumps_$t056
        {0x17823C39u, "jump_58"},  // stunt_jumps_$t057
        {0x873D21A8u, "jump_59"},  // stunt_jumps_$t058
        {0xF03A113Eu, "jump_60"},  // stunt_jumps_$t059
        {0xA2CBFA59u, "jump_61"},  // stunt_jumps_$t060
        {0xD5CCCACFu, "jump_62"},  // stunt_jumps_$t061
        {0x4CC59B75u, "jump_63"},  // stunt_jumps_$t062
        {0x3BC2ABE3u, "jump_64"},  // stunt_jumps_$t063
        {0xA5A63E40u, "jump_65"},  // stunt_jumps_$t064
        {0xD2A10ED6u, "jump_66"},  // stunt_jumps_$t065
        {0x4BA85F6Cu, "jump_67"},  // stunt_jumps_$t066
        {0x3CAF6FFAu, "jump_68"},  // stunt_jumps_$t067
        {0xAC10726Bu, "jump_69"},  // stunt_jumps_$t068
        {0xDB1742FDu, "jump_70"},  // stunt_jumps_$t069
        {0xBBD0CB18u, "jump_71"},  // stunt_jumps_$t070
        {0xCCD7FB8Eu, "jump_72"},  // stunt_jumps_$t071
        {0x55DEAA34u, "jump_73"},  // stunt_jumps_$t072
        {0x22D99AA2u, "jump_74"},  // stunt_jumps_$t073
        {0xBCBD0F01u, "jump_75"},  // stunt_jumps_$t074
        {0xCBBA3F97u, "jump_76"},  // stunt_jumps_$t075
        {0x52B36E2Du, "jump_77"},  // stunt_jumps_$t076
        {0x25B45EBBu, "jump_78"},  // stunt_jumps_$t077
        {0xB50B432Au, "jump_79"},  // stunt_jumps_$t078
        {0xC20C73BCu, "jump_80"},  // stunt_jumps_$t079
    }};

const StuntJumpDefinition* FindStuntJump(const std::uint32_t hash) noexcept {
    const auto found = std::ranges::find_if(
        kStuntJumpDefinitions,
        [hash](const auto& definition) { return definition.hash == hash; });

    return found == kStuntJumpDefinitions.end() ? nullptr : &*found;
}

StuntJumpSnapshot ReadStuntJumpSnapshot(const GameContext& context) {
    StuntJumpSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    if (!context.IsLoaded()) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    const auto player = ReadMemory<std::uintptr_t>(
        game.base + addresses::kCollectibleManagerRva);
    const auto head = ReadMemory<std::uintptr_t>(
        game.base + addresses::kActiveTriggerHeadRva);

    if (!player || !*player || !head || !*head) {
        snapshot.result = ReaderResult::ManagerUnavailable;
        return snapshot;
    }

    const auto completedCount = ReadMemory<std::uint32_t>(
        *player + addresses::kStuntJumpsCompletedOffset);
    const auto totalCount =
        ReadMemory<std::uint32_t>(*player + addresses::kStuntJumpsTotalOffset);

    if (!completedCount || !totalCount) {
        snapshot.result = ReaderResult::InvalidPointer;
        return snapshot;
    }

    // Set when the exploration tables load the jump collection.
    if (*totalCount == 0) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    if (*totalCount != addresses::kExpectedStuntJumpCount ||
        *completedCount > *totalCount) {
        snapshot.result = ReaderResult::InvalidData;
        return snapshot;
    }

    snapshot.gameCompletedCount = *completedCount;

    std::unordered_set<std::uint32_t> identities;
    snapshot.jumps.reserve(kStuntJumpDefinitions.size());

    auto current = *head;

    for (std::size_t visited = 0; visited < kMaximumTriggers; ++visited) {
        const auto type =
            ReadMemory<std::uint32_t>(current + addresses::kTriggerTypeOffset);

        const auto next =
            ReadMemory<std::uintptr_t>(current + addresses::kTriggerNextOffset);

        if (!type || !next || !*next) {
            snapshot.result = ReaderResult::InvalidPointer;
            snapshot.jumps.clear();
            return snapshot;
        }

        if (*type == addresses::kTriggerTypeUniqueJump) {
            const auto identity = ReadMemory<std::uint32_t>(
                current + addresses::kTriggerIdentityHashOffset);

            const auto flags = ReadMemory<std::uint8_t>(
                current + addresses::kTriggerFlagsOffset);

            if (!identity || !flags) {
                snapshot.result = ReaderResult::InvalidPointer;
                snapshot.jumps.clear();
                return snapshot;
            }

            const auto* definition = FindStuntJump(*identity);

            if (!definition || !identities.emplace(*identity).second) {
                snapshot.result = ReaderResult::InvalidData;
                snapshot.jumps.clear();
                return snapshot;
            }

            snapshot.jumps.push_back({
                .key = definition->key,
                .completed = (*flags & addresses::kTriggerDiscoveredFlag) != 0,
            });
        }

        current = *next;

        if (current == *head) {
            break;
        }
    }

    if (current != *head || identities.size() != kStuntJumpDefinitions.size()) {
        snapshot.result = ReaderResult::InvalidData;
        snapshot.jumps.clear();
        return snapshot;
    }

    snapshot.result = ReaderResult::Success;
    return snapshot;
}

std::string SerializeStatus(const StuntJumpSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return fmt::format("[{}]\nresult={}\n", kReaderId,
                           ToString(snapshot.result));
    }

    const auto completed =
        std::count_if(snapshot.jumps.begin(), snapshot.jumps.end(),
                      [](const auto& jump) { return jump.completed; });

    std::string output = fmt::format(
        "[{}]\n"
        "result={}\n"
        "jump_count={}\n"
        "completed_count={}\n"
        "game_completed_count={}\n",
        kReaderId, ToString(snapshot.result), snapshot.jumps.size(), completed,
        snapshot.gameCompletedCount);

    for (const auto& jump : snapshot.jumps) {
        output += fmt::format("{}={}\n", jump.key, jump.completed ? 1 : 0);
    }

    return output;
}

class StuntJumpsReader final : public ProgressionReader {
   public:
    std::string_view Id() const noexcept override {
        return kReaderId;
    }

    ReaderUpdate Poll(const GameContext& context) override {
        const auto snapshot = ReadStuntJumpSnapshot(context);

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
        current.reserve(snapshot.jumps.size());
        for (const auto& jump : snapshot.jumps) {
            current.emplace(jump.key, jump.completed);
        }

        const auto update = baseline_.Observe(std::move(current));
        result.changed = update.kind != BaselineUpdateKind::Unchanged;

        if (update.kind == BaselineUpdateKind::Created ||
            update.kind == BaselineUpdateKind::IdentityChanged) {
            for (const auto& jump : snapshot.jumps) {
                if (jump.completed) {
                    result.events.push_back({
                        .category = std::string{kReaderId},
                        .key = std::string{jump.key},
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
        return SerializeStatus(ReadStuntJumpSnapshot(context));
    }

    void Reset() override {
        baseline_.Invalidate();
    }

   private:
    BaselineTracker<std::string_view, bool> baseline_;
    ReaderResult previousResult_{ReaderResult::ReaderUnavailable};
};
}  // namespace

ProgressionReaderPtr CreateStuntJumpsReader() {
    return std::make_unique<StuntJumpsReader>();
}
}  // namespace sr2ap
