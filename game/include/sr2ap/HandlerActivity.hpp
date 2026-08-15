#pragma once

#include <atomic>

namespace sr2ap {
    class HandlerActivity {
       public:
        class Lease {
           public:
            Lease() = default;

            explicit Lease(HandlerActivity& owner) noexcept : owner_{&owner} {
            }

            ~Lease() {
                if (owner_) {
                    owner_->active_.fetch_sub(1, std::memory_order_release);
                }
            }

            Lease(const Lease&) = delete;
            Lease& operator=(const Lease&) = delete;

            Lease(Lease&& other) noexcept : owner_{other.owner_} {
                other.owner_ = nullptr;
            }

            explicit operator bool() const noexcept {
                return owner_ != nullptr;
            }

           private:
            HandlerActivity* owner_{};
        };

        [[nodiscard]] Lease Acquire() noexcept {
            if (!accepting_.load(std::memory_order_acquire)) {
                return {};
            }

            active_.fetch_add(1, std::memory_order_acquire);
            if (!accepting_.load(std::memory_order_acquire)) {
                active_.fetch_sub(1, std::memory_order_release);
                return {};
            }
            return Lease{*this};
        }

        void Stop() noexcept {
            accepting_.store(false, std::memory_order_release);
        }

        void Start() noexcept {
            accepting_.store(true, std::memory_order_release);
        }

        [[nodiscard]] bool IsIdle() const noexcept {
            return active_.load(std::memory_order_acquire) == 0;
        }

       private:
        std::atomic<unsigned> active_{};
        std::atomic<bool> accepting_{true};
    };
}  // namespace sr2ap
