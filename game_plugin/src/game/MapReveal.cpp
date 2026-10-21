#include "MapReveal.hpp"

#include <windows.h>

#include <array>
#include <cstdint>
#include <mutex>
#include <string>

#include "Addresses.hpp"
#include "GameThreadDispatcher.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
constexpr ULONGLONG kSweepIntervalMs = 1000;
// The stock city has 53 neighborhoods.
constexpr std::int32_t kMaximumNeighborhoods = 256;
// Guards against a corrupt list; the city holds a few thousand triggers.
constexpr std::size_t kMaximumTriggers = 16384;

void RevealMapIcon(const std::uintptr_t function, const std::int32_t list,
                   const std::uint32_t icon) {
    __asm {
            push edi
            mov eax, list
            mov edi, icon
            call function
            pop edi
    }
}
}  // namespace

struct MapReveal::Implementation {
    // Game thread.
    void Sweep() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!installed) {
            return;
        }
        const auto hoods = RevealNeighborhoods();
        const auto icons = RevealTriggers();
        if (hoods || icons) {
            LogDebug("MapReveal", "Revealed hoods=" + std::to_string(hoods) +
                                      " triggers=" + std::to_string(icons));
        }
    }

    std::uint32_t RevealNeighborhoods() const {
        const auto count =
            ReadMemory<std::int32_t>(base + addresses::kNeighborhoodCountRva);
        const auto table =
            ReadMemory<std::uintptr_t>(base + addresses::kNeighborhoodTableRva);
        if (!count || !table || *count <= 0 || *count > kMaximumNeighborhoods ||
            *table == 0) {
            return 0;
        }

        std::uint32_t revealed{};
        for (std::int32_t index = 0; index < *count; ++index) {
            const auto address = *table +
                                 static_cast<std::uintptr_t>(index) *
                                     addresses::kNeighborhoodStride +
                                 addresses::kNeighborhoodFlagsOffset;
            const auto flags = ReadMemory<std::uint8_t>(address);
            if (!flags ||
                (*flags & (addresses::kNeighborhoodExploredFlag |
                           addresses::kNeighborhoodUnexplorableFlag)) != 0) {
                continue;
            }
            // What the game's own explore function does, minus the stat.
            *reinterpret_cast<volatile std::uint8_t*>(address) =
                *flags | addresses::kNeighborhoodExploredFlag;
            ++revealed;
        }
        return revealed;
    }

    std::uint32_t RevealTriggers() const {
        const auto head =
            ReadMemory<std::uintptr_t>(base + addresses::kActiveTriggerHeadRva);
        if (!head || *head == 0) {
            return 0;
        }

        std::uint32_t revealed{};
        auto trigger = *head;
        for (std::size_t visited = 0; visited < kMaximumTriggers; ++visited) {
            if (RevealTrigger(trigger)) {
                ++revealed;
            }
            const auto next = ReadMemory<std::uintptr_t>(
                trigger + addresses::kTriggerNextOffset);
            if (!next || *next == 0 || *next == *head) {
                return revealed;
            }
            trigger = *next;
        }
        LogWarning("MapReveal", "Trigger list did not end; sweep stopped");
        return revealed;
    }

    // Marks a trigger with a map icon discovered and reveals its icon, as the
    // game does for shops flagged explored_at_start.
    bool RevealTrigger(const std::uintptr_t trigger) const {
        const auto type =
            ReadMemory<std::uint32_t>(trigger + addresses::kTriggerTypeOffset);
        const auto iconType = ReadMemory<std::int32_t>(
            trigger + addresses::kTriggerMapIconTypeOffset);
        const auto list = ReadMemory<std::int32_t>(
            trigger + addresses::kTriggerMapIconListOffset);
        const auto flags =
            ReadMemory<std::uint8_t>(trigger + addresses::kTriggerFlagsOffset);
        if (!type || !iconType || !list || !flags ||
            *type == addresses::kTriggerTypeUniqueJump || *iconType == -1 ||
            *list < 0 || *list >= addresses::kMapIconListCount) {
            return false;
        }

        const bool discovered =
            (*flags & addresses::kTriggerDiscoveredFlag) != 0;
        if (!discovered) {
            *reinterpret_cast<volatile std::uint8_t*>(
                trigger + addresses::kTriggerFlagsOffset) =
                *flags | addresses::kTriggerDiscoveredFlag;
        }
        // Zero while the trigger is disabled; enabling it creates an icon
        // that is already revealed.
        const auto icon = ReadMemory<std::uint32_t>(
            trigger + addresses::kTriggerMapIconOffset);
        if (icon && *icon != 0) {
            RevealMapIcon(revealAddress, *list, *icon);
        }
        return !discovered;
    }

    std::mutex mutex;
    std::uintptr_t base{};
    std::uintptr_t revealAddress{};
    bool installed{};
};

MapReveal::MapReveal(GameThreadDispatcher& dispatcher)
    : dispatcher_{dispatcher},
      implementation_{std::make_shared<Implementation>()} {
}

MapReveal::~MapReveal() {
    Remove();
}

bool MapReveal::Install() {
    const auto game = InspectSupportedGameModule();
    if (!game) {
        return false;
    }
    const auto revealAddress = game->base + addresses::kMapIconRevealRva;
    constexpr std::array<std::uint8_t, 8> expectedReveal{
        0x83, 0xF8, 0xFF, 0x74, 0x32, 0x83, 0xF8, 0x08,
    };
    if (!ValidateHookTarget(*game, revealAddress, expectedReveal)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(implementation_->mutex);
    implementation_->base = game->base;
    implementation_->revealAddress = revealAddress;
    implementation_->installed = true;
    return true;
}

void MapReveal::Remove() {
    std::lock_guard<std::mutex> lock(implementation_->mutex);
    implementation_->installed = false;
}

void MapReveal::Update(const GameContext& context) {
    if (!context.IsInteractive()) {
        return;
    }
    const auto now = GetTickCount64();
    if (now < nextSweep_) {
        return;
    }
    nextSweep_ = now + kSweepIntervalMs;
    dispatcher_.Dispatch([state = implementation_] { state->Sweep(); });
}

}  // namespace sr2ap
