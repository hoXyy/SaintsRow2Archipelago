#include "ActivityListMonitor.hpp"

#include <windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <safetyhook.hpp>
#include <string>
#include <utility>

#include "Addresses.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"
#include "util/HandlerActivity.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
// cmp edi, ebx; jz (null descriptor)
constexpr std::array<std::uint8_t, 8> kHitmanListSelected{
    0x3B, 0xFB, 0x0F, 0x84, 0x9C, 0x02, 0x00, 0x00,
};
// cmp byte ptr [0x02528482], 1
constexpr std::array<std::uint8_t, 7> kChopShopListSelected{
    0x80, 0x3D, 0x82, 0x84, 0x52, 0x02, 0x01,
};

// One-based index of `descriptor` in an array of `count` descriptors.
std::optional<std::uint32_t> ListNumber(const std::uintptr_t descriptor,
                                        const std::uintptr_t first,
                                        const std::size_t stride,
                                        const std::uint32_t count) {
    if (descriptor < first || (descriptor - first) % stride != 0) {
        return std::nullopt;
    }
    const auto index = (descriptor - first) / stride;
    if (index >= count) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(index + 1);
}
}  // namespace

struct ActivityListMonitor::Implementation {
    Implementation() = default;

    ~Implementation() {
        Remove();
    }

    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

    bool Install(Callback onListOpened) {
        game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }
        const auto hitmanTarget =
            game->base + addresses::kHitmanListSelectedRva;
        const auto chopShopTarget =
            game->base + addresses::kChopShopListSelectedRva;
        if (!ValidatePatchSite(*game, hitmanTarget, kHitmanListSelected) ||
            !ValidatePatchSite(*game, chopShopTarget, kChopShopListSelected)) {
            LogError("Lists", "Unexpected list menu population code");
            return false;
        }
        listOpened = std::move(onListOpened);

        auto pendingHitman = safetyhook::MidHook::create(
            reinterpret_cast<void*>(hitmanTarget), &HitmanListHook,
            safetyhook::MidHook::StartDisabled);
        auto pendingChopShop = safetyhook::MidHook::create(
            reinterpret_cast<void*>(chopShopTarget), &ChopShopListHook,
            safetyhook::MidHook::StartDisabled);
        if (!pendingHitman || !pendingChopShop) {
            LogError("Lists", "Could not create list menu hooks");
            return false;
        }
        hitmanHook = std::move(*pendingHitman);
        chopShopHook = std::move(*pendingChopShop);
        hookActivity.Start();
        active.store(this, std::memory_order_release);
        if (!hitmanHook.enable() || !chopShopHook.enable()) {
            LogError("Lists", "Could not enable list menu hooks");
            Remove();
            return false;
        }
        return true;
    }

    void Poll() {
        const auto failures =
            callbackFailures.exchange(0, std::memory_order_acq_rel);
        if (failures != 0) {
            LogError("Lists", "Exceptions suppressed inside list hooks=" +
                                  std::to_string(failures));
        }
    }

    void Remove() {
        active.store(nullptr, std::memory_order_release);
        for (auto* hook : {&hitmanHook, &chopShopHook}) {
            if (*hook && hook->enabled()) {
                if (const auto disabled = hook->disable(); !disabled) {
                    LogError("Lists", "Could not disable list menu hook");
                }
            }
        }
        hookActivity.Stop();
        while (!hookActivity.IsIdle()) {
            SwitchToThread();
        }
        hitmanHook.reset();
        chopShopHook.reset();
    }

   private:
    static void HitmanListHook(safetyhook::Context& context) noexcept {
        auto hookLease = hookActivity.Acquire();
        auto* const self = active.load(std::memory_order_acquire);
        if (!hookLease || !self) {
            return;
        }

        const auto descriptor = context.edi;
        const auto list = ListNumber(
            descriptor,
            self->game->base + addresses::kHitmanListTableCandidateRva,
            addresses::kHitmanListDescriptorStride,
            addresses::kHitmanListCount);
        if (!list) {
            return;
        }
        // The pause menu can show a stale list during some activities, so
        // check it was started.
        const auto started = ReadMemory<std::uint8_t>(
            descriptor + addresses::kHitmanListActiveOffset);
        if (started && *started != 0) {
            self->Report(ActivityList::Hitman, *list);
        }
    }

    static void ChopShopListHook(safetyhook::Context& context) noexcept {
        auto hookLease = hookActivity.Acquire();
        auto* const self = active.load(std::memory_order_acquire);
        if (!hookLease || !self) {
            return;
        }

        const auto root = ReadMemory<std::uint32_t>(
            self->game->base + addresses::kChopShopRootGlobalRva);
        if (!root || *root == 0) {
            return;
        }
        const auto count = ReadMemory<std::uint32_t>(
            *root + addresses::kChopShopListCountOffset);
        if (!count) {
            return;
        }
        const auto descriptor = context.esi;
        const auto list =
            ListNumber(descriptor, *root + addresses::kChopShopDescriptorOffset,
                       addresses::kChopShopDescriptorStride, *count);
        if (!list) {
            return;
        }
        const auto accepted = ReadMemory<std::uint8_t>(
            descriptor + addresses::kChopShopListAcceptedOffset);
        if (accepted && (*accepted & 1) != 0) {
            self->Report(ActivityList::ChopShop, *list);
        }
    }

    void Report(const ActivityList activityList,
                const std::uint32_t list) noexcept {
        try {
            if (listOpened) {
                listOpened(activityList, list);
            }
        } catch (...) {
            callbackFailures.fetch_add(1, std::memory_order_release);
        }
    }

    inline static std::atomic<Implementation*> active{};
    inline static HandlerActivity hookActivity;
    const ModuleInfo* game{};
    Callback listOpened;
    safetyhook::MidHook hitmanHook;
    safetyhook::MidHook chopShopHook;
    std::atomic<std::uint32_t> callbackFailures;
};

ActivityListMonitor::ActivityListMonitor() = default;
ActivityListMonitor::~ActivityListMonitor() = default;

bool ActivityListMonitor::Install(Callback listOpened) {
    implementation_ = std::make_unique<Implementation>();
    if (!implementation_->Install(std::move(listOpened))) {
        implementation_.reset();
        return false;
    }
    return true;
}

void ActivityListMonitor::Poll() {
    if (implementation_) {
        implementation_->Poll();
    }
}

void ActivityListMonitor::Remove() {
    implementation_.reset();
}
}  // namespace sr2ap
