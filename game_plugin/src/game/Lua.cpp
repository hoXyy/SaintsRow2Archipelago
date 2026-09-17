#include "Lua.hpp"

#include <array>
#include <cstdint>
#include <limits>

#include "Addresses.hpp"
#include "Memory.hpp"

namespace sr2ap {
namespace {
inline constexpr std::ptrdiff_t kCashAddRva = 0x0056A430;
constexpr std::array<std::uint8_t, 11> kCashAddBytes{
    0x81, 0xEC, 0x2C, 0x05, 0x00, 0x00, 0xA1, 0x50, 0x0B, 0xE8, 0x00,
};
constexpr std::int32_t kCentsPerDollar{100};

using IsMissionCompletedLuaFunction = bool(__thiscall*)(const char*);
using GiveMoneyLuaFunction =
    void(__thiscall*)(void* player, const std::int32_t* amountInCents);

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
}  // namespace Lua
}  // namespace sr2ap