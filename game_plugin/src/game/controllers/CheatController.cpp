#include "CheatController.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "game/Addresses.hpp"
#include "game/BytePatch.hpp"
#include "game/GameThreadDispatcher.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
struct CheatDefinition {
    std::string_view itemName;
    std::uint32_t index;
    std::string_view phoneCode;
    std::ptrdiff_t callbackRva;
    std::uint32_t activations;
};

constexpr std::ptrdiff_t weaponCallbackRva{0x00285370};
constexpr std::ptrdiff_t vehicleCallbackRva{0x00285270};
constexpr std::ptrdiff_t weatherCallbackRva{0x00285480};
constexpr std::ptrdiff_t thunderstormCallbackRva{0x002854C0};
constexpr std::ptrdiff_t restoreWeatherCallbackRva{0x00285B20};
constexpr std::ptrdiff_t timeCallbackRva{0x00285220};
constexpr std::ptrdiff_t noPoliceNotorietyRva{0x00284D40};
constexpr std::ptrdiff_t noGangNotorietyRva{0x00284D70};
constexpr std::ptrdiff_t carRepairRva{0x002852E0};
constexpr std::ptrdiff_t maxHealthRva{0x00284ED0};
constexpr std::array<CheatDefinition, 42> supportedApItems{{
    {"Weather: Clear Skies", 1, "#78669", weatherCallbackRva, 1},
    {"Weather: Restore Normal Cycle", 0, "#78670", restoreWeatherCallbackRva,
     1},
    {"Trap: Heavy Rain", 4, "#78666", weatherCallbackRva, 1},
    {"Trap: Light Rain", 5, "#78668", weatherCallbackRva, 1},
    {"Weather: Overcast", 6, "#78665", weatherCallbackRva, 1},
    {"Trap: Thunderstorm", 25, "#666", thunderstormCallbackRva, 1},
    {"Time: Midnight", 7, "#2400", timeCallbackRva, 1},
    {"Time: Noon", 8, "#1200", timeCallbackRva, 1},
    {"Remove Police Notoriety", 20, "#50", noPoliceNotorietyRva, 1},
    {"Remove Gang Notoriety", 21, "#51", noGangNotorietyRva, 1},
    {"Car Repair", 24, "#1056", carRepairRva, 1},
    {"Max Health", 16, "#1", maxHealthRva, 1},
    {"Five-O (Cop Car)", 68, "#1055", vehicleCallbackRva, 1},
    {"Peewee (Bike)", 131, "#7266837", vehicleCallbackRva, 1},
    {"Gyro Daddy (Helicopter)", 129, "#4976", vehicleCallbackRva, 1},
    {"Destroy (UFO)", 130, "#728237", vehicleCallbackRva, 1},
    {"Miami (Boat)", 114, "#826", vehicleCallbackRva, 1},
    {"Attrazione (Car)", 56, "#1043", vehicleCallbackRva, 1},
    {"Fire Truck", 57, "#1044", vehicleCallbackRva, 1},
    {"Oring", 75, "#1063", vehicleCallbackRva, 1},
    {"Stilwater Municipal (Trash Truck)", 84, "#1072", vehicleCallbackRva, 1},
    {"Raycaster (Car)", 81, "#1068", vehicleCallbackRva, 1},
    {"Superiore (Car)", 85, "#1073", vehicleCallbackRva, 1},
    {"Reaper (Car)", 79, "#1069", vehicleCallbackRva, 1},
    {"The Job (Limo)", 87, "#1075", vehicleCallbackRva, 1},
    {"Vortex (Car)", 125, "#1080", vehicleCallbackRva, 1},
    {"Zenith (Car)", 126, "#1081", vehicleCallbackRva, 1},
    {"Titan (Truck)", 88, "#1076", vehicleCallbackRva, 1},
    {"Quota (Car)", 78, "#1066", vehicleCallbackRva, 1},
    {"Quasar (Car)", 77, "#1065", vehicleCallbackRva, 1},
    {"Phoenix (Car)", 76, "#1064", vehicleCallbackRva, 1},
    {"Longhauler (Truck)", 73, "#1061", vehicleCallbackRva, 1},
    {"Mongoose (Bike)", 74, "#1062", vehicleCallbackRva, 1},
    {"Justice (Car)", 70, "#1058", vehicleCallbackRva, 1},
    {"FBI Car", 67, "#1054", vehicleCallbackRva, 1},
    {"Hollywood (Car)", 69, "#1057", vehicleCallbackRva, 1},
    {"Melbourne (Bike)", 109, "#803", vehicleCallbackRva, 1},
    {"Sandstorm (Bike)", 111, "#805", vehicleCallbackRva, 1},
    {"Ambulance", 53, "#1040", vehicleCallbackRva, 1},
    {"Python (Boat)", 115, "#827", vehicleCallbackRva, 1},
    {"Anchor (Boat)", 54, "#1041", vehicleCallbackRva, 1},
    {"Eiswolf (Car)", 66, "#1053", vehicleCallbackRva, 1},
}};

const CheatDefinition* FindCheat(const std::string_view itemName) {
    const auto found = std::ranges::find_if(
        supportedApItems,
        [itemName](const auto& cheat) { return cheat.itemName == itemName; });
    return found == supportedApItems.end() ? nullptr : &*found;
}
}  // namespace

struct CheatController::Implementation {
    static constexpr std::size_t saveFlagSize{12};

    Implementation() = default;

    ~Implementation() {
        Remove();
    }

    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

    bool Install(const std::vector<std::string>& managedItems) {
        const auto game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }

        gameBase = game->base;
        managedItemNames.clear();
        managedItemNames.insert(managedItems.begin(), managedItems.end());
        activateAddress = game->base + addresses::kCheatActivateRva;
        saveFlagAddress = game->base + addresses::kCheatSaveFlagSiteRva;

        constexpr std::array<std::uint8_t, 6> expectedActivate{
            0x81, 0xEC, 0x20, 0x05, 0x00, 0x00};
        if (!ValidateHookTarget(*game, activateAddress, expectedActivate)) {
            return false;
        }

        constexpr std::array<std::uint8_t, saveFlagSize> expectedSaveFlag{
            0x88, 0x1D, 0x5A, 0x7B, 0x52, 0x02,
            0x88, 0x1D, 0xE6, 0x7B, 0x52, 0x02,
        };
        std::array<std::uint8_t, saveFlagSize> nops{};
        nops.fill(0x90);

        // A site that is already NOPed is accepted as-is and not owned, so it
        // is not restored on removal.
        if (!MatchesBytes(saveFlagAddress, nops) &&
            !saveFlagPatch.Apply("Cheats", "cheat save-flag instructions",
                                 saveFlagAddress, expectedSaveFlag, nops)) {
            return false;
        }

        installed = true;
        return true;
    }

    void Remove() {
        saveFlagPatch.Restore();
        installed = false;
    }

    bool ActivateReceivedItem(const std::string_view itemName,
                              GameThreadDispatcher& dispatcher,
                              ItemCompletion completion) const {
        if (!installed) {
            return false;
        }

        const auto* const definition = FindCheat(itemName);
        if (!definition || !managedItemNames.contains(std::string{itemName})) {
            return false;
        }

        std::optional<std::uint32_t> count =
            ReadMemory<std::uint32_t>(gameBase + addresses::kCheatCountRva);

        if (!count || definition->index >= *count) {
            return false;
        }

        const auto record = gameBase + addresses::kCheatTableRva +
                            static_cast<std::uintptr_t>(definition->index) *
                                addresses::kCheatRecordSize;

        std::optional<std::uint32_t> phoneCodePointer =
            ReadMemory<std::uint32_t>(record +
                                      addresses::kCheatPhoneCodePointerOffset);
        std::optional<std::uint32_t> callback = ReadMemory<std::uint32_t>(
            record + addresses::kCheatActivateCallbackOffset);

        if (!phoneCodePointer || !callback ||
            *callback != gameBase + definition->callbackRva ||
            !IsExecutableAddress(*callback)) {
            return false;
        }

        const auto phoneCode = ReadFixedString(*phoneCodePointer, 32);
        if (!phoneCode || *phoneCode != definition->phoneCode) {
            return false;
        }

        const auto function = activateAddress;
        const auto activationCount = definition->activations;

        const bool queued = dispatcher.Dispatch([function, record,
                                                 activationCount, completion] {
            if (!completion.CanApply()) {
                return;
            }
            for (std::uint32_t index = 0; index < activationCount; ++index) {
                InvokeActivate(function, record);
            }
            completion.Complete(true);
        });

        if (!queued) {
            LogWarning("Cheats",
                       "Failed to queue received item for game thread: " +
                           std::string{itemName});
            return false;
        }

        LogDebug("Cheats", "Queued received item for game thread: " +
                               std::string{itemName});

        return true;
    }

   private:
    static void InvokeActivate(const std::uintptr_t function,
                               const std::uintptr_t record) {
        __asm {
                push esi
                mov esi, record
                push 1
                call function
                pop esi
        }
    }

    std::uintptr_t gameBase{};
    std::uintptr_t activateAddress{};
    std::uintptr_t saveFlagAddress{};
    BytePatch saveFlagPatch;
    std::unordered_set<std::string> managedItemNames;
    bool installed{};
};

CheatController::CheatController() = default;

CheatController::~CheatController() {
    Remove();
}

bool CheatController::Install(const std::vector<std::string>& managedItems) {
    if (implementation_) {
        return false;
    }

    implementation_ = std::make_unique<Implementation>();
    if (!implementation_->Install(managedItems)) {
        implementation_.reset();
        return false;
    }

    return true;
}

void CheatController::Remove() {
    implementation_.reset();
}

bool CheatController::ActivateReceivedItem(const std::string_view itemName,
                                           GameThreadDispatcher& dispatcher,
                                           ItemCompletion completion) {
    return implementation_ && implementation_->ActivateReceivedItem(
                                  itemName, dispatcher, std::move(completion));
}

bool CheatController::SupportsItem(const std::string_view itemName) {
    return FindCheat(itemName) != nullptr;
}

}  // namespace sr2ap
