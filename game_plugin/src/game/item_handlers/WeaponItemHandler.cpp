#include "WeaponItemHandler.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "fmt/format.h"
#include "game/GameThreadDispatcher.hpp"
#include "game/ItemHandler.hpp"
#include "game/Lua.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
struct WeaponItem {
    std::string_view itemName;
    std::string_view weapon;
    std::uint32_t ammo;
};

constexpr std::array<WeaponItem, 38> supportedItems{{
    {"Pimp Slap", "pimp_slap", 0},
    {"Samurai Sword", "samurai_sword", 0},
    {"Pepper Spray", "pepper_spray", 0},
    {"Stun Gun", "stun_gun", 0},
    {"Nightstick", "nightstick", 0},
    {"Knife", "knife", 0},
    {"Baseball Bat", "baseball_bat", 0},
    {"Machete", "machete", 0},
    {"Sledgehammer", "sledgehammer", 0},
    {"Chainsaw", "chainsaw", 0},
    {"Crowbar", "tire_iron", 0},
    {"Butterfly Knife", "butterfly_knife", 0},

    {"VICE 9", "beretta", 200},
    {"Kobra", "Holt_55", 200},
    {"NR4", "glock", 200},
    {"GDHC .50", "desert eagle", 200},
    {".44 Shepherd", "magnum", 200},

    {"T3K Urban", "tec9", 500},
    {"GAL 43", "Gal43", 500},
    {"SKR-9 Threat", "SKR-9", 500},

    {"AS14 Hammer", "AS14", 80},
    {"Tombstone", "pump_action_shotgun", 80},
    {"XS-2 Ultimax", "XS-2", 80},
    {"12 Gauge", "twelve_gauge", 80},
    {"Pimp Cane", "pimpcane", 80},

    {"K6 Krukov", "ak47", 300},
    {"AR-50 XMAC", "AR50", 300},
    {"AR200 SAW", "AR200", 300},

    {"McManus 2010", "mcmanus2010", 50},
    {"RPG Launcher", "rpg_launcher", 10},
    {"Annihilator RPG", "rpg_annihilator", 10},
    {"Minigun", "minigun", 500},
    {"Flamethrower", "flamethrower", 500},

    {"Flashbang", "flashbang", 10},
    {"Molotov Cocktail", "molotov", 10},
    {"Pipe Bomb", "pipe_bomb", 10},
    {"Satchel Charge", "satchel", 10},
    {"Hand Grenade", "grenade", 10},
}};

const WeaponItem* FindItem(const std::string_view itemName) noexcept {
    const auto found = std::ranges::find_if(
        supportedItems,
        [itemName](const auto& trap) { return trap.itemName == itemName; });
    return found == supportedItems.end() ? nullptr : &*found;
}

class WeaponItemHandler final : public ItemHandler {
   public:
    explicit WeaponItemHandler(GameThreadDispatcher& dispatcher)
        : dispatcher_(dispatcher) {
    }

    [[nodiscard]] bool CanHandleItem(
        std::string_view itemName) const noexcept override {
        return FindItem(itemName) != nullptr;
    }

    [[nodiscard]] bool Install() override {
        return Lua::CanAddWeapon();
    }

    [[nodiscard]] bool Handle(const std::string_view itemName,
                              ItemCompletion completion) override {
        const auto* const item = FindItem(itemName);
        if (!item) {
            return false;
        }

        const auto queued = dispatcher_.Dispatch(
            [weapon = std::string{item->weapon}, ammoCount = item->ammo,
             itemName = std::string{itemName}, completion] {
                if (!completion.CanApply()) {
                    return;
                }

                if (!Lua::AddWeapon(weapon.c_str(), 1)) {
                    LogWarning(
                        "Weapon",
                        fmt::format("Failed to trigger item={}", itemName));
                    completion.Complete(false);
                    return;
                }

                // The weapon itself was granted; a retry would duplicate it,
                // so missing ammo is only logged.
                completion.Complete(true);

                if (ammoCount > 0) {
                    if (!Lua::AddWeaponAmmo(weapon.c_str(), ammoCount)) {
                        LogWarning(
                            "Weapon",
                            fmt::format("Failed to add ammo to weapon={}",
                                        weapon));
                    }
                }
            });

        if (!queued) {
            LogWarning("Weapon",
                       fmt::format("Failed to queue item={}", itemName));
        }

        return queued;
    }

    void Remove() override {
    }

   private:
    GameThreadDispatcher& dispatcher_;
};
}  // namespace

ItemHandlerPtr CreateWeaponItemHandler(GameThreadDispatcher& dispatcher) {
    return std::make_unique<WeaponItemHandler>(dispatcher);
}
}  // namespace sr2ap