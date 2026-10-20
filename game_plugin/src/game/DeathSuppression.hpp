#pragma once

#include <atomic>
#include <cstdint>

namespace sr2ap {
// Keeps deaths the plugin causes for DeathLink from being sent back out.
// Shared by the death hook and the DeathLink kill, both on the game thread,
// and expired by the plugin thread.
class DeathSuppression {
   public:
    // Suppresses the next player death until the GetTickCount64 deadline.
    void Begin(std::uint64_t untilTick) noexcept {
        consumed_.store(false, std::memory_order_relaxed);
        until_.store(untilTick, std::memory_order_release);
    }

    // Death hook: true when this death was caused by the plugin.
    [[nodiscard]] bool Consume(std::uint64_t nowTick) noexcept {
        const auto until = until_.exchange(0, std::memory_order_acq_rel);
        if (until == 0 || nowTick > until) {
            return false;
        }
        consumed_.store(true, std::memory_order_release);
        return true;
    }

    // Stops suppressing; true when a death was suppressed since Begin.
    bool End() noexcept {
        until_.store(0, std::memory_order_release);
        return consumed_.exchange(false, std::memory_order_acq_rel);
    }

    void Expire(std::uint64_t nowTick) noexcept {
        auto until = until_.load(std::memory_order_acquire);
        if (until != 0 && nowTick > until) {
            until_.compare_exchange_strong(until, 0, std::memory_order_acq_rel);
        }
    }

   private:
    std::atomic<std::uint64_t> until_{};
    std::atomic<bool> consumed_{};
};
}  // namespace sr2ap
