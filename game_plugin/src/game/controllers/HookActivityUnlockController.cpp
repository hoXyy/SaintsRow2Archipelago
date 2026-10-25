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
    StuntJumps,
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

constexpr std::uint32_t KindBit(const ActivityUnlockKind kind) noexcept {
    return 1u << static_cast<std::uint32_t>(kind);
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
    ActivityUnlockDefinition{"Stunt Jumps Unlock Pass",
                             ActivityUnlockKind::StuntJumps, std::nullopt},
};

constexpr std::array<std::uint8_t, 9> kExpectedHitman{
    0x53, 0x55, 0x56, 0x57, 0xBD, 0x1C, 0x12, 0xEA, 0x00};
constexpr std::array<std::uint8_t, 7> kExpectedHitmanManager{
    0x83, 0xEC, 0x08, 0x53, 0x55, 0x56, 0x57};
constexpr std::array<std::uint8_t, 12> kExpectedChopShopTriggers{
    0x51, 0x80, 0x7C, 0x24, 0x0C, 0x00, 0x53, 0x55, 0x8B, 0x6C, 0x24, 0x10};
constexpr std::array<std::uint8_t, 13> kExpectedChopShopManager{
    0x53, 0x8B, 0x5C, 0x24, 0x08, 0x56, 0x8B,
    0xF1, 0x53, 0x56, 0x88, 0x5E, 0x14};
constexpr std::array<std::uint8_t, 6> kExpectedChopShop{0x51, 0x53, 0x8B,
                                                        0x5C, 0x24, 0x0C};
constexpr std::array<std::uint8_t, 7> kExpectedCd{0x56, 0x8B, 0x35, 0x90,
                                                  0xD7, 0x15, 0x02};
constexpr std::array<std::uint8_t, 10> kExpectedTag{
    0x51, 0x53, 0x56, 0x57, 0x8B, 0x3D, 0xB4, 0x3F, 0x7A, 0x02};
constexpr std::array<std::uint8_t, 10> kExpectedRace{
    0x53, 0x8A, 0x5C, 0x24, 0x08, 0x55, 0x56, 0x57, 0x33, 0xFF};
// Prologue through the instructions the gate hook replaces.
constexpr std::array<std::uint8_t, 34> kExpectedStuntJumpArm{
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x83, 0xEC, 0x4C, 0x53, 0x56, 0x8B,
    0x75, 0x08, 0x80, 0x7E, 0x14, 0x00, 0x57, 0x0F, 0x84, 0xF9, 0x01, 0x00,
    0x00, 0x8B, 0x48, 0x44, 0x8B, 0x3D, 0xD4, 0x03, 0x17, 0x02};
constexpr std::array<std::uint8_t, 9> kExpectedStuntJumpArmExit{
    0x5F, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D, 0xC2, 0x08, 0x00};

enum HookIndex : std::size_t {
    kHitmanHook,
    kHitmanManagerHook,
    kChopShopTriggersHook,
    kChopShopManagerHook,
    kChopShopHook,
    kCdHook,
    kTagHook,
    kRaceHook,
    kHookCount,
};

struct HookDefinition {
    std::string_view name;
    ActivityUnlockKind kind;
    std::ptrdiff_t rva;
    std::span<const std::uint8_t> expected;
    void* callback;
};

const ActivityUnlockDefinition* FindUnlock(
    const std::string_view name) noexcept {
    const auto found = std::ranges::find_if(
        kActivityUnlocks,
        [name](const auto& definition) { return definition.itemName == name; });
    return found == kActivityUnlocks.end() ? nullptr : &*found;
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

        // Don't do anything until the game is actually in-game
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
        nativeHitmanAvailable = storyUnlocked;
        nativeChopShopAvailable = storyUnlocked;
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

        if (definition->kind == ActivityUnlockKind::Races) {
            GrantRaceClass(*definition->raceClass);
            return true;
        }

        ownedKinds |= KindBit(definition->kind);
        switch (definition->kind) {
            case ActivityUnlockKind::Hitman:
                if (nativeHitmanEnabled && nativeHitmanAvailable &&
                    StoryActivitiesOpen()) {
                    hooks[kHitmanManagerHook].thiscall<void>(hitmanManager,
                                                             std::uint8_t{1});
                    hooks[kHitmanHook].stdcall<void>(std::uint8_t{1});
                }
                break;
            case ActivityUnlockKind::ChopShop:
                if (nativeChopShopEnabled && nativeChopShopAvailable &&
                    StoryActivitiesOpen()) {
                    hooks[kChopShopManagerHook].thiscall<void>(chopShopManager,
                                                               std::uint8_t{1});
                    hooks[kChopShopHook].stdcall<void>(std::uint8_t{1});
                }
                break;
            case ActivityUnlockKind::Cd:
                if (nativeCdsEnabled) {
                    hooks[kCdHook].fastcall<void>(std::uint8_t{1});
                }
                break;
            case ActivityUnlockKind::Tags:
                if (nativeTagsEnabled) {
                    hooks[kTagHook].ccall<void>(std::uint8_t{1});
                }
                break;
            case ActivityUnlockKind::Races:
            case ActivityUnlockKind::StuntJumps:
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

    void RefreshCollectiblePolicy() {
        if (!installed || !initialized ||
            !ReadGameContext(&gameModule).IsInteractive()) {
            return;
        }
        ApplyCollectiblePolicy();
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
    static const std::array<HookDefinition, kHookCount>& HookDefinitions() {
        static const std::array<HookDefinition, kHookCount> definitions{{
            {"Hitman", ActivityUnlockKind::Hitman,
             addresses::kHitmanSetAllEnabledRva, kExpectedHitman,
             reinterpret_cast<void*>(&HitmanHook)},
            {"Hitman manager", ActivityUnlockKind::Hitman,
             addresses::kHitmanSetManagerAvailableRva, kExpectedHitmanManager,
             reinterpret_cast<void*>(&HitmanManagerHook)},
            {"Chop Shop triggers", ActivityUnlockKind::ChopShop,
             addresses::kChopShopSetStartTriggersEnabledRva,
             kExpectedChopShopTriggers,
             reinterpret_cast<void*>(&ChopShopTriggersHook)},
            {"Chop Shop manager", ActivityUnlockKind::ChopShop,
             addresses::kChopShopSetManagerAvailableRva,
             kExpectedChopShopManager,
             reinterpret_cast<void*>(&ChopShopManagerHook)},
            {"Chop Shop", ActivityUnlockKind::ChopShop,
             addresses::kChopShopSetAllEnabledRva, kExpectedChopShop,
             reinterpret_cast<void*>(&ChopShopHook)},
            {"CD", ActivityUnlockKind::Cd,
             addresses::kCdSetAllPickupsEnabledRva, kExpectedCd,
             reinterpret_cast<void*>(&CdHook)},
            {"tag", ActivityUnlockKind::Tags,
             addresses::kTagSetAllSpotsEnabledRva, kExpectedTag,
             reinterpret_cast<void*>(&TagHook)},
            {"race", ActivityUnlockKind::Races, addresses::kRaceToggleAllRva,
             kExpectedRace, reinterpret_cast<void*>(&RaceHook)},
        }};
        return definitions;
    }

    bool ConfigureManagedItems(
        const std::span<const std::string> managedItems) {
        for (const auto& itemName : managedItems) {
            const auto* const definition = FindUnlock(itemName);
            if (!definition) {
                LogError("WorldUnlock", "Unknown persistent item: " + itemName);
                return false;
            }

            if (definition->kind == ActivityUnlockKind::Races) {
                managedRaceClasses |= RaceClassBit(*definition->raceClass);
            } else {
                managedKinds |= KindBit(definition->kind);
            }
        }
        return true;
    }

    bool IsManaged(const ActivityUnlockDefinition& definition) const noexcept {
        if (definition.kind == ActivityUnlockKind::Races) {
            return definition.raceClass &&
                   (managedRaceClasses & RaceClassBit(*definition.raceClass)) !=
                       0;
        }
        return IsKindManaged(definition.kind);
    }

    bool IsKindManaged(const ActivityUnlockKind kind) const noexcept {
        if (kind == ActivityUnlockKind::Races) {
            return managedRaceClasses != 0;
        }
        return (managedKinds & KindBit(kind)) != 0;
    }

    // Unmanaged activities are always permitted; managed ones once received.
    bool Permits(const ActivityUnlockKind kind) const noexcept {
        return !IsKindManaged(kind) || (ownedKinds & KindBit(kind)) != 0;
    }

    void ResolveAddresses(const std::uintptr_t base) noexcept {
        gameBase = base;
        hitmanManager =
            reinterpret_cast<void*>(base + addresses::kHitmanManagerRva);
        chopShopManager =
            reinterpret_cast<void*>(base + addresses::kChopShopManagerRva);
        triggerEnableAddress = base + addresses::kTriggerEnableRva;
        storyLockedAddress = base + addresses::kDiversionsAreStoryLockedRva;
        gameplayInstanceAddress = base + addresses::kCurrentGameplayInstanceRva;
    }

    bool ValidateSupportingTargets() const {
        if ((IsKindManaged(ActivityUnlockKind::Hitman) ||
             IsKindManaged(ActivityUnlockKind::ChopShop) ||
             IsKindManaged(ActivityUnlockKind::Races)) &&
            !IsExecutableAddress(storyLockedAddress)) {
            LogError("WorldUnlock", "story gate function is unavailable");
            return false;
        }
        if (IsKindManaged(ActivityUnlockKind::Races) &&
            !IsExecutableAddress(triggerEnableAddress)) {
            LogError("WorldUnlock", "race trigger function is unavailable");
            return false;
        }
        return true;
    }

    bool CreateManagedHooks() {
        const auto& definitions = HookDefinitions();
        for (std::size_t index = 0; index < kHookCount; ++index) {
            const auto& definition = definitions[index];
            if (!IsKindManaged(definition.kind)) {
                continue;
            }

            const auto address = gameBase + definition.rva;
            if (!ValidateHookTarget(gameModule, address, definition.expected)) {
                LogError("WorldUnlock", std::string{definition.name} +
                                            " hook target mismatch");
                return false;
            }

            auto hook = safetyhook::InlineHook::create(
                reinterpret_cast<void*>(address), definition.callback,
                safetyhook::InlineHook::StartDisabled);
            if (!hook) {
                LogError("WorldUnlock", "Could not create " +
                                            std::string{definition.name} +
                                            " hook");
                return false;
            }
            hooks[index] = std::move(*hook);
        }
        return !IsKindManaged(ActivityUnlockKind::StuntJumps) ||
               CreateStuntJumpHook();
    }

    // Jumps are always enabled natively, so instead of toggling triggers
    // the hook refuses to arm one until the unlock is received.
    bool CreateStuntJumpHook() {
        const auto function = gameBase + addresses::kUniqueJumpTryArmRva;
        const auto exit = function + addresses::kUniqueJumpTryArmExitOffset;
        if (!ValidateHookTarget(gameModule, function, kExpectedStuntJumpArm) ||
            !ValidatePatchSite(gameModule, exit, kExpectedStuntJumpArmExit)) {
            LogError("WorldUnlock", "stunt jump hook target mismatch");
            return false;
        }

        auto hook = safetyhook::MidHook::create(
            reinterpret_cast<void*>(function +
                                    addresses::kUniqueJumpTryArmGateOffset),
            &StuntJumpArmHook, safetyhook::MidHook::StartDisabled);
        if (!hook) {
            LogError("WorldUnlock", "Could not create stunt jump hook");
            return false;
        }
        stuntJumpHook = std::move(*hook);
        stuntJumpArmExit = exit;
        return true;
    }

    bool EnableManagedHooks() {
        const auto& definitions = HookDefinitions();
        for (std::size_t index = 0; index < kHookCount; ++index) {
            if (hooks[index] && !hooks[index].enable()) {
                LogError("WorldUnlock",
                         "Could not enable " +
                             std::string{definitions[index].name} + " hook");
                return false;
            }
        }
        if (stuntJumpHook && !stuntJumpHook.enable()) {
            LogError("WorldUnlock", "Could not enable stunt jump hook");
            return false;
        }
        return true;
    }

    bool DisableEnabledHooks() {
        const auto& definitions = HookDefinitions();
        bool success{true};
        if (stuntJumpHook && stuntJumpHook.enabled() &&
            !stuntJumpHook.disable()) {
            LogError("WorldUnlock", "Could not disable stunt jump hook");
            success = false;
        }
        for (std::size_t index = kHookCount; index-- > 0;) {
            auto& hook = hooks[index];
            if (hook && hook.enabled() && !hook.disable()) {
                LogError("WorldUnlock",
                         "Could not disable " +
                             std::string{definitions[index].name} + " hook");
                success = false;
            }
        }
        return success;
    }

    bool AnyHookEnabled() const noexcept {
        return stuntJumpHook.enabled() ||
               std::ranges::any_of(
                   hooks, [](const auto& hook) { return hook.enabled(); });
    }

    void ResetHookObjects() {
        stuntJumpHook.reset();
        for (auto hook = hooks.rbegin(); hook != hooks.rend(); ++hook) {
            hook->reset();
        }
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

    // True when the player is in freeroam and the story no longer locks
    // diversions, so a received unlock can be shown immediately.
    bool StoryActivitiesOpen() const {
        const auto freeroam = IsFreeroam();
        if (!freeroam || !*freeroam) {
            return false;
        }
        const auto storyUnlocked = IsStoryUnlocked();
        return storyUnlocked && *storyUnlocked;
    }

    void ClearOwnership() noexcept {
        ownedKinds = 0;
        ownedRaceClasses = 0;
    }

    void ApplyCurrentPolicy() {
        if (IsKindManaged(ActivityUnlockKind::Hitman)) {
            const bool owned = Permits(ActivityUnlockKind::Hitman);
            hooks[kHitmanManagerHook].thiscall<void>(
                hitmanManager,
                static_cast<std::uint8_t>(nativeHitmanAvailable && owned));
            hooks[kHitmanHook].stdcall<void>(
                static_cast<std::uint8_t>(nativeHitmanEnabled && owned));
        }
        if (IsKindManaged(ActivityUnlockKind::ChopShop)) {
            const bool owned = Permits(ActivityUnlockKind::ChopShop);
            hooks[kChopShopManagerHook].thiscall<void>(
                chopShopManager,
                static_cast<std::uint8_t>(nativeChopShopAvailable && owned));
            hooks[kChopShopHook].stdcall<void>(
                static_cast<std::uint8_t>(nativeChopShopEnabled && owned));
        }
        ApplyCollectiblePolicy();
        if (IsKindManaged(ActivityUnlockKind::Races)) {
            ApplyRaceVisibility();
        }
    }

    void ApplyCollectiblePolicy() {
        if (IsKindManaged(ActivityUnlockKind::Cd)) {
            hooks[kCdHook].fastcall<void>(static_cast<std::uint8_t>(
                nativeCdsEnabled && Permits(ActivityUnlockKind::Cd)));
        }
        if (IsKindManaged(ActivityUnlockKind::Tags)) {
            hooks[kTagHook].ccall<void>(static_cast<std::uint8_t>(
                nativeTagsEnabled && Permits(ActivityUnlockKind::Tags)));
        }
    }

    void GrantRaceClass(const RaceClass raceClass) {
        ownedRaceClasses |= RaceClassBit(raceClass);
        if (nativeRacesEnabled && StoryActivitiesOpen()) {
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
            hooks[kRaceHook].ccall<void>(std::uint8_t{0});
            LogWarning("WorldUnlock", "Invalid race trigger table");
            return;
        }

        hooks[kRaceHook].ccall<void>(std::uint8_t{0});
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

    // The active controller, or null when it is gone or has no `index` hook.
    static Implementation* ActiveFor(const HookIndex index) noexcept {
        auto* const self = active.load(std::memory_order_acquire);
        return self && self->hooks[index] ? self : nullptr;
    }

    // What the original function receives: the game's request, unless the
    // activity is managed and not yet received.
    std::uint8_t Gate(const ActivityUnlockKind kind,
                      const std::uint8_t requested) const noexcept {
        return static_cast<std::uint8_t>(requested != 0 && Permits(kind));
    }

    // Gate, remembering the game's request so it can be re-applied once the
    // unlock is received.
    std::uint8_t Gate(const ActivityUnlockKind kind, bool& native,
                      const std::uint8_t requested) noexcept {
        native = requested != 0;
        return Gate(kind, requested);
    }

    static void __stdcall HitmanHook(const std::uint8_t requested) {
        if (auto* const self = ActiveFor(kHitmanHook)) {
            self->hooks[kHitmanHook].stdcall<void>(
                self->Gate(ActivityUnlockKind::Hitman,
                           self->nativeHitmanEnabled, requested));
        }
    }

    static void __fastcall HitmanManagerHook(void* const manager, void*,
                                             const std::uint8_t requested) {
        if (auto* const self = ActiveFor(kHitmanManagerHook)) {
            self->hooks[kHitmanManagerHook].thiscall<void>(
                manager, self->Gate(ActivityUnlockKind::Hitman,
                                    self->nativeHitmanAvailable, requested));
        }
    }

    static void __stdcall ChopShopHook(const std::uint8_t requested) {
        if (auto* const self = ActiveFor(kChopShopHook)) {
            self->hooks[kChopShopHook].stdcall<void>(
                self->Gate(ActivityUnlockKind::ChopShop,
                           self->nativeChopShopEnabled, requested));
        }
    }

    static void __stdcall ChopShopTriggersHook(void* const manager,
                                               const std::uint8_t requested) {
        if (auto* const self = ActiveFor(kChopShopTriggersHook)) {
            self->hooks[kChopShopTriggersHook].stdcall<void>(
                manager, self->Gate(ActivityUnlockKind::ChopShop, requested));
        }
    }

    static void __fastcall ChopShopManagerHook(void* const manager, void*,
                                               const std::uint8_t requested) {
        if (auto* const self = ActiveFor(kChopShopManagerHook)) {
            self->hooks[kChopShopManagerHook].thiscall<void>(
                manager, self->Gate(ActivityUnlockKind::ChopShop,
                                    self->nativeChopShopAvailable, requested));
        }
    }

    static void __fastcall CdHook(const std::uint8_t requested) {
        if (auto* const self = ActiveFor(kCdHook)) {
            self->hooks[kCdHook].fastcall<void>(self->Gate(
                ActivityUnlockKind::Cd, self->nativeCdsEnabled, requested));
        }
    }

    static void __cdecl TagHook(const std::uint8_t requested) {
        if (auto* const self = ActiveFor(kTagHook)) {
            self->hooks[kTagHook].ccall<void>(self->Gate(
                ActivityUnlockKind::Tags, self->nativeTagsEnabled, requested));
        }
    }

    static void __cdecl RaceHook(const std::uint8_t requested) {
        auto* const self = ActiveFor(kRaceHook);
        if (!self) {
            return;
        }
        self->nativeRacesEnabled = requested != 0;
        if (!requested) {
            self->hooks[kRaceHook].ccall<void>(std::uint8_t{0});
            return;
        }
        self->ApplyRaceVisibility();
    }

    static void StuntJumpArmHook(safetyhook::Context& context) {
        auto* const self = active.load(std::memory_order_acquire);
        if (self && self->stuntJumpHook &&
            !self->Permits(ActivityUnlockKind::StuntJumps)) {
            context.eip = self->stuntJumpArmExit;
        }
    }

    // KindBit masks for every kind except Races.
    std::uint32_t managedKinds{};
    std::uint32_t ownedKinds{};
    std::uint32_t managedRaceClasses{};
    std::uint32_t ownedRaceClasses{};

    bool nativeHitmanEnabled{};
    bool nativeChopShopEnabled{};
    bool nativeHitmanAvailable{};
    bool nativeChopShopAvailable{};
    bool nativeCdsEnabled{true};
    bool nativeTagsEnabled{true};
    bool nativeRacesEnabled{};

    std::uintptr_t gameBase{};
    ModuleInfo gameModule;
    void* hitmanManager{};
    void* chopShopManager{};
    std::uintptr_t triggerEnableAddress{};
    std::uintptr_t storyLockedAddress{};
    std::uintptr_t gameplayInstanceAddress{};

    std::array<safetyhook::InlineHook, kHookCount> hooks;
    safetyhook::MidHook stuntJumpHook;
    std::uintptr_t stuntJumpArmExit{};

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

void HookActivityUnlockController::RefreshCollectiblePolicy() {
    if (implementation_) {
        implementation_->RefreshCollectiblePolicy();
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
