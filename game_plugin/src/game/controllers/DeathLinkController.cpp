#include "DeathLinkController.hpp"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <utility>

#include "game/DeathLinkActions.hpp"
#include "game/GameThreadDispatcher.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
constexpr std::uint64_t kRetryDelayMs{250};

enum class TaskState : std::uint8_t {
    Running,
    Applied,
    Deferred,
    Dropped,
};

TaskState ToTaskState(const death::Outcome outcome) {
    switch (outcome) {
        case death::Outcome::Applied:
            return TaskState::Applied;
        case death::Outcome::Dropped:
            return TaskState::Dropped;
        case death::Outcome::Deferred:
            break;
    }
    return TaskState::Deferred;
}

struct TaskResult {
    // Claimed by the first Finish; published through `finished`.
    std::atomic<bool> claimed{};
    TaskState state{TaskState::Running};
    const char* detail{};
    std::atomic<bool> finished{};
};

// Held only by the dispatched task: if the dispatcher discards the task
// without running it, the death is retried instead of waiting forever.
struct TaskToken {
    explicit TaskToken(std::shared_ptr<TaskResult> owner)
        : result{std::move(owner)} {
    }

    ~TaskToken() {
        Finish(TaskState::Deferred, "task was discarded");
    }

    TaskToken(const TaskToken&) = delete;
    TaskToken& operator=(const TaskToken&) = delete;

    void Finish(const TaskState state, const char* detail) {
        if (result->claimed.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        result->state = state;
        result->detail = detail;
        result->finished.store(true, std::memory_order_release);
    }

    std::shared_ptr<TaskResult> result;
};

std::string Describe(const DeathLinkController::Death& death) {
    return death.source.empty() ? std::string{"unknown player"} : death.source;
}
}  // namespace

struct DeathLinkController::Implementation {
    const ModuleInfo* game{};
    death::Functions functions;
    std::optional<Death> pending;
    std::shared_ptr<TaskResult> task;
    std::uint64_t retryAt{};
    const char* lastDeferral{};
};

DeathLinkController::DeathLinkController(
    GameThreadDispatcher& dispatcher, std::shared_ptr<GameEventQueue> events,
    std::shared_ptr<DeathSuppression> suppression)
    : dispatcher_{dispatcher},
      events_{std::move(events)},
      suppression_{std::move(suppression)} {
}

DeathLinkController::~DeathLinkController() = default;

bool DeathLinkController::Install() {
    const auto* const game = InspectSupportedGameModule();
    if (!game || !dispatcher_.IsInstalled()) {
        return false;
    }
    const auto functions = death::ResolveFunctions(*game);
    if (!functions) {
        LogError("DeathLink", "Unexpected kill function targets");
        return false;
    }
    implementation_ = std::make_unique<Implementation>();
    implementation_->game = game;
    implementation_->functions = *functions;
    return true;
}

void DeathLinkController::Remove() {
    implementation_.reset();
}

void DeathLinkController::Receive(std::string source, std::string cause) {
    if (!implementation_) {
        LogWarning("DeathLink",
                   "Ignored death from " + source + "; kills are unavailable");
        return;
    }
    auto& pending = implementation_->pending;
    if (pending) {
        LogInfo("DeathLink",
                "Death from " + source + " joins the one already pending");
    }
    pending = Death{std::move(source), std::move(cause)};
}

void DeathLinkController::Reset() {
    if (!implementation_) {
        return;
    }
    auto& self = *implementation_;
    if (self.pending) {
        LogInfo("DeathLink", "Dropped pending death from " +
                                 Describe(*self.pending) +
                                 " because the world changed");
    }
    self.pending.reset();
    self.task.reset();
    self.lastDeferral = nullptr;
}

std::optional<DeathLinkController::Death> DeathLinkController::Update(
    const GameContext& context) {
    if (!implementation_) {
        return std::nullopt;
    }
    auto& self = *implementation_;
    const auto now = GetTickCount64();

    if (self.task) {
        if (!self.task->finished.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        const auto state = self.task->state;
        const char* const detail = self.task->detail;
        self.task.reset();
        if (!self.pending) {
            return std::nullopt;
        }
        switch (state) {
            case TaskState::Applied: {
                LogInfo("DeathLink", "Death from " + Describe(*self.pending) +
                                         ": " + detail);
                auto death = std::move(*self.pending);
                self.pending.reset();
                self.lastDeferral = nullptr;
                return death;
            }
            case TaskState::Dropped:
                LogInfo("DeathLink", "Death from " + Describe(*self.pending) +
                                         " not applied: " + detail);
                self.pending.reset();
                self.lastDeferral = nullptr;
                return std::nullopt;
            default:
                if (detail != self.lastDeferral) {
                    LogDebug("DeathLink",
                             std::string{"Waiting to apply death: "} + detail);
                    self.lastDeferral = detail;
                }
                self.retryAt = now + kRetryDelayMs;
                return std::nullopt;
        }
    }

    if (!self.pending || now < self.retryAt || !context.IsInteractive()) {
        return std::nullopt;
    }

    auto result = std::make_shared<TaskResult>();
    auto token = std::make_shared<TaskToken>(result);
    // A save load bumps the epoch on the game thread, before this thread
    // hears about it.
    const bool queued = dispatcher_.Dispatch(
        [token, events = events_, epoch = events_->ObservedEpoch(),
         game = self.game, functions = self.functions,
         suppression = suppression_] {
            if (!events->IsCurrent(epoch)) {
                token->Finish(TaskState::Dropped, "the world changed");
                return;
            }
            if (!ReadGameContext(game).IsInteractive()) {
                token->Finish(TaskState::Deferred, "gameplay is busy");
                return;
            }
            const auto applied =
                death::KillPlayer(*game, functions, *suppression);
            token->Finish(ToTaskState(applied.outcome), applied.detail);
        });
    if (queued) {
        self.task = std::move(result);
    } else {
        self.retryAt = now + kRetryDelayMs;
    }
    return std::nullopt;
}
}  // namespace sr2ap
