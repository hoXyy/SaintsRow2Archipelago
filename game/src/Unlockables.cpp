#include "sr2ap/Unlockables.hpp"

#include "sr2ap/Addresses.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <safetyhook.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace sr2ap {
    namespace {
        struct ManagedUnlockable {
            std::string_view itemName;
            std::uint32_t hash;
        };

        constexpr std::array<ManagedUnlockable, 92> managedUnlockables{{
            {"Weapon Cache: Kobra Pistols", 0x8AFA5DC5},
            {"Weapon Cache: GAL 43", 0x32373CF6},
            {"Weapon Cache: Minigun", 0x1B14530F},
            {"Weapon Cache: Pimpcane", 0x87F731EE},
            {"Weapon Cache: AR-50 XMAC Special", 0x8AD02963},
            {"Weapon Cache: Infinite Pistol Ammo", 0xB14D40BF},
            {"Weapon Cache: Infinite SMG Ammo", 0x0E25ECC8},
            {"Weapon Cache: Infinite Shotgun Ammo", 0xD8C93508},
            {"Weapon Cache: Infinite Rifle Ammo", 0xB8A5236F},
            {"Upgraded Sprint", 0xCB77EE3E},
            {"Infinite Sprint", 0x9B63E698},
            {"Weapon Cache: Flamethrower", 0x9A8B4845},
            {"Weapon Cache: Annihilator RPG", 0x5CE5B28A},
            {"Health Generation x2", 0xB4A34E58},
            {"Health Generation x3", 0x3FCDF655},
            {"Improved Weapon Accuracy (5%)", 0x620D6795},
            {"Improved Weapon Accuracy (15%)", 0xDACEE671},
            {"Vehicle: Ultor Tornado", 0x7AC7C6F8},
            {"Vehicle: News Helicopter", 0x3697CA58},
            {"Vehicle: Medical Helicopter", 0x9169B869},
            {"Vehicle: Saints Oppressor", 0x9169B869},
            {"Vehicle: Buggy", 0x25A4CA19},
            {"Vehicle: Saints Combine", 0x70306F95},
            {"Vehicle: Saints Tow Truck", 0x8B7EFDEF},
            {"Vehicle: Saints Fire Truck", 0x33779D2D},
            {"Vehicle: Saints Ambulance", 0xDF75BBB3},
            {"Vehicle: Saints Taxi", 0x8956692D},
            {"Vehicle: Saints Sabretooth", 0x79706B27},
            {"Vehicle: Saints Skipper", 0xEDA2DFED},
            {"Vehicle: Saints Wolverine", 0x83947538},
            {"Vehicle: Julius's Eiswolf", 0x6E885CB6},
            {"Vehicle: Donnie's Voxel", 0x399AF80E},
            {"Vehicle: The General's Bulldog", 0xF825F176},
            {"Vehicle: Maero's Monster Truck", 0x279F86DB},
            {"Vehicle: Akuji's Kaneda", 0xAB0971E0},
            {"Vehicle: Ultor APC", 0x8A076548},
            {"Vehicle: Septic Truck", 0x19A37BEC},
            {"Vehicle: Unique ATV", 0x1704112C},
            {"Vehicle: Unique Bootlegger", 0x99864910},
            {"Vehicle: Unique Phoenix", 0xD55FFBCC},
            {"Vehicle: Race Car Bezier", 0x01A3CB93},
            {"Vehicles: Escort Vehicles (Ethel, Socialite)", 0xC0F5F68E},
            {"Vehicles: Demolition Derby Vehicles", 0xA8BA56CD},
            {"Weapon Cache: Shock Paddles", 0x467CEA09},
            {"Weapon Cache: Pepper Spray", 0x63EB811B},
            {"Weapon Cache: Chainsaw", 0xD0CFAEB0},
            {"Weapon Cache: Hand Grenades", 0x5A4C7D4F},
            {"Weapon Cache: Satchel Charges", 0xB4421C63},
            {"Mechanic Discount (5%)", 0x48B79091},
            {"Mechanic Discount (15%)", 0xDD89989F},
            {"Mechanic Discount (75%)", 0x11616D7F},
            {"Improved Melee Damage (15%)", 0x94C19E54},
            {"Improved Melee Damage (30%)", 0x365BD8AE},
            {"Police Notoriety Reduced (5%)", 0x0C6E2F6B},
            {"Police Notoriety Reduced (15%)", 0x80A55A22},
            {"Samedi Notoriety Reduced (5%)", 0xA3EF186B},
            {"Samedi Notoriety Reduced (15%)", 0x800ADB15},
            {"Brotherhood Notoriety Reduced (5%)", 0x8C4E7260},
            {"Brotherhood Notoriety Reduced (15%)", 0x17F7A3F7},
            {"Ronin Notoriety Reduced (5%)", 0x8ED966F8},
            {"Ronin Notoriety Reduced (15%)", 0xE9212F95},
            {"Reduced Vehicle Damage (5%)", 0x4DE39C90},
            {"Reduced Vehicle Damage (15%)", 0xAA8BFC05},
            {"Reduced Bullet Damage (5%)", 0xD2CCFA18},
            {"Reduced Bullet Damage (15%)", 0x4977D871},
            {"Reduced Explosion Damage (5%)", 0x0ADA00CE},
            {"Reduced Explosion Damage (15%)", 0x261FB96A},
            {"Reduced Fall Damage (100%)", 0xFECFBA7E},
            {"Weapon Store Discount (5%)", 0x545888B6},
            {"Weapon Store Discount (15%)", 0x789FC2EC},
            {"Food Discount (5%)", 0xE499153B},
            {"Food Discount (15%)", 0xEB26FCEC},
            {"Food Discount (100%)", 0x4FD57C8B},
            {"Crib Customization Discount (5%)", 0x1C1F3AEB},
            {"Crib Customization Discount (15%)", 0x6D0DA817},
            {"Melee Combat Style - Ronin", 0xDB63856F},
            {"Melee Combat Style - Brotherhood", 0x57F57254},
            {"Melee Combat Style - Sons of Samedi", 0x884F05F9},
            {"Clothing: Avenger Jacket", 0xF5BB2188},
            {"Clothing: Fireman Outfit", 0xAA7ECC97},
            {"Clothing: Pimp Outfit", 0x81EAACD7},
            {"Clothing: Fire Fighter Suit", 0xAF66178F},
            {"Clothing: Paintball Mask", 0x01625D98},
            {"Clothing: Cowboy Hat", 0x4D865CD3},
            {"Clothing: Traffic Cone Hat", 0x65FEAF23},
            {"Clothing: Zombie Mask", 0x730BD821},
            {"Gang Vehicles: Brotherhood", 0xBE96D761},
            {"Gang Vehicles: Ronin", 0x3200205A},
            {"Gang Vehicles: Sons of Samedi", 0x612CA0CC},
            {"Music Discount (100%)", 0xE931B16D},
            {"Clothing Discount (5%)", 0xE57FF3B5},
            {"Clothing Discount (15%)", 0xE127B42D},
        }};

        const ManagedUnlockable* FindManagedUnlockable(const std::string_view name) {
            const auto found = std::find_if(managedUnlockables.begin(), managedUnlockables.end(),
                                            [name](const auto& entry) { return entry.itemName == name; });
            return found == managedUnlockables.end() ? nullptr : &*found;
        }
    }  // namespace

    struct UnlockableController::Implementation {
        using EnqueueFunction = void(__fastcall*)(void*, int);

        static constexpr std::size_t overwrittenSize{6};
        static constexpr std::uint32_t maximumUnlockables{512};

        Implementation() = default;

        ~Implementation() {
            Remove();
        }

        Implementation(const Implementation&) = delete;
        Implementation& operator=(const Implementation&) = delete;
        Implementation(Implementation&&) = delete;
        Implementation& operator=(Implementation&&) = delete;

        bool Install(bool blockVanilla, const std::vector<std::string>& managedItems) {
            const auto game = InspectSupportedGameModule();
            if (!game) {
                return false;
            }
            processorAddress = game->base + addresses::kUnlockableProcessorRva;
            enqueueAddress = game->base + addresses::kUnlockableEnqueueRva;
            arrayPointerOperand = game->base + addresses::kUnlockableArrayPointerOperandRva;
            countAddress = game->base + addresses::kUnlockableCountRva;
            queueCountAddress = game->base + addresses::kUnlockableQueueCountRva;
            blockVanillaRewards = blockVanilla;
            managedItemNames.clear();
            managedHashes.clear();
            for (const auto& name : managedItems) {
                const auto* definition = FindManagedUnlockable(name);
                if (!definition) {
                    return false;
                }
                managedItemNames.insert(name);
                managedHashes.insert(definition->hash);
            }
            constexpr std::array<std::uint8_t, overwrittenSize> expected{0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8};
            if (DetectDetour(reinterpret_cast<const void*>(processorAddress)) != DetourKind::None ||
                !SafeCopy(reinterpret_cast<const void*>(processorAddress), prologue.data(), prologue.size()) ||
                prologue != expected) {
                return false;
            }

            auto hook =
                safetyhook::InlineHook::create(reinterpret_cast<void*>(processorAddress),
                                               reinterpret_cast<void*>(&Hook), safetyhook::InlineHook::StartDisabled);
            if (!hook) {
                LogError("Unlockables", "SafetyHook could not create the processor hook");
                return false;
            }

            processorHook = std::move(*hook);
            active.store(this, std::memory_order_release);
            if (const auto enabled = processorHook.enable(); !enabled) {
                active.store(nullptr, std::memory_order_release);
                processorHook.reset();
                LogError("Unlockables", "SafetyHook could not enable the processor hook");
                return false;
            }
            installed = true;
            return true;
        }

        void Remove() {
            if (!installed) {
                return;
            }
            if (const auto disabled = processorHook.disable(); !disabled) {
                LogError("Unlockables", "SafetyHook could not disable the processor hook");
                return;
            }
            installed = false;
            active.store(nullptr, std::memory_order_release);
            processorHook.reset();
        }

        bool QueueReceivedItem(const std::string_view itemName) {
            if (!installed) {
                return false;
            }

            const auto* definition = FindManagedUnlockable(itemName);
            if (!definition || managedItemNames.find(std::string{itemName}) == managedItemNames.end()) {
                return false;
            }

            std::uint32_t array{}, count{};
            if (!SafeCopy(reinterpret_cast<const void*>(arrayPointerOperand), &array, sizeof(array)) || !array ||
                !SafeCopy(reinterpret_cast<const void*>(countAddress), &count, sizeof(count)) || count == 0 ||
                count > maximumUnlockables) {
                return false;
            }
            std::uintptr_t record{};
            for (std::uint32_t index = 0; index < count; ++index) {
                const auto candidate = static_cast<std::uintptr_t>(array) +
                                       static_cast<std::uintptr_t>(index) * addresses::kUnlockableRecordSize;
                std::uint32_t hash{};
                if (SafeCopy(reinterpret_cast<const void*>(candidate), &hash, sizeof(hash)) &&
                    hash == definition->hash) {
                    record = candidate;
                    break;
                }
            }
            if (!record) {
                return false;
            }
            {
                std::lock_guard<std::mutex> lock(allowanceMutex);
                ++allowances[definition->hash];
            }
            std::uint32_t queueBefore{}, queueAfter{};
            SafeCopy(reinterpret_cast<const void*>(queueCountAddress), &queueBefore, sizeof(queueBefore));
            reinterpret_cast<EnqueueFunction>(enqueueAddress)(reinterpret_cast<void*>(record), 1);
            SafeCopy(reinterpret_cast<const void*>(queueCountAddress), &queueAfter, sizeof(queueAfter));
            if (queueAfter <= queueBefore) {
                ConsumeAllowance(definition->hash);
                return false;
            }
            LogInfo("Unlockables", "Queued received item: " + std::string{itemName});
            return true;
        }

        bool ConsumeAllowance(std::uint32_t hash) {
            std::lock_guard<std::mutex> lock(allowanceMutex);
            const auto found = allowances.find(hash);
            if (found == allowances.end() || found->second == 0) {
                return false;
            }
            if (--found->second == 0) {
                allowances.erase(found);
            }
            return true;
        }

        bool IsManagedHash(std::uint32_t hash) const {
            return managedHashes.find(hash) != managedHashes.end();
        }

        static void __stdcall Hook(void* const item) {
            auto* const self = active.load(std::memory_order_acquire);
            if (!self || !self->processorHook) {
                return;
            }

            std::uint32_t hash{};
            if (!SafeCopy(item, &hash, sizeof(hash))) {
                self->CallOriginal(item);
                return;
            }

            if (self->IsManagedHash(hash)) {
                if (self->ConsumeAllowance(hash)) {
                    self->CallOriginal(item);
                } else if (!self->blockVanillaRewards) {
                    self->CallOriginal(item);
                } else {
                    LogInfo("Unlockables", "Blocked vanilla unlockable hash=" + std::to_string(hash));
                }
                return;
            }

            self->CallOriginal(item);
        }

        void CallOriginal(void* const item) {
            processorHook.stdcall<void>(item);
        }

        inline static std::atomic<Implementation*> active{};
        std::uintptr_t processorAddress{};
        std::uintptr_t enqueueAddress{};
        std::uintptr_t arrayPointerOperand{};
        std::uintptr_t countAddress{};
        std::uintptr_t queueCountAddress{};
        std::array<std::uint8_t, overwrittenSize> prologue{};
        safetyhook::InlineHook processorHook;
        std::mutex allowanceMutex;
        std::unordered_map<std::uint32_t, std::uint32_t> allowances;
        std::unordered_set<std::string> managedItemNames;
        std::unordered_set<std::uint32_t> managedHashes;
        bool blockVanillaRewards{};
        bool installed{};
    };

    UnlockableController::UnlockableController() = default;

    UnlockableController::~UnlockableController() {
        Remove();
    }

    bool UnlockableController::Install(bool blockVanillaRewards, const std::vector<std::string>& managedItems) {
        if (implementation_) {
            return false;
        }
        implementation_ = std::make_unique<Implementation>();
        if (!implementation_->Install(blockVanillaRewards, managedItems)) {
            implementation_.reset();
            return false;
        }
        return true;
    }

    void UnlockableController::Remove() {
        implementation_.reset();
    }

    bool UnlockableController::QueueReceivedItem(const std::string_view itemName) {
        return implementation_ && implementation_->QueueReceivedItem(itemName);
    }

    bool UnlockableController::SupportsItem(const std::string_view itemName) {
        return FindManagedUnlockable(itemName) != nullptr;
    }
}  // namespace sr2ap
