#include "sr2ap/Cheats.hpp"

#include "sr2ap/Addresses.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <safetyhook.hpp>
#include <string>
#include <unordered_set>
#include <vector>

namespace sr2ap {
    namespace {
        struct CheatDefinition {
            std::string_view itemName;
            std::uint32_t index;
            std::string_view phoneCode;
            std::ptrdiff_t callbackRva;
            std::uint32_t activations;
        };

        constexpr std::ptrdiff_t cashCallbackRva{0x00284D10};
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
        constexpr std::array<CheatDefinition, 79> supportedApItems{{
            {"$1,000", 13, "#2274666399", cashCallbackRva, 1},
            {"$5,000", 13, "#2274666399", cashCallbackRva, 5},
            {"$10,000", 13, "#2274666399", cashCallbackRva, 10},
            {"Weapon: Pimp Slap", 52, "#969", weaponCallbackRva, 1},
            {"Weapon: Samurai Sword", 97, "#948", weaponCallbackRva, 1},
            {"Weapon: Pepper Spray", 93, "#943", weaponCallbackRva, 1},
            {"Weapon: Stun Gun", 101, "#953", weaponCallbackRva, 1},
            {"Weapon: Nightstick", 49, "#941", weaponCallbackRva, 1},
            {"Weapon: Knife", 43, "#936", weaponCallbackRva, 1},
            {"Weapon: Baseball Bat", 33, "#926", weaponCallbackRva, 1},
            {"Weapon: Machete", 45, "#937", weaponCallbackRva, 1},
            {"Weapon: Sledgehammer", 100, "#952", weaponCallbackRva, 1},
            {"Weapon: Chainsaw", 34, "#927", weaponCallbackRva, 1},
            {"Weapon: VICE 9", 105, "#957", weaponCallbackRva, 1},
            {"Weapon: Kobra", 44, "#934", weaponCallbackRva, 1},
            {"Weapon: NR4", 41, "#942", weaponCallbackRva, 1},
            {"Weapon: GDHC .50", 39, "#932", weaponCallbackRva, 1},
            {"Weapon: .44 Shepherd", 3, "#921", weaponCallbackRva, 1},
            {"Weapon: T3K Urban", 102, "#954", weaponCallbackRva, 1},
            {"Weapon: GAL 43", 38, "#931", weaponCallbackRva, 1},
            {"Weapon: SKR-9 Threat", 96, "#951", weaponCallbackRva, 1},
            {"Weapon: AS14 Hammer", 32, "#925", weaponCallbackRva, 1},
            {"Weapon: Tombstone", 104, "#956", weaponCallbackRva, 1},
            {"Weapon: XS-2 Ultimax", 106, "#958", weaponCallbackRva, 1},
            {"Weapon: 12 Gauge", 2, "#920", weaponCallbackRva, 1},
            {"Weapon: Pimp Cane", 51, "#944", weaponCallbackRva, 1},
            {"Weapon: K6 Krukov", 42, "#935", weaponCallbackRva, 1},
            {"Weapon: AR-50 XMAC", 29, "#923", weaponCallbackRva, 1},
            {"Weapon: AR200 SAW", 31, "#922", weaponCallbackRva, 1},
            {"Weapon: McManus 2010", 46, "#938", weaponCallbackRva, 1},
            {"Weapon: RPG Launcher", 94, "#946", weaponCallbackRva, 1},
            {"Weapon: Annihilator RPG", 95, "#947", weaponCallbackRva, 1},
            {"Weapon: Flashbang", 37, "#930", weaponCallbackRva, 1},
            {"Weapon: Molotov Cocktail", 48, "#940", weaponCallbackRva, 1},
            {"Weapon: Pipe Bomb", 50, "#945", weaponCallbackRva, 1},
            {"Weapon: Satchel Charge", 98, "#949", weaponCallbackRva, 1},
            {"Weapon: Hand Grenade", 40, "#933", weaponCallbackRva, 1},
            {"Weather: Clear Skies", 1, "#78669", weatherCallbackRva, 1},
            {"Weather: Restore Normal Cycle", 0, "#78670", restoreWeatherCallbackRva, 1},
            {"Weather: Heavy Rain", 4, "#78666", weatherCallbackRva, 1},
            {"Weather: Light Rain", 5, "#78668", weatherCallbackRva, 1},
            {"Weather: Overcast", 6, "#78665", weatherCallbackRva, 1},
            {"Weather: Thunderstorm", 25, "#666", thunderstormCallbackRva, 1},
            {"Time: Midnight", 7, "#2400", timeCallbackRva, 1},
            {"Time: Noon", 8, "#1200", timeCallbackRva, 1},
            {"Remove Police Notoriety", 20, "#50", noPoliceNotorietyRva, 1},
            {"Remove Gang Notoriety", 21, "#51", noGangNotorietyRva, 1},
            {"Car Repair", 24, "#1056", carRepairRva, 1},
            {"Max Health", 16, "#1", maxHealthRva, 1},
            {"Vehicle: Five-O", 68, "#1055", vehicleCallbackRva, 1},
            {"Vehicle: Peewee", 131, "#7266837", vehicleCallbackRva, 1},
            {"Vehicle: Gyro Daddy", 129, "#4976", vehicleCallbackRva, 1},
            {"Vehicle: Destroy (UFO)", 130, "#728237", vehicleCallbackRva, 1},
            {"Vehicle: Miami", 114, "#826", vehicleCallbackRva, 1},
            {"Vehicle: Attrazione", 56, "#1043", vehicleCallbackRva, 1},
            {"Vehicle: Fire Truck", 57, "#1044", vehicleCallbackRva, 1},
            {"Vehicle: Oring", 75, "#1063", vehicleCallbackRva, 1},
            {"Vehicle: Stilwater Municipal", 84, "#1072", vehicleCallbackRva, 1},
            {"Vehicle: Raycaster", 81, "#1068", vehicleCallbackRva, 1},
            {"Vehicle: Superiore", 85, "#1073", vehicleCallbackRva, 1},
            {"Vehicle: Reaper", 79, "#1069", vehicleCallbackRva, 1},
            {"Vehicle: The Job", 87, "#1075", vehicleCallbackRva, 1},
            {"Vehicle: Vortex", 125, "#1080", vehicleCallbackRva, 1},
            {"Vehicle: Zenith", 126, "#1081", vehicleCallbackRva, 1},
            {"Vehicle: Titan", 88, "#1076", vehicleCallbackRva, 1},
            {"Vehicle: Quota", 78, "#1066", vehicleCallbackRva, 1},
            {"Vehicle: Quasar", 77, "#1065", vehicleCallbackRva, 1},
            {"Vehicle: Phoenix", 76, "#1064", vehicleCallbackRva, 1},
            {"Vehicle: Longhauler", 73, "#1061", vehicleCallbackRva, 1},
            {"Vehicle: Mongoose", 74, "#1062", vehicleCallbackRva, 1},
            {"Vehicle: Justice", 70, "#1058", vehicleCallbackRva, 1},
            {"Vehicle: FBI", 67, "#1054", vehicleCallbackRva, 1},
            {"Vehicle: Hollywood", 69, "#1057", vehicleCallbackRva, 1},
            {"Vehicle: Melbourne", 109, "#803", vehicleCallbackRva, 1},
            {"Vehicle: Sandstorm", 111, "#805", vehicleCallbackRva, 1},
            {"Vehicle: Ambulance", 53, "#1040", vehicleCallbackRva, 1},
            {"Vehicle: Python", 115, "#827", vehicleCallbackRva, 1},
            {"Vehicle: Anchor", 54, "#1041", vehicleCallbackRva, 1},
            {"Vehicle: Eiswolf", 66, "#1053", vehicleCallbackRva, 1},
        }};

        const CheatDefinition* FindCheat(const std::string_view itemName) {
            const auto found = std::find_if(supportedApItems.begin(), supportedApItems.end(),
                                            [itemName](const auto& cheat) { return cheat.itemName == itemName; });
            return found == supportedApItems.end() ? nullptr : &*found;
        }
    }  // namespace

    struct CheatController::Implementation {
        static constexpr std::size_t saveFlagSize{12};

        struct PendingActivation {
            std::uintptr_t record;
            std::uint32_t count;
        };

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
            frameDispatchAddress = game->base + addresses::kGameFrameDispatchSiteRva;
            saveFlagAddress = game->base + addresses::kCheatSaveFlagSiteRva;

            constexpr std::array<std::uint8_t, 6> expectedActivate{0x81, 0xEC, 0x20, 0x05, 0x00, 0x00};
            std::array<std::uint8_t, expectedActivate.size()> actualActivate{};
            if (!IsInsideModule(game->handle, reinterpret_cast<const void*>(activateAddress)) ||
                !IsExecutableAddress(reinterpret_cast<const void*>(activateAddress)) ||
                DetectDetour(reinterpret_cast<const void*>(activateAddress)) != DetourKind::None ||
                !SafeCopy(reinterpret_cast<const void*>(activateAddress), actualActivate.data(),
                          actualActivate.size()) ||
                actualActivate != expectedActivate) {
                return false;
            }

            constexpr std::array<std::uint8_t, 7> expectedFrameDispatch{
                0x83, 0x3D, 0x24, 0x8B, 0x52, 0x02, 0x00,
            };
            std::array<std::uint8_t, expectedFrameDispatch.size()> actualFrameDispatch{};
            if (!IsInsideModule(game->handle, reinterpret_cast<const void*>(frameDispatchAddress)) ||
                !IsExecutableAddress(reinterpret_cast<const void*>(frameDispatchAddress)) ||
                DetectDetour(reinterpret_cast<const void*>(frameDispatchAddress)) != DetourKind::None ||
                !SafeCopy(reinterpret_cast<const void*>(frameDispatchAddress), actualFrameDispatch.data(),
                          actualFrameDispatch.size()) ||
                actualFrameDispatch != expectedFrameDispatch) {
                return false;
            }

            auto hook = safetyhook::MidHook::create(reinterpret_cast<void*>(frameDispatchAddress), &FrameHook,
                                                    safetyhook::MidHook::StartDisabled);
            if (!hook) {
                return false;
            }
            frameHook = std::move(*hook);
            active.store(this, std::memory_order_release);
            if (const auto enabled = frameHook.enable(); !enabled) {
                active.store(nullptr, std::memory_order_release);
                frameHook.reset();
                return false;
            }

            constexpr std::array<std::uint8_t, saveFlagSize> expectedSaveFlag{
                0x88, 0x1D, 0x5A, 0x7B, 0x52, 0x02, 0x88, 0x1D, 0xE6, 0x7B, 0x52, 0x02,
            };
            std::array<std::uint8_t, expectedSaveFlag.size()> actualSaveFlag{};
            if (!SafeCopy(reinterpret_cast<const void*>(saveFlagAddress), actualSaveFlag.data(),
                          actualSaveFlag.size())) {
                return false;
            }
            std::array<std::uint8_t, expectedSaveFlag.size()> nops{};
            nops.fill(0x90);
            if (actualSaveFlag == expectedSaveFlag) {
                originalSaveFlag = actualSaveFlag;

                if (!WriteCode(nops)) {
                    return false;
                }

                ownsSaveFlagPatch = true;
            } else if (actualSaveFlag != nops) {
                return false;
            }

            installed = true;
            return true;
        }

        void Remove() {
            if (!installed) {
                DisableFrameHook();
                return;
            }

            DisableFrameHook();

            if (ownsSaveFlagPatch) {
                std::array<std::uint8_t, saveFlagSize> current{};
                std::array<std::uint8_t, saveFlagSize> nops{};
                nops.fill(0x90);

                if (!SafeCopy(reinterpret_cast<const void*>(saveFlagAddress), current.data(), current.size()) ||
                    current != nops) {
                    LogWarning("Cheats", "Save-flag patch changed after installation; original bytes not restored");
                    ownsSaveFlagPatch = false;
                    installed = false;
                    return;
                }

                if (!WriteCode(originalSaveFlag)) {
                    LogError("Cheats", "Failed to restore cheat save-flag instructions");
                    ownsSaveFlagPatch = false;
                    installed = false;
                    return;
                }
            }
            ownsSaveFlagPatch = false;
            installed = false;
        }

        bool ActivateReceivedItem(const std::string_view itemName) {
            if (!installed) {
                return false;
            }
            const auto* definition = FindCheat(itemName);
            if (!definition || managedItemNames.find(std::string{itemName}) == managedItemNames.end()) {
                return false;
            }

            std::uint32_t count{};
            if (!SafeCopy(reinterpret_cast<const void*>(gameBase + addresses::kCheatCountRva), &count, sizeof(count)) ||
                definition->index >= count) {
                return false;
            }
            const auto record = gameBase + addresses::kCheatTableRva +
                                static_cast<std::uintptr_t>(definition->index) * addresses::kCheatRecordSize;
            std::uint32_t phoneCodePointer{};
            std::uint32_t callback{};

            if (!SafeCopy(reinterpret_cast<const void*>(record + addresses::kCheatPhoneCodePointerOffset),
                          &phoneCodePointer, sizeof(phoneCodePointer)) ||
                !SafeCopy(reinterpret_cast<const void*>(record + addresses::kCheatActivateCallbackOffset), &callback,
                          sizeof(callback)) ||
                callback != gameBase + definition->callbackRva ||
                !IsExecutableAddress(reinterpret_cast<const void*>(callback))) {
                return false;
            }

            const auto phoneCode = ReadFixedString(phoneCodePointer, 32);
            if (!phoneCode || *phoneCode != definition->phoneCode) {
                return false;
            }

            {
                std::lock_guard<std::mutex> lock(pendingMutex);
                pendingActivations.push_back({record, definition->activations});
            }

            LogDebug("Cheats", "Queued received item for game thread: " + std::string{itemName});
            return true;
        }

       private:
        void DisableFrameHook() {
            if (frameHook) {
                if (const auto disabled = frameHook.disable(); !disabled) {
                    LogError("Cheats", "Failed to disable game-thread dispatch hook");
                    return;
                }
                frameHook.reset();
            }
            active.store(nullptr, std::memory_order_release);
            std::lock_guard<std::mutex> lock(pendingMutex);
            pendingActivations.clear();
        }

        void DrainPendingActivations() {
            std::vector<PendingActivation> pending;
            {
                std::lock_guard<std::mutex> lock(pendingMutex);
                pending.swap(pendingActivations);
            }
            for (const auto& activation : pending) {
                for (std::uint32_t index = 0; index < activation.count; ++index) {
                    InvokeActivate(activateAddress, activation.record);
                }
            }
        }

        static void FrameHook(safetyhook::Context&) {
            auto* const self = active.load(std::memory_order_acquire);
            if (!self) {
                return;
            }
            self->DrainPendingActivations();
        }

        static void InvokeActivate(const std::uintptr_t function, const std::uintptr_t record) {
            __asm {
                push esi
                mov esi, record
                push 1
                call function
                pop esi
            }
        }

        bool WriteCode(const std::array<std::uint8_t, 12>& bytes) const {
            auto* const destination = reinterpret_cast<void*>(saveFlagAddress);
            DWORD previousProtection{};
            if (!VirtualProtect(destination, bytes.size(), PAGE_EXECUTE_READWRITE, &previousProtection)) {
                return false;
            }

            std::memcpy(destination, bytes.data(), bytes.size());
            FlushInstructionCache(GetCurrentProcess(), destination, bytes.size());
            DWORD ignoredProtection{};
            VirtualProtect(destination, bytes.size(), previousProtection, &ignoredProtection);
            return true;
        }

        std::uintptr_t gameBase{};
        std::uintptr_t activateAddress{};
        std::uintptr_t frameDispatchAddress{};
        std::uintptr_t saveFlagAddress{};
        std::array<std::uint8_t, saveFlagSize> originalSaveFlag{};
        bool ownsSaveFlagPatch{};
        safetyhook::MidHook frameHook;
        std::mutex pendingMutex;
        std::vector<PendingActivation> pendingActivations;
        std::unordered_set<std::string> managedItemNames;
        bool installed{};
        inline static std::atomic<Implementation*> active{};
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

    bool CheatController::ActivateReceivedItem(const std::string_view itemName) {
        return implementation_ && implementation_->ActivateReceivedItem(itemName);
    }

    bool CheatController::SupportsItem(const std::string_view itemName) {
        return FindCheat(itemName) != nullptr;
    }
}  // namespace sr2ap
