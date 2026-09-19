#include "HookActivityUnlockController.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <ranges>
#include <safetyhook.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
enum class RaceClass : std::uint32_t {
    Car = 0,
    Bike = 1,
    Plane = 2,
    Helicopter = 3,
    BoatAndJetSki = 4,
};

enum class ActivityUnlockKind {
    Hitman,
    ChopShop,
    Cd,
    Tags,
    Races,
};

struct ActivityUnlockDefinition {
    std::string_view itemName;
    ActivityUnlockKind kind;
    std::optional<RaceClass> raceClass;
};

struct RaceTrigger {
    void* trigger{};
    std::uint32_t raceClass{};
};

constexpr std::uint32_t kRaceClassCount{5};
constexpr std::uint32_t kExpectedRaceCount{27};

constexpr std::uint32_t RaceClassBit(const RaceClass value) noexcept {
    return 1u << static_cast<std::uint32_t>(value);
}

constexpr std::array kActivityUnlocks{
    ActivityUnlockDefinition{"Hitman Activity Unlock Pass",
                             ActivityUnlockKind::Hitman, std::nullopt},
    ActivityUnlockDefinition{"Chop Shop Activity Unlock Pass",
                             ActivityUnlockKind::ChopShop, std::nullopt},
    ActivityUnlockDefinition{"CD Collectible Unlock Pass",
                             ActivityUnlockKind::Cd, std::nullopt},
    ActivityUnlockDefinition{"Tags Collectible Unlock Pass",
                             ActivityUnlockKind::Tags, std::nullopt},
    ActivityUnlockDefinition{"Car Races Activity Unlock Pass",
                             ActivityUnlockKind::Races, RaceClass::Car},
    ActivityUnlockDefinition{"Bike Races Activity Unlock Pass",
                             ActivityUnlockKind::Races, RaceClass::Bike},
    ActivityUnlockDefinition{"Plane Races Activity Unlock Pass",
                             ActivityUnlockKind::Races, RaceClass::Plane},
    ActivityUnlockDefinition{"Helicopter Races Activity Unlock Pass",
                             ActivityUnlockKind::Races, RaceClass::Helicopter},
    ActivityUnlockDefinition{"Boat Races Activity Unlock Pass",
                             ActivityUnlockKind::Races,
                             RaceClass::BoatAndJetSki},
};

constexpr std::array<std::uint8_t, 6> kExpectedChopShop{
    std::uint8_t{0x51}, 0x53, 0x8B, 0x5C, 0x24, 0x0C,
};
constexpr std::array<std::uint8_t, 9> kExpectedHitman{
    std::uint8_t{0x53}, 0x55, 0x56, 0x57, 0xBD, 0x1C, 0x12, 0xEA, 0x00,
};
constexpr std::array<std::uint8_t, 7> kExpectedCd{
    std::uint8_t{0x56}, 0x8B, 0x35, 0x90, 0xD7, 0x15, 0x02,
};
constexpr std::array<std::uint8_t, 10> kExpectedTag{
    std::uint8_t{0x51}, 0x53, 0x56, 0x57, 0x8B, 0x3D, 0xB4, 0x3F, 0x7A, 0x02,
};
constexpr std::array<std::uint8_t, 10> kExpectedRace{
    std::uint8_t{0x53}, 0x8A, 0x5C, 0x24, 0x08, 0x55, 0x56, 0x57, 0x33, 0xFF,
};

const ActivityUnlockDefinition* FindUnlock(
    const std::string_view name) noexcept {
    const auto found = std::ranges::find_if(
        kActivityUnlocks,
        [name](const auto& definition) { return definition.itemName == name; });
    return found == kActivityUnlocks.end() ? nullptr : &*found;
}

template <std::size_t Size>
bool CreateManagedHook(const bool managed, safetyhook::InlineHook& destination,
                       const std::uintptr_t address,
                       const std::array<std::uint8_t, Size>& expected,
                       void* const callback, const std::string_view name) {
    if (!managed) {
        return true;
    }
    if (!ValidateHookTarget(address, expected)) {
        LogError("WorldUnlock", std::string{name} + " hook target mismatch");
        return false;
    }

    auto hook = safetyhook::InlineHook::create(
        reinterpret_cast<void*>(address), callback,
        safetyhook::InlineHook::StartDisabled);
    if (!hook) {
        LogError("WorldUnlock",
                 "Could not create " + std::string{name} + " hook");
        return false;
    }
    destination = std::move(*hook);
    return true;
}

bool EnableHook(safetyhook::InlineHook& hook, const std::string_view name) {
    if (!hook) {
        return true;
    }
    if (!hook.enable()) {
        LogError("WorldUnlock",
                 "Could not enable " + std::string{name} + " hook");
        return false;
    }
    return true;
}

bool DisableHook(safetyhook::InlineHook& hook, const std::string_view name) {
    if (!hook || !hook.enabled()) {
        return true;
    }
    if (!hook.disable()) {
        LogError("WorldUnlock",
                 "Could not disable " + std::string{name} + " hook");
        return false;
    }
    return true;
}
}  // namespace

struct HookActivityUnlockController::Implementation {
    Implementation() = default;

    ~Implementation() {
        Remove();
    }

    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

    bool Install(const std::span<const std::string> managedItems) {
        if (installed || managedItems.empty()) {
            return false;
        }

        const auto game = InspectSupportedGameModule();
        if (!game) {
            LogError("WorldUnlock", "Unsupported executable");
            return false;
        }
        if (!ConfigureManagedItems(managedItems)) {
            return false;
        }

        gameModule = *game;
        ResolveAddresses(gameModule.base);
        if (!ValidateSupportingTargets() || !CreateManagedHooks()) {
            ResetHookObjects();
            return false;
        }

        Implementation* expected{};
        if (!active.compare_exchange_strong(expected, this,
                                            std::memory_order_acq_rel)) {
            LogError("WorldUnlock",
                     "Another world unlock controller is already active");
            ResetHookObjects();
            return false;
        }

        if (!EnableManagedHooks()) {
            const bool rolledBack = DisableEnabledHooks();
            if (rolledBack) {
                active.store(nullptr, std::memory_order_release);
                ResetHookObjects();
            } else {
                LogError("WorldUnlock",
                         "Could not roll back partially enabled hooks");
            }
            return false;
        }

        installed = true;
        return true;
    }

    bool InitializePolicy() {
        if (!installed) {
            return false;
        }

        // Session policies may be installed while the game is still at the
        // main menu. Do not call native gameplay functions until the normal
        // readiness checks say the world is interactive.
        if (!ReadGameContext(&gameModule).IsInteractive()) {
            return false;
        }

        const auto freeroam = IsFreeroam();
        if (!freeroam) {
            LogWarning("WorldUnlock",
                       "Could not read the current gameplay instance");
            return false;
        }

        bool storyUnlocked{};
        if (*freeroam) {
            const auto unlocked = IsStoryUnlocked();
            if (!unlocked) {
                LogWarning("WorldUnlock", "Could not read the story gate");
                return false;
            }
            storyUnlocked = *unlocked;
        }

        nativeHitmanEnabled = storyUnlocked;
        nativeChopShopEnabled = storyUnlocked;
        nativeRacesEnabled = storyUnlocked;
        nativeCdsEnabled = true;
        nativeTagsEnabled = true;

        ClearOwnership();
        ApplyCurrentPolicy();
        initialized = true;
        return true;
    }

    bool SupportsItem(const std::string_view itemName) const noexcept {
        const auto* const definition = FindUnlock(itemName);
        return definition && IsManaged(*definition);
    }

    bool Grant(const std::string_view itemName) {
        if (!installed || !initialized) {
            return false;
        }

        const auto* const definition = FindUnlock(itemName);
        if (!definition || !IsManaged(*definition)) {
            return false;
        }

        switch (definition->kind) {
            case ActivityUnlockKind::Hitman:
                ownsHitman = true;
                ApplyHitmanAfterReceipt();
                break;
            case ActivityUnlockKind::ChopShop:
                ownsChopShop = true;
                ApplyChopShopAfterReceipt();
                break;
            case ActivityUnlockKind::Cd:
                ownsCds = true;
                if (nativeCdsEnabled) {
                    cdHook.fastcall<void>(std::uint8_t{1});
                }
                break;
            case ActivityUnlockKind::Tags:
                ownsTags = true;
                if (nativeTagsEnabled) {
                    tagHook.ccall<void>(std::uint8_t{1});
                }
                break;
            case ActivityUnlockKind::Races:
                GrantRaceClass(*definition->raceClass);
                break;
        }
        return true;
    }

    void ResetPersistentState() {
        if (!installed || !initialized) {
            return;
        }
        ClearOwnership();
        ApplyCurrentPolicy();
    }

    void Remove() {
        if (!installed && !AnyHookEnabled()) {
            ResetHookObjects();
            return;
        }

        if (!DisableEnabledHooks()) {
            LogError("WorldUnlock",
                     "Could not disable every world unlock hook");
            return;
        }

        installed = false;
        initialized = false;
        active.store(nullptr, std::memory_order_release);
        ResetHookObjects();
        ClearOwnership();
    }

   private:
    bool ConfigureManagedItems(
        const std::span<const std::string> managedItems) {
        for (const auto& itemName : managedItems) {
            const auto* const definition = FindUnlock(itemName);
            if (!definition) {
                LogError("WorldUnlock", "Unknown persistent item: " + itemName);
                return false;
            }

            switch (definition->kind) {
                case ActivityUnlockKind::Hitman:
                    managedHitman = true;
                    break;
                case ActivityUnlockKind::ChopShop:
                    managedChopShop = true;
                    break;
                case ActivityUnlockKind::Cd:
                    managedCds = true;
                    break;
                case ActivityUnlockKind::Tags:
                    managedTags = true;
                    break;
                case ActivityUnlockKind::Races:
                    managedRaceClasses |= RaceClassBit(*definition->raceClass);
                    break;
            }
        }
        return true;
    }

    bool IsManaged(const ActivityUnlockDefinition& definition) const noexcept {
        switch (definition.kind) {
            case ActivityUnlockKind::Hitman:
                return managedHitman;
            case ActivityUnlockKind::ChopShop:
                return managedChopShop;
            case ActivityUnlockKind::Cd:
                return managedCds;
            case ActivityUnlockKind::Tags:
                return managedTags;
            case ActivityUnlockKind::Races:
                return definition.raceClass &&
                       (managedRaceClasses &
                        RaceClassBit(*definition.raceClass)) != 0;
        }
        return false;
    }

    void ResolveAddresses(const std::uintptr_t base) noexcept {
        gameBase = base;
        hitmanAddress = base + addresses::kHitmanSetAllEnabledRva;
        chopShopAddress = base + addresses::kChopShopSetAllEnabledRva;
        cdAddress = base + addresses::kCdSetAllPickupsEnabledRva;
        tagAddress = base + addresses::kTagSetAllSpotsEnabledRva;
        raceAddress = base + addresses::kRaceToggleAllRva;
        triggerEnableAddress = base + addresses::kTriggerEnableRva;
        storyLockedAddress = base + addresses::kDiversionsAreStoryLockedRva;
        gameplayInstanceAddress = base + addresses::kCurrentGameplayInstanceRva;
    }

    bool ValidateSupportingTargets() const {
        if ((managedHitman || managedChopShop || managedRaceClasses != 0) &&
            !IsExecutableAddress(storyLockedAddress)) {
            LogError("WorldUnlock", "story gate function is unavailable");
            return false;
        }
        if (managedRaceClasses != 0 &&
            !IsExecutableAddress(triggerEnableAddress)) {
            LogError("WorldUnlock", "race trigger function is unavailable");
            return false;
        }
        return true;
    }

    bool CreateManagedHooks() {
        return CreateManagedHook(
                   managedHitman, hitmanHook, hitmanAddress, kExpectedHitman,
                   reinterpret_cast<void*>(&HitmanHook), "Hitman") &&
               CreateManagedHook(managedChopShop, chopShopHook, chopShopAddress,
                                 kExpectedChopShop,
                                 reinterpret_cast<void*>(&ChopShopHook),
                                 "Chop Shop") &&
               CreateManagedHook(managedCds, cdHook, cdAddress, kExpectedCd,
                                 reinterpret_cast<void*>(&CdHook), "CD") &&
               CreateManagedHook(managedTags, tagHook, tagAddress, kExpectedTag,
                                 reinterpret_cast<void*>(&TagHook), "tag") &&
               CreateManagedHook(managedRaceClasses != 0, raceHook, raceAddress,
                                 kExpectedRace,
                                 reinterpret_cast<void*>(&RaceHook), "race");
    }

    bool EnableManagedHooks() {
        return EnableHook(hitmanHook, "Hitman") &&
               EnableHook(chopShopHook, "Chop Shop") &&
               EnableHook(cdHook, "CD") && EnableHook(tagHook, "tag") &&
               EnableHook(raceHook, "race");
    }

    bool DisableEnabledHooks() {
        bool success{true};
        success = DisableHook(raceHook, "race") && success;
        success = DisableHook(tagHook, "tag") && success;
        success = DisableHook(cdHook, "CD") && success;
        success = DisableHook(chopShopHook, "Chop Shop") && success;
        success = DisableHook(hitmanHook, "Hitman") && success;
        return success;
    }

    bool AnyHookEnabled() const noexcept {
        return hitmanHook.enabled() || chopShopHook.enabled() ||
               cdHook.enabled() || tagHook.enabled() || raceHook.enabled();
    }

    void ResetHookObjects() {
        raceHook.reset();
        tagHook.reset();
        cdHook.reset();
        chopShopHook.reset();
        hitmanHook.reset();
    }

    std::optional<bool> IsFreeroam() const {
        const auto current =
            ReadMemory<std::uintptr_t>(gameplayInstanceAddress);
        if (!current) {
            return std::nullopt;
        }
        return *current == 0;
    }

    std::optional<bool> IsStoryUnlocked() const {
        if (!IsExecutableAddress(storyLockedAddress)) {
            return std::nullopt;
        }
        using Function = int(__cdecl*)();
        const auto function = reinterpret_cast<Function>(storyLockedAddress);
        return function() == 0;
    }

    void ClearOwnership() noexcept {
        ownsHitman = false;
        ownsChopShop = false;
        ownsCds = false;
        ownsTags = false;
        ownedRaceClasses = 0;
    }

    void ApplyCurrentPolicy() {
        if (managedHitman) {
            hitmanHook.stdcall<void>(
                static_cast<std::uint8_t>(nativeHitmanEnabled && ownsHitman));
        }
        if (managedChopShop) {
            chopShopHook.stdcall<void>(static_cast<std::uint8_t>(
                nativeChopShopEnabled && ownsChopShop));
        }
        if (managedCds) {
            cdHook.fastcall<void>(
                static_cast<std::uint8_t>(nativeCdsEnabled && ownsCds));
        }
        if (managedTags) {
            tagHook.ccall<void>(
                static_cast<std::uint8_t>(nativeTagsEnabled && ownsTags));
        }
        if (managedRaceClasses != 0) {
            ApplyRaceVisibility();
        }
    }

    void ApplyHitmanAfterReceipt() {
        const auto freeroam = IsFreeroam();
        if (!nativeHitmanEnabled || !freeroam || !*freeroam) {
            return;
        }
        const auto storyUnlocked = IsStoryUnlocked();
        if (storyUnlocked && *storyUnlocked) {
            hitmanHook.stdcall<void>(std::uint8_t{1});
        }
    }

    void ApplyChopShopAfterReceipt() {
        const auto freeroam = IsFreeroam();
        if (!nativeChopShopEnabled || !freeroam || !*freeroam) {
            return;
        }
        const auto storyUnlocked = IsStoryUnlocked();
        if (storyUnlocked && *storyUnlocked) {
            chopShopHook.stdcall<void>(std::uint8_t{1});
        }
    }

    void GrantRaceClass(const RaceClass raceClass) {
        ownedRaceClasses |= RaceClassBit(raceClass);
        const auto freeroam = IsFreeroam();
        if (!nativeRacesEnabled || !freeroam || !*freeroam) {
            return;
        }
        const auto storyUnlocked = IsStoryUnlocked();
        if (storyUnlocked && *storyUnlocked) {
            ApplyRaceVisibility();
        }
    }

    bool IsRaceClassEnabled(const std::uint32_t raceClass) const noexcept {
        if (raceClass >= kRaceClassCount) {
            return false;
        }
        const auto bit = 1u << raceClass;
        return (managedRaceClasses & bit) == 0 || (ownedRaceClasses & bit) != 0;
    }

    std::optional<std::vector<RaceTrigger>> ReadRaceTriggers() const {
        const auto count = ReadMemory<std::uint32_t>(
            gameBase + addresses::kRacingRecordCountRva);
        if (!count || *count != kExpectedRaceCount) {
            return std::nullopt;
        }

        std::vector<RaceTrigger> triggers;
        triggers.reserve(*count);
        for (std::uint32_t index = 0; index < *count; ++index) {
            const auto record = gameBase + addresses::kRacingRecordTableRva +
                                static_cast<std::uintptr_t>(index) *
                                    addresses::kRacingRecordStride;
            const auto raceClass = ReadMemory<std::uint32_t>(
                record + addresses::kRacingRecordClassOffset);
            const auto trigger = ReadMemory<std::uintptr_t>(
                record + addresses::kRacingRecordStartTriggerOffset);
            if (!raceClass || !trigger || *raceClass >= kRaceClassCount) {
                return std::nullopt;
            }
            triggers.push_back({reinterpret_cast<void*>(*trigger), *raceClass});
        }
        return triggers;
    }

    void ApplyRaceVisibility() {
        const auto triggers = ReadRaceTriggers();
        if (!triggers) {
            raceHook.ccall<void>(std::uint8_t{0});
            LogWarning("WorldUnlock", "Invalid race trigger table");
            return;
        }

        raceHook.ccall<void>(std::uint8_t{0});
        if (!nativeRacesEnabled) {
            return;
        }

        for (const auto& entry : *triggers) {
            if (entry.trigger && IsRaceClassEnabled(entry.raceClass)) {
                EnableRaceTrigger(triggerEnableAddress, entry.trigger);
            }
        }
    }

    static void EnableRaceTrigger(const std::uintptr_t function,
                                  void* const trigger) {
        if (!trigger) {
            return;
        }

        __asm {
            push esi
            mov esi, trigger
            mov eax, function
            call eax
            pop esi
        }
    }

    static void __stdcall HitmanHook(const std::uint8_t requested) {
        auto* const self = active.load(std::memory_order_acquire);
        if (!self || !self->hitmanHook) {
            return;
        }
        self->nativeHitmanEnabled = requested != 0;
        const bool permitted = !self->managedHitman || self->ownsHitman;
        self->hitmanHook.stdcall<void>(
            static_cast<std::uint8_t>(requested != 0 && permitted));
    }

    static void __stdcall ChopShopHook(const std::uint8_t requested) {
        auto* const self = active.load(std::memory_order_acquire);
        if (!self || !self->chopShopHook) {
            return;
        }
        self->nativeChopShopEnabled = requested != 0;
        const bool permitted = !self->managedChopShop || self->ownsChopShop;
        self->chopShopHook.stdcall<void>(
            static_cast<std::uint8_t>(requested != 0 && permitted));
    }

    static void __fastcall CdHook(const std::uint8_t requested) {
        auto* const self = active.load(std::memory_order_acquire);
        if (!self || !self->cdHook) {
            return;
        }
        self->nativeCdsEnabled = requested != 0;
        const bool permitted = !self->managedCds || self->ownsCds;
        self->cdHook.fastcall<void>(
            static_cast<std::uint8_t>(requested != 0 && permitted));
    }

    static void __cdecl TagHook(const std::uint8_t requested) {
        auto* const self = active.load(std::memory_order_acquire);
        if (!self || !self->tagHook) {
            return;
        }
        self->nativeTagsEnabled = requested != 0;
        const bool permitted = !self->managedTags || self->ownsTags;
        self->tagHook.ccall<void>(
            static_cast<std::uint8_t>(requested != 0 && permitted));
    }

    static void __cdecl RaceHook(const std::uint8_t requested) {
        auto* const self = active.load(std::memory_order_acquire);
        if (!self || !self->raceHook) {
            return;
        }
        self->nativeRacesEnabled = requested != 0;
        if (!requested) {
            self->raceHook.ccall<void>(std::uint8_t{0});
            return;
        }
        self->ApplyRaceVisibility();
    }

    bool managedHitman{};
    bool managedChopShop{};
    bool managedCds{};
    bool managedTags{};
    std::uint32_t managedRaceClasses{};

    bool ownsHitman{};
    bool ownsChopShop{};
    bool ownsCds{};
    bool ownsTags{};
    std::uint32_t ownedRaceClasses{};

    bool nativeHitmanEnabled{};
    bool nativeChopShopEnabled{};
    bool nativeCdsEnabled{true};
    bool nativeTagsEnabled{true};
    bool nativeRacesEnabled{};

    std::uintptr_t gameBase{};
    ModuleInfo gameModule;
    std::uintptr_t hitmanAddress{};
    std::uintptr_t chopShopAddress{};
    std::uintptr_t cdAddress{};
    std::uintptr_t tagAddress{};
    std::uintptr_t raceAddress{};
    std::uintptr_t triggerEnableAddress{};
    std::uintptr_t storyLockedAddress{};
    std::uintptr_t gameplayInstanceAddress{};

    safetyhook::InlineHook hitmanHook;
    safetyhook::InlineHook chopShopHook;
    safetyhook::InlineHook cdHook;
    safetyhook::InlineHook tagHook;
    safetyhook::InlineHook raceHook;

    bool installed{};
    bool initialized{};

    inline static std::atomic<Implementation*> active{};
};

HookActivityUnlockController::HookActivityUnlockController() = default;

HookActivityUnlockController::~HookActivityUnlockController() {
    Remove();
}

bool HookActivityUnlockController::Install(
    const std::span<const std::string> managedItems) {
    if (implementation_) {
        return false;
    }

    auto implementation = std::make_unique<Implementation>();
    if (!implementation->Install(managedItems)) {
        return false;
    }
    implementation_ = std::move(implementation);
    return true;
}

bool HookActivityUnlockController::InitializePolicy() {
    return implementation_ && implementation_->InitializePolicy();
}

bool HookActivityUnlockController::SupportsItem(
    const std::string_view itemName) const noexcept {
    return implementation_ && implementation_->SupportsItem(itemName);
}

bool HookActivityUnlockController::Grant(const std::string_view itemName) {
    return implementation_ && implementation_->Grant(itemName);
}

void HookActivityUnlockController::ResetPersistentState() {
    if (implementation_) {
        implementation_->ResetPersistentState();
    }
}

void HookActivityUnlockController::Remove() {
    if (!implementation_) {
        return;
    }
    implementation_->Remove();
    implementation_.reset();
}

}  // namespace sr2ap
