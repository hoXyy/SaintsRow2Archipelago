#pragma once

#include <cstdint>
#include <optional>

#include "game/DeathSuppression.hpp"
#include "game/ModuleInfo.hpp"

namespace sr2ap::death {
// The DeathLink-only targets. Killing, the death check and detonation come
// from game/Lua.hpp.
struct Functions {
    std::uintptr_t setHitPoints{};
    std::uintptr_t currentActivityType{};
    std::uintptr_t currentDiversionType{};
};

// Null when any target is missing or already detoured by another mod.
std::optional<Functions> ResolveFunctions(const ModuleInfo& game);

struct PlayerVitals {
    std::uintptr_t player{};
    float hitPoints{};
    std::int32_t bleedOutTimer{};

    [[nodiscard]] bool IsAlive() const noexcept {
        return hitPoints > 0.0f && bleedOutTimer < 0;
    }
};

// Any thread; only reads memory.
std::optional<PlayerVitals> ReadPlayerVitals(const ModuleInfo& game);

enum class Outcome {
    Applied,
    // Not possible right now (activity not at a killable stage, kill did not
    // land); try again later.
    Deferred,
    // Nothing to do: the player is already dying or the activity car is gone.
    Dropped,
};

struct Result {
    Outcome outcome{};
    const char* detail{};
};

// Game thread. Kills the player, or in Demolition Derby and races destroys
// their car, since those activities reset the player instead of killing them.
Result KillPlayer(const ModuleInfo& game, const Functions& functions,
                  DeathSuppression& suppression);
}  // namespace sr2ap::death
