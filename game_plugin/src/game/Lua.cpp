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

    if (!IsInsideModule(game.handle, reinterpret_cast<const void*>(address)) ||
        !IsExecutableAddress(address) ||
        DetectDetour(reinterpret_cast<const void*>(address)) !=
            DetourKind::None) {
        return nullptr;
    }

    const auto actual =
        ReadMemoryIntoArray<std::uint8_t, kCashAddBytes.size()>(address);

    if (!actual || *actual != kCashAddBytes) {
        return nullptr;
    }

    return reinterpret_cast<GiveMoneyLuaFunction>(address);
}

// activity unlock stuff
inline constexpr std::ptrdiff_t kMissionUnlockRva = 0x002A7270;

[[nodiscard]] MissionUnlockFunction ResolveMissionUnlockFunction(
    const ModuleInfo& game) noexcept {
    const auto address = game.base + kMissionUnlockRva;

    if (!IsInsideModule(game.handle, reinterpret_cast<const void*>(address)) ||
        !IsExecutableAddress(address) ||
        DetectDetour(reinterpret_cast<const void*>(address)) !=
            DetourKind::None) {
        return nullptr;
    }

    return reinterpret_cast<MissionUnlockFunction>(address);
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

    if (!IsInsideModule(game.handle,
                        reinterpret_cast<const void*>(resolveAddress)) ||
        !IsInsideModule(game.handle,
                        reinterpret_cast<const void*>(addAddress)) ||
        !IsExecutableAddress(resolveAddress) ||
        !IsExecutableAddress(addAddress) ||
        DetectDetour(reinterpret_cast<const void*>(resolveAddress)) !=
            DetourKind::None ||
        DetectDetour(reinterpret_cast<const void*>(addAddress)) !=
            DetourKind::None) {
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
}  // namespace

namespace Lua {
bool IsMissionComplete(const ModuleInfo& game, const char* mission) {
    const auto address = game.base + kMissionCompletedQueryRva;
    if (!IsInsideModule(game.handle, reinterpret_cast<const void*>(address)) ||
        !IsExecutableAddress(address) ||
        DetectDetour(reinterpret_cast<const void*>(address)) !=
            DetourKind::None) {
        return false;
    }

    const auto isMissionComplete =
        reinterpret_cast<IsMissionCompletedLuaFunction>(address);

    return isMissionComplete(mission);
}

bool CanUnlockActivity() {
    const auto game = InspectSupportedGameModule();
    return game && ResolveMissionUnlockFunction(*game) != nullptr;
}

bool UnlockActivity(const char* progressionTag) {
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
}  // namespace Lua
}  // namespace sr2ap