#include "DeathLinkActions.hpp"

#include <windows.h>

#include <array>
#include <cstdint>

#include "Addresses.hpp"
#include "Lua.hpp"
#include "Memory.hpp"

namespace sr2ap::death {
namespace {
constexpr std::array<std::uint8_t, 12> kSetHitPointsEntry{
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x2C, 0x05, 0x00, 0x00,
};
constexpr std::array<std::uint8_t, 7> kCurrentActivityTypeEntry{
    0xA1, 0x60, 0x3C, 0x7B, 0x02, 0x85, 0xC0,
};
constexpr std::array<std::uint8_t, 7> kCurrentDiversionTypeEntry{
    0xA1, 0x88, 0x78, 0x52, 0x02, 0x85, 0xC0,
};

// A kill finishes inside the damage call; an explosion may hurt the player a
// little later.
constexpr std::uint64_t kKillSuppressionMs{1'000};
constexpr std::uint64_t kDetonationSuppressionMs{5'000};

using SetHitPointsFunction = void(__thiscall*)(std::uintptr_t human,
                                               const float* hitPoints);
using CurrentTypeFunction = std::int32_t(__cdecl*)();

std::uintptr_t ReadPlayer(const ModuleInfo& game) {
    const auto player =
        ReadMemory<std::uint32_t>(game.base + addresses::kPlayerGlobalRva);
    return player ? *player : 0;
}

std::uintptr_t ResolveVehicle(const ModuleInfo& game,
                              const std::uint32_t handle) {
    if (handle == 0) {
        return 0;
    }
    const auto entry = game.base + addresses::kObjectHandleTableRva +
                       (handle & addresses::kObjectHandleIndexMask) *
                           addresses::kObjectHandleEntryStride;
    const auto object = ReadMemory<std::uint32_t>(entry);
    if (!object || *object == 0) {
        return 0;
    }
    const auto objectHandle =
        ReadMemory<std::uint32_t>(*object + addresses::kObjectHandleOffset);
    const auto type =
        ReadMemory<std::uint32_t>(*object + addresses::kObjectTypeOffset);
    if (!objectHandle || *objectHandle != handle || !type ||
        *type != addresses::kObjectTypeVehicle) {
        return 0;
    }
    return *object;
}

bool IsWrecked(const std::uintptr_t vehicle) {
    const auto flags = ReadMemory<std::uint32_t>(
        vehicle + addresses::kVehicleStateFlagsOffset);
    const auto hitPoints =
        ReadMemory<std::int32_t>(vehicle + addresses::kVehicleHitPointsOffset);
    return !flags || !hitPoints ||
           (*flags & addresses::kVehicleWreckedFlag) != 0 || *hitPoints <= 0;
}

Result Detonate(const ModuleInfo& game, DeathSuppression& suppression,
                const std::uint32_t handle) {
    const auto vehicle = ResolveVehicle(game, handle);
    if (vehicle == 0) {
        return {Outcome::Dropped, "activity vehicle is gone"};
    }
    const auto active =
        ReadMemory<std::uint8_t>(vehicle + addresses::kVehicleActiveOffset);
    if (!active || *active != 1) {
        return {Outcome::Deferred, "activity vehicle is not loaded"};
    }
    if (IsWrecked(vehicle)) {
        return {Outcome::Dropped, "activity vehicle is already wrecked"};
    }

    suppression.Begin(GetTickCount64() + kDetonationSuppressionMs);
    if (!Lua::DetonateVehicle(game, vehicle)) {
        suppression.End();
        return {Outcome::Deferred, "could not read the vehicle position"};
    }
    if (!IsWrecked(vehicle)) {
        suppression.End();
        return {Outcome::Deferred, "the vehicle did not take the damage"};
    }
    return {Outcome::Applied, "destroyed the activity vehicle"};
}

Result KillOnFoot(const ModuleInfo& game, const Functions& functions,
                  DeathSuppression& suppression, const std::uintptr_t player) {
    suppression.Begin(GetTickCount64() + kKillSuppressionMs);
    Lua::KillCharacter(game, player);
    const char* detail = "killed the player";
    if (!Lua::IsCharacterDead(game, player)) {
        // Invulnerable, usually a scripted mission section.
        constexpr float kNoHitPoints{0.0f};
        reinterpret_cast<SetHitPointsFunction>(functions.setHitPoints)(
            player, &kNoHitPoints);
        detail = "killed the player through invulnerability";
    }
    // Demolition Derby and races reset the player instead, so the death
    // having run counts too.
    const bool died = suppression.End() || Lua::IsCharacterDead(game, player);
    return died ? Result{Outcome::Applied, detail}
                : Result{Outcome::Deferred, "the kill did not land"};
}

// The explosion is for show: it does not always kill the people inside, and
// some cars cannot blow up, so the kill follows unless the explosion did it.
Result DetonateAndKill(const ModuleInfo& game, const Functions& functions,
                       DeathSuppression& suppression,
                       const std::uintptr_t player,
                       const std::uint32_t vehicle) {
    Detonate(game, suppression, vehicle);
    if (Lua::IsCharacterDead(game, player)) {
        suppression.End();
        return {Outcome::Applied, "blew up the player's vehicle"};
    }
    return KillOnFoot(game, functions, suppression, player);
}
}  // namespace

std::optional<Functions> ResolveFunctions(const ModuleInfo& game) {
    const Functions functions{
        .setHitPoints = game.base + addresses::kHumanSetHitPointsRva,
        .currentActivityType = game.base + addresses::kCurrentActivityTypeRva,
        .currentDiversionType = game.base + addresses::kCurrentDiversionTypeRva,
    };
    if (!Lua::CanKillCharacter(game) || !Lua::CanQueryCharacterDead(game) ||
        !Lua::CanDetonateVehicle(game) ||
        !ValidateHookTarget(game, functions.setHitPoints, kSetHitPointsEntry) ||
        !ValidateHookTarget(game, functions.currentActivityType,
                            kCurrentActivityTypeEntry) ||
        !ValidateHookTarget(game, functions.currentDiversionType,
                            kCurrentDiversionTypeEntry)) {
        return std::nullopt;
    }
    return functions;
}

std::optional<PlayerVitals> ReadPlayerVitals(const ModuleInfo& game) {
    const auto player = ReadPlayer(game);
    if (player == 0) {
        return std::nullopt;
    }
    const auto hitPoints =
        ReadMemory<float>(player + addresses::kHumanHitPointsOffset);
    const auto bleedOutTimer =
        ReadMemory<std::int32_t>(player + addresses::kHumanBleedOutTimerOffset);
    if (!hitPoints || !bleedOutTimer) {
        return std::nullopt;
    }
    return PlayerVitals{
        .player = player,
        .hitPoints = *hitPoints,
        .bleedOutTimer = *bleedOutTimer,
    };
}

Result KillPlayer(const ModuleInfo& game, const Functions& functions,
                  DeathSuppression& suppression) {
    const auto player = ReadPlayer(game);
    if (player == 0) {
        return {Outcome::Deferred, "no player"};
    }
    if (Lua::IsCharacterDead(game, player)) {
        return {Outcome::Dropped, "the player is already dead"};
    }

    const auto activity =
        reinterpret_cast<CurrentTypeFunction>(functions.currentActivityType)();
    const auto diversion =
        reinterpret_cast<CurrentTypeFunction>(functions.currentDiversionType)();

    if (activity == addresses::kActivityTypeDemolitionDerby) {
        const auto state =
            ReadMemory<std::uint32_t>(game.base + addresses::kDerbyStateRva);
        if (!state || *state != addresses::kDerbyStateMatch) {
            return {Outcome::Deferred, "derby match has not started"};
        }
        const auto handle = ReadMemory<std::uint32_t>(
            game.base + addresses::kDerbyPlayerVehicleRva);
        return Detonate(game, suppression, handle ? *handle : 0);
    }

    if (activity == addresses::kActivityTypeRacing ||
        diversion == addresses::kDiversionTypeRacing) {
        const auto manager =
            ReadMemory<std::uint32_t>(game.base + addresses::kRaceManagerRva);
        if (manager && *manager != 0) {
            const auto state = ReadMemory<std::uint32_t>(
                *manager + addresses::kRaceManagerStateOffset);
            if (!state || *state < addresses::kRaceStateCountdown) {
                return {Outcome::Deferred, "race car is not assigned yet"};
            }
            if (*state <= addresses::kRaceStateRacing) {
                const auto handle = ReadMemory<std::uint32_t>(
                    *manager + addresses::kRaceManagerPlayerVehicleOffset);
                return Detonate(game, suppression, handle ? *handle : 0);
            }
        } else if (activity == addresses::kActivityTypeRacing) {
            return {Outcome::Deferred, "race has not started"};
        }
    }

    const auto vehicle = ReadMemory<std::uint32_t>(
        player + addresses::kHumanVehicleHandleOffset);
    if (vehicle && *vehicle != 0) {
        return DetonateAndKill(game, functions, suppression, player, *vehicle);
    }
    return KillOnFoot(game, functions, suppression, player);
}
}  // namespace sr2ap::death
