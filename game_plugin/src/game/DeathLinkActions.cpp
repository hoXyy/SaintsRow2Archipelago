#include "DeathLinkActions.hpp"

#include <windows.h>

#include <array>
#include <cstdint>

#include "Addresses.hpp"
#include "Memory.hpp"

namespace sr2ap::death {
namespace {
constexpr std::array<std::uint8_t, 6> kIsDeadEntry{
    0x8B, 0x54, 0x24, 0x04, 0x85, 0xD2,
};
constexpr std::array<std::uint8_t, 7> kDamageEntry{
    0x81, 0xEC, 0x4C, 0x05, 0x00, 0x00, 0xA1,
};
constexpr std::array<std::uint8_t, 12> kSetHitPointsEntry{
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x2C, 0x05, 0x00, 0x00,
};
constexpr std::array<std::uint8_t, 12> kVehicleDamageEntry{
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x54, 0x07, 0x00, 0x00,
};
constexpr std::array<std::uint8_t, 7> kCurrentActivityTypeEntry{
    0xA1, 0x60, 0x3C, 0x7B, 0x02, 0x85, 0xC0,
};
constexpr std::array<std::uint8_t, 7> kCurrentDiversionTypeEntry{
    0xA1, 0x88, 0x78, 0x52, 0x02, 0x85, 0xC0,
};

// The same arguments Lua vehicle_detonate passes.
constexpr float kDetonationRadius{100.0f};
constexpr std::int32_t kDetonationDamage{0x7FFFFFFF};
constexpr std::int32_t kDetonationDamageType{1};
// Negative damage kills, like Lua's character_kill.
constexpr float kKillDamage{-1.0f};
// A kill finishes inside the damage call; an explosion may hurt the player a
// little later.
constexpr std::uint64_t kKillSuppressionMs{1'000};
constexpr std::uint64_t kDetonationSuppressionMs{5'000};

using IsDeadFunction = bool(__cdecl*)(std::uintptr_t human);
using SetHitPointsFunction = void(__thiscall*)(std::uintptr_t human,
                                               const float* hitPoints);
using VehicleDamageFunction = void(__cdecl*)(
    std::uintptr_t vehicle, std::uintptr_t attacker, std::int32_t, std::int32_t,
    const float* position, const float* direction, float radius,
    std::int32_t damage, std::int32_t damageType, const float* impulse, float,
    std::int32_t, std::int32_t, std::int32_t);
using CurrentTypeFunction = std::int32_t(__cdecl*)();

void CallDamage(const std::uintptr_t function, const std::uintptr_t human,
                const float damage) {
    __asm {
        push 0
        push 0
        push damage
        push human
        xor ecx, ecx
        xor edx, edx
        call function
        fstp st(0)
        add esp, 0x10
    }
}

bool IsDead(const Functions& functions, const std::uintptr_t human) {
    return reinterpret_cast<IsDeadFunction>(functions.isDead)(human);
}

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

Result Detonate(const ModuleInfo& game, const Functions& functions,
                DeathSuppression& suppression, const std::uint32_t handle) {
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
    const auto position = ReadMemoryIntoArray<float, 3>(
        vehicle + addresses::kVehiclePositionOffset);
    if (!position) {
        return {Outcome::Deferred, "could not read the vehicle position"};
    }

    constexpr std::array<float, 3> kNoImpulse{};
    suppression.Begin(GetTickCount64() + kDetonationSuppressionMs);
    reinterpret_cast<VehicleDamageFunction>(functions.vehicleDamage)(
        vehicle, 0, 0, 0, position->data(), nullptr, kDetonationRadius,
        kDetonationDamage, kDetonationDamageType, kNoImpulse.data(), 0.0f, 0, 0,
        0);

    if (!IsWrecked(vehicle)) {
        suppression.End();
        return {Outcome::Deferred, "the vehicle did not take the damage"};
    }
    return {Outcome::Applied, "destroyed the activity vehicle"};
}

Result KillOnFoot(const Functions& functions, DeathSuppression& suppression,
                  const std::uintptr_t player) {
    suppression.Begin(GetTickCount64() + kKillSuppressionMs);
    CallDamage(functions.damage, player, kKillDamage);
    const char* detail = "killed the player";
    if (!IsDead(functions, player)) {
        // Invulnerable, usually a scripted mission section.
        constexpr float kNoHitPoints{0.0f};
        reinterpret_cast<SetHitPointsFunction>(functions.setHitPoints)(
            player, &kNoHitPoints);
        detail = "killed the player through invulnerability";
    }
    // Demolition Derby and races reset the player instead, so the death
    // having run counts too.
    const bool died = suppression.End() || IsDead(functions, player);
    return died ? Result{Outcome::Applied, detail}
                : Result{Outcome::Deferred, "the kill did not land"};
}

// The explosion is for show: it does not always kill the people inside, and
// some cars cannot blow up, so the kill follows unless the explosion did it.
Result DetonateAndKill(const ModuleInfo& game, const Functions& functions,
                       DeathSuppression& suppression,
                       const std::uintptr_t player,
                       const std::uint32_t vehicle) {
    Detonate(game, functions, suppression, vehicle);
    if (IsDead(functions, player)) {
        suppression.End();
        return {Outcome::Applied, "blew up the player's vehicle"};
    }
    return KillOnFoot(functions, suppression, player);
}
}  // namespace

std::optional<Functions> ResolveFunctions(const ModuleInfo& game) {
    const Functions functions{
        .isDead = game.base + addresses::kHumanIsDeadRva,
        .damage = game.base + addresses::kHumanDamageRva,
        .setHitPoints = game.base + addresses::kHumanSetHitPointsRva,
        .vehicleDamage = game.base + addresses::kVehicleDamageRva,
        .currentActivityType = game.base + addresses::kCurrentActivityTypeRva,
        .currentDiversionType = game.base + addresses::kCurrentDiversionTypeRva,
    };
    if (!ValidateHookTarget(game, functions.isDead, kIsDeadEntry) ||
        !ValidateHookTarget(game, functions.damage, kDamageEntry) ||
        !ValidateHookTarget(game, functions.setHitPoints, kSetHitPointsEntry) ||
        !ValidateHookTarget(game, functions.vehicleDamage,
                            kVehicleDamageEntry) ||
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
    if (IsDead(functions, player)) {
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
        return Detonate(game, functions, suppression, handle ? *handle : 0);
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
                return Detonate(game, functions, suppression,
                                handle ? *handle : 0);
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
    return KillOnFoot(functions, suppression, player);
}
}  // namespace sr2ap::death
