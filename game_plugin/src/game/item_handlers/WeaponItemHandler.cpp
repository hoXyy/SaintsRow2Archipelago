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
    {"Weapon: Pimp Slap", "pimp_slap", 0},
    {"Weapon: Samurai Sword", "samurai_sword", 0},
    {"Weapon: Pepper Spray", "pepper_spray", 0},
    {"Weapon: Stun Gun", "stun_gun", 0},
    {"Weapon: Nightstick", "nightstick", 0},
    {"Weapon: Knife", "knife", 0},
    {"Weapon: Baseball Bat", "baseball_bat", 0},
    {"Weapon: Machete", "machete", 0},
    {"Weapon: Sledgehammer", "sledgehammer", 0},
    {"Weapon: Chainsaw", "chainsaw", 0},
    {"Weapon: Crowbar", "tire_iron", 0},
    {"Weapon: Butterfly Knife", "butterfly_knife", 0},

    {"Weapon: VICE 9", "beretta", 200},
    {"Weapon: Kobra", "Holt_55", 200},
    {"Weapon: NR4", "glock", 200},
    {"Weapon: GDHC .50", "desert eagle", 200},
    {"Weapon: .44 Shepherd", "magnum", 200},

    {"Weapon: T3K Urban", "tec9", 500},
    {"Weapon: GAL 43", "Gal43", 500},
    {"Weapon: SKR-9 Threat", "SKR-9", 500},

    {"Weapon: AS14 Hammer", "AS14", 80},
    {"Weapon: Tombstone", "pump_action_shotgun", 80},
    {"Weapon: XS-2 Ultimax", "XS-2", 80},
    {"Weapon: 12 Gauge", "twelve_gauge", 80},
    {"Weapon: Pimp Cane", "pimpcane", 80},

    {"Weapon: K6 Krukov", "ak47", 300},
    {"Weapon: AR-50 XMAC", "AR50", 300},
    {"Weapon: AR200 SAW", "AR200", 300},

    {"Weapon: McManus 2010", "mcmanus2010", 50},
    {"Weapon: RPG Launcher", "rpg_launcher", 10},
    {"Weapon: Annihilator RPG", "rpg_annihilator", 10},
    {"Weapon: Minigun", "minigun", 500},
    {"Weapon: Flamethrower", "flamethrower", 500},

    {"Weapon: Flashbang", "flashbang", 10},
    {"Weapon: Molotov Cocktail", "molotov", 10},
    {"Weapon: Pipe Bomb", "pipe_bomb", 10},
    {"Weapon: Satchel Charge", "satchel", 10},
    {"Weapon: Hand Grenade", "grenade", 10},
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

    [[nodiscard]] bool Handle(const std::string_view itemName) override {
        const auto* const item = FindItem(itemName);
        if (!item) {
            return false;
        }

        const auto queued = dispatcher_.Dispatch(
            [weapon = std::string{item->weapon}, ammoCount = item->ammo,
             itemName = std::string{itemName}] {
                if (!Lua::AddWeapon(weapon.c_str(), 1)) {
                    LogWarning(
                        "Weapon",
                        fmt::format("Failed to trigger item={}", itemName));
                    return;
                }

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