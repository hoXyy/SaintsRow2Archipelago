#include "SaveRevisionMonitor.hpp"

#include <windows.h>

#include <array>
#include <atomic>
#include <cstddef>
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
struct SaveHeader {
    std::uint32_t checksum{};
    std::array<std::byte, 8> reserved{};
    std::array<char, 8> kind{};
};

static_assert(offsetof(SaveHeader, kind) == 0x0C);
static_assert(sizeof(SaveHeader) == 0x14);

constexpr std::array kCitySaveKind{'s', 'r', '2', '_', 'c', 'i', 't', 'y'};

std::optional<std::uint32_t> ReadChecksum(std::uintptr_t object) {
    SaveHeader header{};
    if (!object || !SafeCopy(reinterpret_cast<const void*>(object), &header,
                             sizeof(header)) ||
        header.kind != kCitySaveKind) {
        return std::nullopt;
    }
    return header.checksum;
}
}  // namespace

struct SaveRevisionMonitor::Implementation {
    Implementation() = default;

    ~Implementation() {
        Remove();
    }

    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

    bool Install(Callback onLoaded, Callback onSaved,
                 LoadStartingCallback onLoadStarting) {
        const auto game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }
        const auto loadCall = game->base + addresses::kSaveLoadInternalCallRva;
        const auto saveCall = game->base + addresses::kSaveWriteOpenCallRva;
        const auto loadTarget = game->base + addresses::kSaveLoadAllRva;
        const auto saveTarget = game->base + addresses::kCFileOpenRva;
        if (ResolveRelativeCallTarget(loadCall) != loadTarget ||
            ResolveRelativeCallTarget(saveCall) != saveTarget) {
            LogError("SaveRevision", "Unexpected save/load call targets");
            return false;
        }
        loaded = std::move(onLoaded);
        saved = std::move(onSaved);
        loadStarting = std::move(onLoadStarting);

        auto pendingLoadHook = safetyhook::MidHook::create(
            reinterpret_cast<void*>(loadCall), &LoadCallHook,
            safetyhook::MidHook::StartDisabled);
        if (!pendingLoadHook) {
            LogError("SaveRevision", "Could not create load-call hook");
            return false;
        }

        auto pendingSaveHook = safetyhook::MidHook::create(
            reinterpret_cast<void*>(saveCall), &SaveCallHook,
            safetyhook::MidHook::StartDisabled);
        if (!pendingSaveHook) {
            LogError("SaveRevision", "Could not create save-call hook");
            return false;
        }

        loadHook = std::move(*pendingLoadHook);
        saveHook = std::move(*pendingSaveHook);
        hookActivity.Start();

        if (const auto enabled = loadHook.enable(); !enabled) {
            LogError("SaveRevision", "Could not enable load-call hook");
            Remove();
            return false;
        }
        if (const auto enabled = saveHook.enable(); !enabled) {
            LogError("SaveRevision", "Could not enable save-call hook");
            Remove();
            return false;
        }

        active.store(this, std::memory_order_release);
        return true;
    }

    void Poll() {
        const auto load = loadSequence.load(std::memory_order_acquire);
        if (load != reportedLoad) {
            reportedLoad = load;
            const auto value = loadChecksum.load(std::memory_order_relaxed);
            currentChecksum.store(value, std::memory_order_release);
            if (loaded) {
                loaded(value);
            }
        }
        const auto save = saveSequence.load(std::memory_order_acquire);
        if (save != reportedSave) {
            reportedSave = save;
            if (saved) {
                saved(saveChecksum.load(std::memory_order_relaxed));
            }
        }
        const auto failures =
            callbackFailures.exchange(0, std::memory_order_acq_rel);
        if (failures != 0) {
            LogError("SaveRevision",
                     "Exceptions suppressed inside load-call hook=" +
                         std::to_string(failures));
        }
    }

    std::optional<std::uint32_t> CurrentChecksum() const {
        const auto value = currentChecksum.load(std::memory_order_acquire);
        return value == 0 ? std::nullopt : std::optional<std::uint32_t>{value};
    }

    void Remove() {
        active.store(nullptr, std::memory_order_release);
        DisableHook(saveHook, "save");
        DisableHook(loadHook, "load");

        hookActivity.Stop();
        while (!hookActivity.IsIdle()) {
            SwitchToThread();
        }

        saveHook.reset();
        loadHook.reset();
    }

   private:
    static void DisableHook(safetyhook::MidHook& hook, const char* name) {
        if (!hook || !hook.enabled()) {
            return;
        }
        if (const auto disabled = hook.disable(); !disabled) {
            LogError("SaveRevision",
                     std::string("Could not disable ") + name + "-call hook");
        }
    }

    static void LoadCallHook(safetyhook::Context& context) noexcept {
        auto hookLease = hookActivity.Acquire();
        auto* const self = active.load(std::memory_order_acquire);
        if (!hookLease || !self) {
            return;
        }

        try {
            if (self->loadStarting) {
                self->loadStarting(
                    static_cast<std::uint32_t>(GetCurrentThreadId()));
            }
        } catch (...) {
            self->callbackFailures.fetch_add(1, std::memory_order_release);
        }

        self->saveObject.store(context.eax, std::memory_order_relaxed);
        if (const auto checksum = ReadChecksum(context.eax)) {
            self->loadChecksum.store(*checksum, std::memory_order_relaxed);
            self->loadSequence.fetch_add(1, std::memory_order_release);
        }
    }

    static void SaveCallHook(safetyhook::Context&) noexcept {
        auto hookLease = hookActivity.Acquire();
        auto* const self = active.load(std::memory_order_acquire);
        if (!hookLease || !self) {
            return;
        }

        if (const auto checksum = ReadChecksum(
                self->saveObject.load(std::memory_order_relaxed))) {
            self->saveChecksum.store(*checksum, std::memory_order_relaxed);
            self->saveSequence.fetch_add(1, std::memory_order_release);
        }
    }

    inline static std::atomic<Implementation*> active{};
    inline static HandlerActivity hookActivity;
    Callback loaded;
    Callback saved;
    LoadStartingCallback loadStarting;
    safetyhook::MidHook loadHook;
    safetyhook::MidHook saveHook;
    std::atomic<std::uintptr_t> saveObject{};
    std::atomic<std::uint32_t> loadChecksum{};
    std::atomic<std::uint32_t> saveChecksum{};
    std::atomic<std::uint32_t> currentChecksum{};
    std::atomic<std::uint64_t> loadSequence{};
    std::atomic<std::uint64_t> saveSequence{};
    std::atomic<std::uint32_t> callbackFailures{};
    std::uint64_t reportedLoad{};
    std::uint64_t reportedSave{};
};

SaveRevisionMonitor::SaveRevisionMonitor() = default;
SaveRevisionMonitor::~SaveRevisionMonitor() = default;

bool SaveRevisionMonitor::Install(Callback loaded, Callback saved,
                                  LoadStartingCallback loadStarting) {
    implementation_ = std::make_unique<Implementation>();
    if (!implementation_->Install(std::move(loaded), std::move(saved),
                                  std::move(loadStarting))) {
        implementation_.reset();
        return false;
    }
    return true;
}

void SaveRevisionMonitor::Poll() {
    if (implementation_) {
        implementation_->Poll();
    }
}

void SaveRevisionMonitor::Remove() {
    implementation_.reset();
}

std::optional<std::uint32_t> SaveRevisionMonitor::CurrentChecksum() const {
    return implementation_ ? implementation_->CurrentChecksum() : std::nullopt;
}
}  // namespace sr2ap
