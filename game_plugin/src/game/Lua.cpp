#include "Lua.hpp"

#include <array>
#include <cstdint>
#include <limits>

#include "Addresses.hpp"
#include "Memory.hpp"
#include "game/ModuleInfo.hpp"

namespace sr2ap {
namespace {
// Lua functions mapping
using IsMissionCompletedLuaFunction = bool(__thiscall*)(const char*);
using GiveMoneyLuaFunction =
    void(__thiscall*)(void* player, const std::int32_t* amountInCents);
using MissionUnlockFunction = void(__fastcall*)(const char* progressionTag);

// money add stuff
inline constexpr std::ptrdiff_t kCashAddRva = 0x0056A430;
constexpr std::array<std::uint8_t, 11> kCashAddBytes{
    0x81, 0xEC, 0x2C, 0x05, 0x00, 0x00, 0xA1, 0x50, 0x0B, 0xE8, 0x00,
};
constexpr std::int32_t kCentsPerDollar{100};

[[nodiscard]] GiveMoneyLuaFunction ResolveCashAdd(
    const ModuleInfo& game) noexcept {
    const auto address = game.base + kCashAddRva;

    return ValidateHookTarget(game, address, kCashAddBytes)
               ? reinterpret_cast<GiveMoneyLuaFunction>(address)
               : nullptr;
}

// activity unlock stuff
inline constexpr std::ptrdiff_t kMissionUnlockRva = 0x002A7270;

[[nodiscard]] MissionUnlockFunction ResolveMissionUnlockFunction(
    const ModuleInfo& game) noexcept {
    const auto address = game.base + kMissionUnlockRva;

    return ValidateFunctionTarget(game, address)
               ? reinterpret_cast<MissionUnlockFunction>(address)
               : nullptr;
}

// weapon add stuff
inline constexpr std::ptrdiff_t kResolveWeaponRva = 0x007716E0;
inline constexpr std::ptrdiff_t kAddWeaponRva = 0x00159890;
inline constexpr std::size_t kPlayerInventoryOffset = 0x10C4;
inline constexpr std::ptrdiff_t kAddAmmoRva = 0x00634140;

using ResolveWeaponFunction = void*(__cdecl*)(const char* inventoryName);

using AddWeaponFunction = void*(__thiscall*)(void* weaponDefinition,
                                             void* inventory,
                                             std::int32_t count,
                                             std::int32_t equipNow,
                                             std::int32_t synchronize);

struct InventoryFunctions {
    ResolveWeaponFunction resolveWeapon;
    AddWeaponFunction addWeapon;
};

[[nodiscard]] std::optional<InventoryFunctions> ResolveInventoryFunctions(
    const ModuleInfo& game) noexcept {
    const auto resolveAddress = game.base + kResolveWeaponRva;
    const auto addAddress = game.base + kAddWeaponRva;

    if (!ValidateFunctionTarget(game, resolveAddress) ||
        !ValidateFunctionTarget(game, addAddress)) {
        return std::nullopt;
    }

    return InventoryFunctions{
        .resolveWeapon =
            reinterpret_cast<ResolveWeaponFunction>(resolveAddress),
        .addWeapon = reinterpret_cast<AddWeaponFunction>(addAddress),
    };
}

void CallAddAmmo(const std::uintptr_t functionAddress, void* player,
                 const std::int32_t weaponCategory, const std::int32_t amount) {
    __asm {
        mov edx, weaponCategory
        push amount
        push player
        call functionAddress
        add esp, 8
    }
}

// character kill / dead stuff
constexpr std::array<std::uint8_t, 7> kHumanDamageEntry{
    0x81, 0xEC, 0x4C, 0x05, 0x00, 0x00, 0xA1,
};
constexpr std::array<std::uint8_t, 6> kHumanIsDeadEntry{
    0x8B, 0x54, 0x24, 0x04, 0x85, 0xD2,
};
// Negative damage removes all hit points.
constexpr float kKillDamage{-1.0f};

using IsDeadFunction = bool(__cdecl*)(std::uintptr_t human);

void CallHumanDamage(const std::uintptr_t functionAddress,
                     const std::uintptr_t human, const float damage) {
    __asm {
        push 0
        push 0
        push damage
        push human
        xor ecx, ecx
        xor edx, edx
        call functionAddress
        fstp st(0)
        add esp, 0x10
    }
}

// vehicle detonate stuff
constexpr std::array<std::uint8_t, 12> kVehicleDamageEntry{
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x54, 0x07, 0x00, 0x00,
};
constexpr float kDetonationRadius{100.0f};
constexpr std::int32_t kDetonationDamage{0x7FFFFFFF};
constexpr std::int32_t kDetonationDamageType{1};

using VehicleDamageFunction = void(__cdecl*)(
    std::uintptr_t vehicle, std::uintptr_t attacker, std::int32_t, std::int32_t,
    const float* position, const float* direction, float radius,
    std::int32_t damage, std::int32_t damageType, const float* impulse, float,
    std::int32_t, std::int32_t, std::int32_t);
}  // namespace

namespace Lua {
bool CanQueryMissions(const ModuleInfo& game) {
    return ValidateFunctionTarget(game, game.base + kMissionCompletedQueryRva);
}

bool IsMissionComplete(const ModuleInfo& game, const char* mission) {
    const auto isMissionComplete =
        reinterpret_cast<IsMissionCompletedLuaFunction>(
            game.base + kMissionCompletedQueryRva);

    return isMissionComplete(mission);
}

bool CanUnlockActivity() {
    const auto game = InspectSupportedGameModule();
    return game && ResolveMissionUnlockFunction(*game) != nullptr;
}

bool UnlockActivity(const char* progressionTag) {
    if (!progressionTag || progressionTag[0] == '\0') {
        return false;
    }

    const auto game = InspectSupportedGameModule();

    if (!game) {
        return false;
    }

    const auto unlockMission = ResolveMissionUnlockFunction(*game);

    if (unlockMission == nullptr) {
        return false;
    }

    unlockMission(progressionTag);
    return true;
};

bool CanGiveMoney() {
    const auto game = InspectSupportedGameModule();
    return game && ResolveCashAdd(*game) != nullptr;
}

bool GiveMoney(const std::int32_t amount) {
    if (amount <= 0 ||
        amount > std::numeric_limits<std::int32_t>::max() / kCentsPerDollar) {
        return false;
    }

    const auto game = InspectSupportedGameModule();
    if (!game) {
        return false;
    }

    const auto cashAdd = ResolveCashAdd(*game);
    if (!cashAdd) {
        return false;
    }

    const auto player =
        ReadMemory<std::uint32_t>(game->base + addresses::kPlayerGlobalRva);

    if (!player || *player == 0) {
        return false;
    }

    const std::int32_t cents = amount * kCentsPerDollar;

    cashAdd(reinterpret_cast<void*>(static_cast<std::uintptr_t>(*player)),
            &cents);

    return true;
}

bool CanAddWeapon() {
    const auto game = InspectSupportedGameModule();
    return game && ResolveInventoryFunctions(*game).has_value();
}

bool AddWeapon(const char* weaponName, std::int32_t count) {
    if (!weaponName || weaponName[0] == '\0' || count <= 0) {
        return false;
    }

    const auto game = InspectSupportedGameModule();
    if (!game) {
        return false;
    }

    const auto inventoryFunctions = ResolveInventoryFunctions(*game);
    if (!inventoryFunctions) {
        return false;
    }

    const auto playerAddress =
        ReadMemory<std::uint32_t>(game->base + addresses::kPlayerGlobalRva);

    if (!playerAddress || *playerAddress == 0) {
        return false;
    }

    auto* const weapon = inventoryFunctions->resolveWeapon(weaponName);
    if (!weapon) {
        return false;
    }

    const auto player = static_cast<std::uintptr_t>(*playerAddress);
    auto* const inventory =
        reinterpret_cast<void*>(player + kPlayerInventoryOffset);

    inventoryFunctions->addWeapon(weapon, inventory, count, 0, 1);

    return true;
}

bool AddWeaponAmmo(const char* weaponName, std::int32_t amount) {
    if (!weaponName || weaponName[0] == '\0' || amount <= 0) {
        return false;
    }

    const auto game = InspectSupportedGameModule();
    if (!game) {
        return false;
    }

    const auto functions = ResolveInventoryFunctions(*game);
    if (!functions) {
        return false;
    }

    const auto playerAddress =
        ReadMemory<std::uint32_t>(game->base + addresses::kPlayerGlobalRva);

    if (!playerAddress || *playerAddress == 0) {
        return false;
    }

    auto* const weapon = functions->resolveWeapon(weaponName);
    if (!weapon) {
        return false;
    }

    constexpr std::size_t kWeaponCategoryOffset{0x3C};
    const auto category = ReadMemory<std::int32_t>(
        reinterpret_cast<std::uintptr_t>(weapon) + kWeaponCategoryOffset);

    if (!category || *category == 9) {
        return false;
    }

    CallAddAmmo(
        game->base + kAddAmmoRva,
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(*playerAddress)),
        *category, amount);

    return true;
}

bool CanKillCharacter(const ModuleInfo& game) {
    return ValidateHookTarget(game, game.base + addresses::kHumanDamageRva,
                              kHumanDamageEntry);
}

void KillCharacter(const ModuleInfo& game, const std::uintptr_t human) {
    CallHumanDamage(game.base + addresses::kHumanDamageRva, human, kKillDamage);
}

bool CanQueryCharacterDead(const ModuleInfo& game) {
    return ValidateHookTarget(game, game.base + addresses::kHumanIsDeadRva,
                              kHumanIsDeadEntry);
}

bool IsCharacterDead(const ModuleInfo& game, const std::uintptr_t human) {
    const auto isDead = reinterpret_cast<IsDeadFunction>(
        game.base + addresses::kHumanIsDeadRva);
    return isDead(human);
}

bool CanDetonateVehicle(const ModuleInfo& game) {
    return ValidateHookTarget(game, game.base + addresses::kVehicleDamageRva,
                              kVehicleDamageEntry);
}

bool DetonateVehicle(const ModuleInfo& game, const std::uintptr_t vehicle) {
    const auto position = ReadMemoryIntoArray<float, 3>(
        vehicle + addresses::kVehiclePositionOffset);
    if (!position) {
        return false;
    }

    // The same arguments Lua vehicle_detonate passes.
    constexpr std::array<float, 3> kNoImpulse{};
    const auto vehicleDamage = reinterpret_cast<VehicleDamageFunction>(
        game.base + addresses::kVehicleDamageRva);
    vehicleDamage(vehicle, 0, 0, 0, position->data(), nullptr,
                  kDetonationRadius, kDetonationDamage, kDetonationDamageType,
                  kNoImpulse.data(), 0.0f, 0, 0, 0);
    return true;
}
}  // namespace Lua
}  // namespace sr2ap