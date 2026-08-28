#include "sr2ap/Notoriety.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

#include "sr2ap/Addresses.hpp"
#include "sr2ap/Cheats.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

namespace sr2ap {
namespace {
struct NotorietyTrap {
    std::string_view itemName;
    int faction_id;
};

constexpr std::array<NotorietyTrap, 4> supportedTraps{{
    {"Trap: Max Police Notoriety", 3},
    {"Trap: Max Ronin Notoriety", 1},
    {"Trap: Max Brotherhood Notoriety", 0},
    {"Trap: Max Samedi Notoriety", 2},
}};
constexpr float maximumNotorietyLevel{5.0F};
constexpr float noNotoriety{0.0F};

const NotorietyTrap* FindTrap(const std::string_view itemName) {
    const auto found = std::find_if(
        supportedTraps.begin(), supportedTraps.end(),
        [itemName](const auto& trap) { return trap.itemName == itemName; });
    return found == supportedTraps.end() ? nullptr : &*found;
}

}  // namespace

struct NotorietyController::Implementation {
    bool Install() {
        const auto game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }

        setAddress = game->base + addresses::kNotorietySetRva;
        constexpr std::array<std::uint8_t, 6> expectedSet{0x81, 0xEC, 0x2C,
                                                          0x05, 0x00, 0x00};
        std::array<std::uint8_t, expectedSet.size()> actualSet{};
        installed =
            IsInsideModule(game->handle,
                           reinterpret_cast<const void*>(setAddress)) &&
            IsExecutableAddress(reinterpret_cast<const void*>(setAddress)) &&
            DetectDetour(reinterpret_cast<const void*>(setAddress)) ==
                DetourKind::None &&
            SafeCopy(reinterpret_cast<const void*>(setAddress),
                     actualSet.data(), actualSet.size()) &&
            actualSet == expectedSet;
        return installed;
    }

    void Remove() noexcept {
        installed = false;
        setAddress = 0;
    }

    bool ActivateReceivedItem(const std::string_view itemName,
                              CheatController& gameThreadDispatcher) const {
        const auto* const trap = FindTrap(itemName);
        if (!trap) {
            return false;
        }

        if (!installed) {
            LogWarning(
                "Notoriety",
                "Rejected trap while native integration is unavailable: " +
                    std::string{itemName});
            return false;
        }

        const auto nativeSetAddress = setAddress;
        const int factionId = trap->faction_id;
        const bool isPolice = factionId == 3;
        std::string ownedItemName{itemName};

        const bool queued = gameThreadDispatcher.DispatchOnGameThread(
            [nativeSetAddress, factionId, isPolice,
             itemName = std::move(ownedItemName)] {
                const auto setNotoriety =
                    reinterpret_cast<void(__cdecl*)(int, float)>(
                        nativeSetAddress);

                // trap doesn't work if the player already has notoriety, so
                // need to set
                // it to 0 before setting it to max for police just need to set
                // the police notoriety to 0, but for a gang need to set all
                // gang notoriety to 0 first
                if (isPolice) {
                    setNotoriety(factionId, noNotoriety);
                } else {
                    setNotoriety(0, noNotoriety);
                    setNotoriety(1, noNotoriety);
                    setNotoriety(2, noNotoriety);
                }

                setNotoriety(factionId, maximumNotorietyLevel);

                LogInfo("Notoriety",
                        "Activated trap item=" + itemName + " faction=" +
                            std::to_string(factionId) + " level=5");
            });

        if (!queued) {
            LogWarning("Notoriety", "Unable to queue trap for game thread: " +
                                        std::string{itemName});
        }

        return queued;
    }

    std::uintptr_t setAddress{};
    bool installed{};
};

NotorietyController::NotorietyController() = default;
NotorietyController::~NotorietyController() = default;

bool NotorietyController::Install() {
    if (!implementation_) {
        implementation_ = std::make_unique<Implementation>();
    }
    return implementation_->Install();
}

void NotorietyController::Remove() {
    if (implementation_) {
        implementation_->Remove();
    }
}

bool NotorietyController::ActivateReceivedItem(
    const std::string_view itemName,
    CheatController& gameThreadDispatcher) const {
    return implementation_ && implementation_->ActivateReceivedItem(
                                  itemName, gameThreadDispatcher);
}
}  // namespace sr2ap
