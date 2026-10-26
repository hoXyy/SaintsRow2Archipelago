#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace sr2ap {
enum class GameEventKind {
    SaveLoaded,
    SaveWritten,
    RequestCompleted,
    PlayerDied,
    ListOpened,
    JobLevelCompleted
};

enum class ActivityList {
    Hitman,
    ChopShop
};

enum class Job {
    Paramedic,
    Firefighter,
    Taxi,
    Towing,
    Hoing
};

struct GameEvent {
    GameEventKind kind{};
    std::uint32_t checksum{};
    std::uint64_t requestId{};
    bool applied{};
    ActivityList activityList{};
    // One-based list number.
    std::uint32_t list{};
    Job job{};
    // One-based level number.
    std::uint32_t level{};
};

class GameEventQueue {
   public:
    using InteractivePredicate = std::function<bool()>;

    explicit GameEventQueue(InteractivePredicate isInteractive = {});

    // Any thread.
    void PushSaveLoaded(std::uint32_t checksum);
    void PushSaveWritten(std::uint32_t checksum);
    void PushRequestCompleted(std::uint64_t requestId, bool applied);
    void PushPlayerDied();
    void PushListOpened(ActivityList activityList, std::uint32_t list);
    void PushJobLevelCompleted(Job job, std::uint32_t level);

    // Plugin thread.
    [[nodiscard]] std::vector<GameEvent> Drain();
    void InvalidateWorld();
    [[nodiscard]] std::uint32_t ObservedEpoch() const noexcept;

    // Game thread.
    [[nodiscard]] bool IsCurrent(std::uint32_t epoch) const noexcept;
    [[nodiscard]] bool CanApply(std::uint32_t epoch) const;

   private:
    InteractivePredicate isInteractive_;
    std::mutex mutex_;
    std::vector<GameEvent> events_;
    std::atomic<std::uint32_t> liveEpoch_{};
    std::atomic<std::uint32_t> observedEpoch_{};
};

class ItemCompletion {
   public:
    ItemCompletion() = default;
    ItemCompletion(std::shared_ptr<GameEventQueue> queue,
                   std::uint64_t requestId);

    [[nodiscard]] bool CanApply() const;
    [[nodiscard]] bool IsCurrent() const;
    void Complete(bool applied) const;

   private:
    struct State;
    std::shared_ptr<State> state_;
};
}  // namespace sr2ap
