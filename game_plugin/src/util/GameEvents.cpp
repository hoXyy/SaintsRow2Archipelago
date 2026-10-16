#include "GameEvents.hpp"

#include <utility>

namespace sr2ap {
GameEventQueue::GameEventQueue(InteractivePredicate isInteractive)
    : isInteractive_{std::move(isInteractive)} {
}

void GameEventQueue::PushSaveLoaded(const std::uint32_t checksum) {
    std::lock_guard<std::mutex> lock(mutex_);
    liveEpoch_.fetch_add(1, std::memory_order_acq_rel);
    events_.push_back({GameEventKind::SaveLoaded, checksum, 0, false});
}

void GameEventQueue::PushSaveWritten(const std::uint32_t checksum) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back({GameEventKind::SaveWritten, checksum, 0, false});
}

void GameEventQueue::PushRequestCompleted(const std::uint64_t requestId,
                                          const bool applied) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back({GameEventKind::RequestCompleted, 0, requestId, applied});
}

std::vector<GameEvent> GameEventQueue::Drain() {
    std::vector<GameEvent> events;
    std::lock_guard<std::mutex> lock(mutex_);
    events.swap(events_);
    observedEpoch_.store(liveEpoch_.load(std::memory_order_acquire),
                         std::memory_order_release);
    return events;
}

void GameEventQueue::InvalidateWorld() {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto epoch = liveEpoch_.fetch_add(1, std::memory_order_acq_rel) + 1;
    observedEpoch_.store(epoch, std::memory_order_release);
}

std::uint32_t GameEventQueue::ObservedEpoch() const noexcept {
    return observedEpoch_.load(std::memory_order_acquire);
}

bool GameEventQueue::IsCurrent(const std::uint32_t epoch) const noexcept {
    return liveEpoch_.load(std::memory_order_acquire) == epoch;
}

bool GameEventQueue::CanApply(const std::uint32_t epoch) const {
    return IsCurrent(epoch) && (!isInteractive_ || isInteractive_());
}

struct ItemCompletion::State {
    State(std::shared_ptr<GameEventQueue> owner, const std::uint64_t id)
        : queue{std::move(owner)},
          requestId{id},
          epoch{queue->ObservedEpoch()} {
    }

    ~State() {
        try {
            Complete(false);
        } catch (...) {
        }
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;

    void Complete(const bool applied) {
        if (!done.exchange(true, std::memory_order_acq_rel)) {
            queue->PushRequestCompleted(requestId, applied);
        }
    }

    std::shared_ptr<GameEventQueue> queue;
    std::uint64_t requestId{};
    std::uint32_t epoch{};
    std::atomic_bool done{};
};

ItemCompletion::ItemCompletion(std::shared_ptr<GameEventQueue> queue,
                               const std::uint64_t requestId)
    : state_{std::make_shared<State>(std::move(queue), requestId)} {
}

bool ItemCompletion::CanApply() const {
    return state_ && state_->queue->CanApply(state_->epoch);
}

bool ItemCompletion::IsCurrent() const {
    return state_ && state_->queue->IsCurrent(state_->epoch);
}

void ItemCompletion::Complete(const bool applied) const {
    if (state_) {
        state_->Complete(applied);
    }
}
}  // namespace sr2ap
