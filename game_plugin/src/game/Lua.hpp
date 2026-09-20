#pragma once

#include "ModuleInfo.hpp"

namespace sr2ap::Lua {
inline constexpr std::ptrdiff_t kMissionCompletedQueryRva = 0x002A6E50;
bool IsMissionComplete(const ModuleInfo& game, const char* mission);
bool CanUnlockActivity();
bool UnlockActivity(const char* progressionTag);
bool CanGiveMoney();
bool GiveMoney(const std::int32_t amount);
}  // namespace sr2ap::Lua