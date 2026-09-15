#include "Lua.hpp"

#include "Memory.hpp"

namespace sr2ap {
namespace {
using IsMissionCompletedLuaFunction = bool(__thiscall*)(const char*);
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

    const auto query = reinterpret_cast<IsMissionCompletedLuaFunction>(address);

    return query(mission);
}
}  // namespace Lua
}  // namespace sr2ap