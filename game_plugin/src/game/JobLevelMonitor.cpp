#include "JobLevelMonitor.hpp"

#include <windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <safetyhook.hpp>
#include <span>
#include <string>
#include <utility>

#include "Addresses.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"
#include "util/HandlerActivity.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
// cmp ecx, [taxi level count]
constexpr std::array<std::uint8_t, 6> kTaxiLevelCompleted{
    0x3B, 0x0D, 0xE0, 0x78, 0x52, 0x02,
};
// cmp eax, [paramedic level count]
constexpr std::array<std::uint8_t, 6> kParamedicLevelCompleted{
    0x3B, 0x05, 0x50, 0xE9, 0xE9, 0x00,
};
// mov ecx, [ebp+0x1C]; push eax; push ecx
constexpr std::array<std::uint8_t, 5> kFirefighterLevelCompleted{
    0x8B, 0x4D, 0x1C, 0x50, 0x51,
};
// mov ecx, [ebp+0x14]; mov eax, [towing level count]
constexpr std::array<std::uint8_t, 8> kTowingLevelCompleted{
    0x8B, 0x4D, 0x14, 0xA1, 0x94, 0x40, 0x7A, 0x02,
};
// mov eax, [ebx+0x748]
constexpr std::array<std::uint8_t, 6> kHoingLevelCompleted{
    0x8B, 0x83, 0x48, 0x07, 0x00, 0x00,
};

struct HookSite {
    std::ptrdiff_t rva;
    std::span<const std::uint8_t> expected;
    safetyhook::MidHookFn callback;
};
}  // namespace

struct JobLevelMonitor::Implementation {
    Implementation() = default;

    ~Implementation() {
        Remove();
    }

    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

    bool Install(Callback onLevelCompleted) {
        game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }
        const std::array<HookSite, kHookCount> sites{{
            {addresses::kTaxiLevelCompletedRva, kTaxiLevelCompleted, &TaxiHook},
            {addresses::kParamedicLevelCompletedRva, kParamedicLevelCompleted,
             &ParamedicHook},
            {addresses::kFirefighterLevelCompletedRva,
             kFirefighterLevelCompleted, &FirefighterHook},
            {addresses::kTowingLevelCompletedRva, kTowingLevelCompleted,
             &TowingHook},
            {addresses::kHoingLevelCompletedRva, kHoingLevelCompleted,
             &HoingHook},
        }};
        for (const auto& site : sites) {
            if (!ValidatePatchSite(*game, game->base + site.rva,
                                   site.expected)) {
                LogError("Jobs", "Unexpected job level completion code");
                return false;
            }
        }
        levelCompleted = std::move(onLevelCompleted);

        for (std::size_t index = 0; index < sites.size(); ++index) {
            auto pending = safetyhook::MidHook::create(
                reinterpret_cast<void*>(game->base + sites[index].rva),
                sites[index].callback, safetyhook::MidHook::StartDisabled);
            if (!pending) {
                LogError("Jobs", "Could not create job level hooks");
                return false;
            }
            hooks[index] = std::move(*pending);
        }
        hookActivity.Start();
        active.store(this, std::memory_order_release);
        for (auto& hook : hooks) {
            if (!hook.enable()) {
                LogError("Jobs", "Could not enable job level hooks");
                Remove();
                return false;
            }
        }
        return true;
    }

    void Poll() {
        const auto failures =
            callbackFailures.exchange(0, std::memory_order_acq_rel);
        if (failures != 0) {
            LogError("Jobs", "Exceptions suppressed inside job level hooks=" +
                                 std::to_string(failures));
        }
    }

    void Remove() {
        active.store(nullptr, std::memory_order_release);
        for (auto& hook : hooks) {
            if (hook && hook.enabled()) {
                if (const auto disabled = hook.disable(); !disabled) {
                    LogError("Jobs", "Could not disable job level hook");
                }
            }
        }
        hookActivity.Stop();
        while (!hookActivity.IsIdle()) {
            SwitchToThread();
        }
        for (auto& hook : hooks) {
            hook.reset();
        }
    }

   private:
    static constexpr std::size_t kHookCount = 5;

    // Runs `report(self)` while the monitor is installed.
    template <typename Report>
    static void WithMonitor(Report&& report) noexcept {
        auto hookLease = hookActivity.Acquire();
        auto* const self = active.load(std::memory_order_acquire);
        if (hookLease && self) {
            report(*self);
        }
    }

    static void TaxiHook(safetyhook::Context& context) noexcept {
        WithMonitor(
            [&](Implementation& self) { self.Report(Job::Taxi, context.ecx); });
    }

    static void ParamedicHook(safetyhook::Context& context) noexcept {
        WithMonitor([&](Implementation& self) {
            self.Report(Job::Paramedic, context.eax);
        });
    }

    static void FirefighterHook(safetyhook::Context& context) noexcept {
        WithMonitor([&](Implementation& self) {
            if (const auto levels = ReadMemory<std::uint32_t>(
                    context.ebp + addresses::kFirefighterLevelsOffset)) {
                self.Report(Job::Firefighter, *levels);
            }
        });
    }

    static void TowingHook(safetyhook::Context& context) noexcept {
        WithMonitor([&](Implementation& self) {
            if (const auto index = ReadMemory<std::uint32_t>(
                    context.ebp + addresses::kTowingLevelIndexOffset)) {
                self.Report(Job::Towing, *index + 1);
            }
        });
    }

    static void HoingHook(safetyhook::Context&) noexcept {
        WithMonitor([](Implementation& self) {
            if (const auto highest = ReadMemory<std::uint32_t>(
                    self.game->base + addresses::kHoingManagerRva +
                    addresses::kHoingHighestLevelOffset)) {
                self.Report(Job::Hoing, *highest);
            }
        });
    }

    void Report(const Job job, const std::uint32_t level) noexcept {
        if (level == 0 || level > addresses::kJobLevelCount) {
            return;
        }
        try {
            if (levelCompleted) {
                levelCompleted(job, level);
            }
        } catch (...) {
            callbackFailures.fetch_add(1, std::memory_order_release);
        }
    }

    inline static std::atomic<Implementation*> active{};
    inline static HandlerActivity hookActivity;
    const ModuleInfo* game{};
    Callback levelCompleted;
    std::array<safetyhook::MidHook, kHookCount> hooks;
    std::atomic<std::uint32_t> callbackFailures;
};

JobLevelMonitor::JobLevelMonitor() = default;
JobLevelMonitor::~JobLevelMonitor() = default;

bool JobLevelMonitor::Install(Callback levelCompleted) {
    implementation_ = std::make_unique<Implementation>();
    if (!implementation_->Install(std::move(levelCompleted))) {
        implementation_.reset();
        return false;
    }
    return true;
}

void JobLevelMonitor::Poll() {
    if (implementation_) {
        implementation_->Poll();
    }
}

void JobLevelMonitor::Remove() {
    implementation_.reset();
}
}  // namespace sr2ap
