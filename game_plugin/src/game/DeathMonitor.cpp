#include "DeathMonitor.hpp"

#include <windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <safetyhook.hpp>
#include <string>
#include <utility>

#include "Addresses.hpp"
#include "DeathLinkActions.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"
#include "util/HandlerActivity.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
constexpr std::array<std::uint8_t, 9> kHumanDieEntry{
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x83, 0xEC, 0x34,
};
// Long enough to skip the instant reset in Demolition Derby and races, where
// the player is alive again on the next frame.
constexpr std::uint64_t kRearmAfterAliveMs{1'000};
}  // namespace

struct DeathMonitor::Implementation {
    Implementation() = default;

    ~Implementation() {
        Remove();
    }

    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

    bool Install(std::shared_ptr<DeathSuppression> deathSuppression,
                 Callback onPlayerDied) {
        game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }
        const auto target = game->base + addresses::kHumanDieRva;
        if (!ValidateHookTarget(*game, target, kHumanDieEntry)) {
            LogError("DeathLink", "Unexpected human death routine");
            return false;
        }
        suppression = std::move(deathSuppression);
        playerDied = std::move(onPlayerDied);

        auto pendingHook = safetyhook::MidHook::create(
            reinterpret_cast<void*>(target), &HumanDieHook,
            safetyhook::MidHook::StartDisabled);
        if (!pendingHook) {
            LogError("DeathLink", "Could not create human death hook");
            return false;
        }
        hook = std::move(*pendingHook);
        hookActivity.Start();
        active.store(this, std::memory_order_release);
        if (const auto enabled = hook.enable(); !enabled) {
            LogError("DeathLink", "Could not enable human death hook");
            Remove();
            return false;
        }
        return true;
    }

    void Poll(const GameContext& context) {
        const auto failures =
            callbackFailures.exchange(0, std::memory_order_acq_rel);
        if (failures != 0) {
            LogError("DeathLink", "Exceptions suppressed inside death hook=" +
                                      std::to_string(failures));
        }
        const auto suppressed =
            suppressedDeaths.exchange(0, std::memory_order_acq_rel);
        if (suppressed != 0) {
            LogInfo("DeathLink",
                    "Not sending player death caused by DeathLink count=" +
                        std::to_string(suppressed));
        }

        const auto now = GetTickCount64();
        suppression->Expire(now);
        if (armed.load(std::memory_order_acquire)) {
            return;
        }

        const auto vitals =
            context.IsLoaded() ? death::ReadPlayerVitals(*game) : std::nullopt;
        if (!vitals || !vitals->IsAlive()) {
            aliveSince = 0;
            return;
        }
        if (aliveSince == 0) {
            aliveSince = now;
        } else if (now - aliveSince >= kRearmAfterAliveMs) {
            armed.store(true, std::memory_order_release);
            aliveSince = 0;
            LogDebug("DeathLink", "Player alive; watching for death");
        }
    }

    void Reset() {
        armed.store(false, std::memory_order_release);
        aliveSince = 0;
        suppression->End();
    }

    void Remove() {
        active.store(nullptr, std::memory_order_release);
        if (hook && hook.enabled()) {
            if (const auto disabled = hook.disable(); !disabled) {
                LogError("DeathLink", "Could not disable human death hook");
            }
        }
        hookActivity.Stop();
        while (!hookActivity.IsIdle()) {
            SwitchToThread();
        }
        hook.reset();
    }

   private:
    static void HumanDieHook(safetyhook::Context& context) noexcept {
        auto hookLease = hookActivity.Acquire();
        auto* const self = active.load(std::memory_order_acquire);
        if (!hookLease || !self) {
            return;
        }

        // At the routine's entry [esp] is the return address and [esp + 4]
        // the dying human. NPCs die through here too.
        std::uint32_t human{};
        const auto player = ReadMemory<std::uint32_t>(
            self->game->base + addresses::kPlayerGlobalRva);
        if (!SafeCopy(reinterpret_cast<const void*>(context.esp + 4), &human,
                      sizeof(human)) ||
            human == 0 || !player || human != *player) {
            return;
        }

        const bool wasArmed =
            self->armed.exchange(false, std::memory_order_acq_rel);
        if (self->suppression->Consume(GetTickCount64())) {
            self->suppressedDeaths.fetch_add(1, std::memory_order_release);
            return;
        }
        if (!wasArmed) {
            return;
        }
        try {
            if (self->playerDied) {
                self->playerDied();
            }
        } catch (...) {
            self->callbackFailures.fetch_add(1, std::memory_order_release);
        }
    }

    inline static std::atomic<Implementation*> active{};
    inline static HandlerActivity hookActivity;
    const ModuleInfo* game{};
    std::shared_ptr<DeathSuppression> suppression;
    Callback playerDied;
    safetyhook::MidHook hook;
    std::atomic<bool> armed{};
    std::uint64_t aliveSince{};
    std::atomic<std::uint32_t> callbackFailures;
    std::atomic<std::uint32_t> suppressedDeaths;
};

DeathMonitor::DeathMonitor() = default;
DeathMonitor::~DeathMonitor() = default;

bool DeathMonitor::Install(std::shared_ptr<DeathSuppression> suppression,
                           Callback playerDied) {
    implementation_ = std::make_unique<Implementation>();
    if (!implementation_->Install(std::move(suppression),
                                  std::move(playerDied))) {
        implementation_.reset();
        return false;
    }
    return true;
}

void DeathMonitor::Poll(const GameContext& context) {
    if (implementation_) {
        implementation_->Poll(context);
    }
}

void DeathMonitor::Reset() {
    if (implementation_) {
        implementation_->Reset();
    }
}

void DeathMonitor::Remove() {
    implementation_.reset();
}
}  // namespace sr2ap
