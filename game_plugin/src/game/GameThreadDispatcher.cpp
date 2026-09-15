#include "GameThreadDispatcher.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <safetyhook.hpp>
#include <utility>
#include <vector>

#include "Addresses.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {

struct GameThreadDispatcher::Implementation {
    Implementation() = default;

    ~Implementation() {
        Remove();
    }

    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

    bool Install() {
        const auto game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }

        const auto frameDispatchAddress =
            game->base + addresses::kGameFrameDispatchSiteRva;
        constexpr std::array<std::uint8_t, 7> expectedFrameDispatch{
            0x83, 0x3D, 0x24, 0x8B, 0x52, 0x02, 0x00,
        };
        const auto actualFrameDispatch =
            ReadMemoryIntoArray<std::uint8_t, expectedFrameDispatch.size()>(
                frameDispatchAddress);

        if (!IsInsideModule(game->handle, reinterpret_cast<const void*>(
                                              frameDispatchAddress)) ||
            !IsExecutableAddress(frameDispatchAddress) ||
            DetectDetour(reinterpret_cast<const void*>(frameDispatchAddress)) !=
                DetourKind::None ||
            !actualFrameDispatch ||
            *actualFrameDispatch != expectedFrameDispatch) {
            return false;
        }

        auto hook = safetyhook::MidHook::create(
            reinterpret_cast<void*>(frameDispatchAddress), &FrameHook,
            safetyhook::MidHook::StartDisabled);
        if (!hook) {
            return false;
        }

        frameHook = std::move(*hook);
        active.store(this, std::memory_order_release);
        if (const auto enabled = frameHook.enable(); !enabled) {
            active.store(nullptr, std::memory_order_release);
            frameHook.reset();
            return false;
        }

        installed = true;
        return true;
    }

    void Remove() {
        if (frameHook) {
            if (const auto disabled = frameHook.disable(); !disabled) {
                LogError("GameThreadDispatcher",
                         "Failed to disable game-thread dispatch hook");
            }
            Deactivate();
            frameHook.reset();
        } else {
            Deactivate();
        }

        std::lock_guard<std::mutex> lock(pendingMutex);
        pendingTasks.clear();
        installed = false;
    }

    bool Dispatch(Task task) {
        if (!task) {
            return false;
        }

        std::lock_guard<std::mutex> lock(pendingMutex);
        if (!installed) {
            return false;
        }

        pendingTasks.push_back(std::move(task));
        return true;
    }

   private:
    void Deactivate() noexcept {
        auto* expected = this;
        active.compare_exchange_strong(expected, nullptr,
                                       std::memory_order_acq_rel);
    }

    void DrainPendingTasks() {
        std::vector<Task> tasks;
        {
            std::lock_guard<std::mutex> lock(pendingMutex);
            tasks.swap(pendingTasks);
        }
        for (const auto& task : tasks) {
            task();
        }
    }

    static void FrameHook(safetyhook::Context&) {
        auto* const self = active.load(std::memory_order_acquire);
        if (self) {
            self->DrainPendingTasks();
        }
    }

    safetyhook::MidHook frameHook;
    std::mutex pendingMutex;
    std::vector<Task> pendingTasks;
    bool installed{};
    inline static std::atomic<Implementation*> active{};
};

GameThreadDispatcher::GameThreadDispatcher() = default;

GameThreadDispatcher::~GameThreadDispatcher() {
    Remove();
}

bool GameThreadDispatcher::Install() {
    if (implementation_) {
        return false;
    }

    implementation_ = std::make_unique<Implementation>();
    if (!implementation_->Install()) {
        implementation_.reset();
        return false;
    }

    return true;
}

void GameThreadDispatcher::Remove() {
    implementation_.reset();
}

bool GameThreadDispatcher::Dispatch(Task task) {
    return implementation_ && implementation_->Dispatch(std::move(task));
}

}  // namespace sr2ap
