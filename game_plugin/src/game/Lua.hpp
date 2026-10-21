#pragma once

#include <cstdint>

#include "ModuleInfo.hpp"

namespace sr2ap::Lua {
inline constexpr std::ptrdiff_t kMissionCompletedQueryRva = 0x002A6E50;
bool CanQueryMissions(const ModuleInfo& game);
// Callers must check CanQueryMissions first; this does not re-validate.
bool IsMissionComplete(const ModuleInfo& game, const char* mission);
bool CanUnlockActivity();
bool UnlockActivity(const char* progressionTag);
bool CanGiveMoney();
bool GiveMoney(const std::int32_t amount);
bool CanAddWeapon();
bool AddWeapon(const char* weaponName, std::int32_t count);
bool AddWeaponAmmo(const char* weaponName, std::int32_t amount);

// The character and vehicle functions below run on the game thread. Callers
// must check the matching Can* function first; they do not re-validate.
bool CanKillCharacter(const ModuleInfo& game);
// character_kill: removes all hit points. Invulnerable characters survive.
void KillCharacter(const ModuleInfo& game, std::uintptr_t human);
bool CanQueryCharacterDead(const ModuleInfo& game);
// character_is_dead: out of hit points, or bleeding out.
bool IsCharacterDead(const ModuleInfo& game, std::uintptr_t human);
bool CanDetonateVehicle(const ModuleInfo& game);
// vehicle_detonate. False when the vehicle position cannot be read.
bool DetonateVehicle(const ModuleInfo& game, std::uintptr_t vehicle);
}  // namespace sr2ap::Lua