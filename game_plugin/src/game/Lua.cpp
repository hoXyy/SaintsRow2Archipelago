#include "Lua.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

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

// consumable stuff
inline constexpr std::ptrdiff_t kGiveConsumableRva = 0x005882A0;
constexpr std::array<std::uint8_t, 9> kGiveConsumableBytes{
    0x83, 0xEC, 0x08, 0x56, 0x57, 0x8D, 0x44, 0x24, 0x0C,
};
inline constexpr std::ptrdiff_t kRemoveConsumableRva = 0x00588730;
constexpr std::array<std::uint8_t, 7> kRemoveConsumableBytes{
    0x39, 0x82, 0xF4, 0x0C, 0x00, 0x00, 0x56,
};
// Free list of the 48 nodes all humans share; null once every node is taken.
inline constexpr std::ptrdiff_t kConsumableFreeListRva = 0x01D43FA0;
inline constexpr std::ptrdiff_t kDrugCountRva = 0x01D43FA4;
inline constexpr std::ptrdiff_t kDrugTableRva = 0x026EF860;
inline constexpr std::size_t kDrugStride = 0x50;
inline constexpr std::size_t kDrugCostOffset = 0x24;
inline constexpr std::size_t kDrugTypeOffset = 0x34;
inline constexpr std::size_t kMaxDrugNameLength = 64;
// Circular list of held nodes, and the node being consumed right now.
inline constexpr std::size_t kHumanConsumablesOffset = 0xCF4;
inline constexpr std::size_t kHumanConsumingNodeOffset = 0xCF8;
inline constexpr std::size_t kConsumableNodeDrugOffset = 0x08;
// The slot scan drops the whole list when it finds more nodes than this.
inline constexpr std::size_t kConsumableSlots = 4;

struct ConsumableFunctions {
    std::uintptr_t give;
    std::uintptr_t remove;
};

struct HeldConsumable {
    std::uintptr_t node;
    std::int32_t type;
    std::int32_t cost;
};

struct HeldConsumables {
    std::size_t nodeCount;
    std::vector<HeldConsumable> items;
};

[[nodiscard]] std::optional<ConsumableFunctions> ResolveConsumableFunctions(
    const ModuleInfo& game) noexcept {
    const auto giveAddress = game.base + kGiveConsumableRva;
    const auto removeAddress = game.base + kRemoveConsumableRva;

    if (!ValidateHookTarget(game, giveAddress, kGiveConsumableBytes) ||
        !ValidateHookTarget(game, removeAddress, kRemoveConsumableBytes)) {
        return std::nullopt;
    }

    return ConsumableFunctions{.give = giveAddress, .remove = removeAddress};
}

[[nodiscard]] std::optional<std::int32_t> FindDrugType(
    const ModuleInfo& game, const std::string_view drugName) {
    const auto count = ReadMemory<std::int32_t>(game.base + kDrugCountRva);
    if (!count) {
        return std::nullopt;
    }

    for (std::int32_t i = 0; i < *count; ++i) {
        const auto entry = game.base + kDrugTableRva + i * kDrugStride;
        const auto namePointer = ReadMemory<std::uint32_t>(entry);
        if (!namePointer || *namePointer == 0) {
            continue;
        }

        const auto name = ReadFixedString(*namePointer, kMaxDrugNameLength);
        // The game finds entries by a lowercase name hash.
        if (name && std::ranges::equal(*name, drugName, [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) ==
                       std::tolower(static_cast<unsigned char>(b));
            })) {
            return ReadMemory<std::int32_t>(entry + kDrugTypeOffset);
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<HeldConsumables> ReadHeldConsumables(
    const std::uintptr_t human) {
    const auto head =
        ReadMemory<std::uint32_t>(human + kHumanConsumablesOffset);
    if (!head) {
        return std::nullopt;
    }

    HeldConsumables held{};
    std::uintptr_t node = *head;
    while (node != 0 && held.nodeCount < kConsumableSlots) {
        const auto drug =
            ReadMemory<std::uint32_t>(node + kConsumableNodeDrugOffset);
        const auto next = ReadMemory<std::uint32_t>(node);
        if (!drug || !next) {
            return std::nullopt;
        }

        ++held.nodeCount;
        if (*drug != 0) {
            const auto type = ReadMemory<std::int32_t>(*drug + kDrugTypeOffset);
            const auto cost = ReadMemory<std::int32_t>(*drug + kDrugCostOffset);
            if (!type || !cost) {
                return std::nullopt;
            }
            held.items.push_back({.node = node, .type = *type, .cost = *cost});
        }

        node = *next;
        if (node == *head) {
            break;
        }
    }

    return held;
}

// The cheapest item, of the incoming drug type if any is held. Never the one
// being consumed: its node would go back to the pool mid-animation.
[[nodiscard]] std::optional<std::uintptr_t> PickConsumableToReplace(
    const std::vector<HeldConsumable>& items, const std::int32_t incomingType,
    const std::uintptr_t consumingNode) {
    const auto rank = [incomingType](const HeldConsumable& item) {
        return std::pair{item.type != incomingType, item.cost};
    };

    const HeldConsumable* picked = nullptr;
    for (const auto& item : items) {
        if (item.node != consumingNode &&
            (!picked || rank(item) < rank(*picked))) {
            picked = &item;
        }
    }

    return picked ? std::optional{picked->node} : std::nullopt;
}

[[nodiscard]] std::uintptr_t CallGiveConsumable(
    const std::uintptr_t functionAddress, const char* drugName,
    const std::uintptr_t human) {
    std::uintptr_t node{};
    __asm {
        mov ecx, drugName
        push human
        call functionAddress
        add esp, 4
        mov node, eax
    }
    return node;
}

void CallRemoveConsumable(const std::uintptr_t functionAddress,
                          const std::uintptr_t node,
                          const std::uintptr_t human) {
    __asm {
        mov eax, node
        mov edx, human
        call functionAddress
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

bool CanGiveConsumable() {
    const auto game = InspectSupportedGameModule();
    return game && ResolveConsumableFunctions(*game).has_value();
}

bool GiveConsumable(const char* drugName) {
    if (!drugName || drugName[0] == '\0') {
        return false;
    }

    const auto game = InspectSupportedGameModule();
    if (!game) {
        return false;
    }

    const auto functions = ResolveConsumableFunctions(*game);
    if (!functions) {
        return false;
    }

    const auto playerAddress =
        ReadMemory<std::uint32_t>(game->base + addresses::kPlayerGlobalRva);

    if (!playerAddress || *playerAddress == 0) {
        return false;
    }

    // Checked up front so an unknown name never costs a held item.
    const auto drugType = FindDrugType(*game, drugName);
    if (!drugType) {
        return false;
    }

    const auto player = static_cast<std::uintptr_t>(*playerAddress);
    const auto held = ReadHeldConsumables(player);
    if (!held) {
        return false;
    }

    if (held->nodeCount >= kConsumableSlots) {
        const auto consumingNode =
            ReadMemory<std::uint32_t>(player + kHumanConsumingNodeOffset);
        if (!consumingNode) {
            return false;
        }

        const auto replaced =
            PickConsumableToReplace(held->items, *drugType, *consumingNode);
        if (!replaced) {
            return false;
        }

        CallRemoveConsumable(functions->remove, *replaced, player);
    }

    // The give function writes to the node without checking the pool.
    const auto freeList =
        ReadMemory<std::uint32_t>(game->base + kConsumableFreeListRva);
    if (!freeList || *freeList == 0) {
        return false;
    }

    return CallGiveConsumable(functions->give, drugName, player) != 0;
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