#pragma once

#include "ModuleInfo.hpp"

namespace sr2ap {
namespace Lua {
inline constexpr std::ptrdiff_t kMissionCompletedQueryRva = 0x002A6E50;

bool IsMissionComplete(const ModuleInfo& game, const char* mission);
}  // namespace Lua
}  // namespace sr2ap
