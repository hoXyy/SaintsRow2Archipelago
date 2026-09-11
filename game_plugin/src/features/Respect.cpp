#include "Respect.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <string>

#include "game/Addresses.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/HandlerActivity.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
constexpr std::array<std::string_view, 2> kRespectItems{
    {"+1 Respect", "+1 Bonus Respect"}};
constexpr std::uint32_t kMaximumRespectBars{99};
constexpr std::size_t kInstructionSize{6};
using Instruction = std::array<std::uint8_t, kInstructionSize>;
constexpr Instruction kSaveLoadWriter{{0x89, 0x95, 0x00, 0x12, 0x00, 0x00}};

struct BlockedWriter {
    const char* name;
    std::ptrdiff_t rva;
    Instruction expected;
};

constexpr std::array<BlockedWriter, 3> kBlockedWriters{{
    {"style_award",
     addresses::kRespectAwardWriteRva,
     {0x89, 0x97, 0x00, 0x12, 0x00, 0x00}},
    {"generic_set",
     addresses::kRespectSetWriteRva,
     {0x89, 0x8A, 0x00, 0x12, 0x00, 0x00}},
    {"generic_add",
     addresses::kRespectAddWriteRva,
     {0x89, 0x91, 0x00, 0x12, 0x00, 0x00}},
}};
}  // namespace

struct RespectController::Implementation {
    struct RuntimeState {
        std::uint32_t player{};
        std::uint32_t pointsPerBar{};
    };

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
        gameBase = game->base;

        const auto saveLoadAddress = gameBase + addresses::kRespectLoadWriteRva;
        std::optional<Instruction> saveLoadActual =
            ReadMemoryIntoArray<std::uint8_t, kInstructionSize>(
                saveLoadAddress);
        if (!IsInsideModule(game->handle,
                            reinterpret_cast<const void*>(saveLoadAddress)) ||
            !IsExecutableAddress(
                reinterpret_cast<const void*>(saveLoadAddress)) ||
            !saveLoadActual || *saveLoadActual != kSaveLoadWriter) {
            LogError("Respect", "Unexpected bytes at writer=save_load");
            return false;
        }

        for (const auto& writer : kBlockedWriters) {
            const auto address = gameBase + writer.rva;
            std::optional<Instruction> actual =
                ReadMemoryIntoArray<std::uint8_t, kInstructionSize>(address);

            if (!IsInsideModule(game->handle,
                                reinterpret_cast<const void*>(address)) ||
                !IsExecutableAddress(reinterpret_cast<const void*>(address)) ||
                !actual || *actual != writer.expected) {
                LogError("Respect", std::string("Unexpected bytes at writer=") +
                                        writer.name);
                return false;
            }
        }

        exceptionHandler = AddVectoredExceptionHandler(1, &HandleException);
        if (!exceptionHandler) {
            LogError("Respect", "Failed to install save-load writer handler");
            return false;
        }
        handlerActivity.Start();
        active.store(this, std::memory_order_release);

        Instruction breakpoint{};
        breakpoint.fill(0x90);
        breakpoint.front() = 0xCC;

        const auto saveLoadAddressWriteResult = WriteExecutableMemory(
            reinterpret_cast<void*>(saveLoadAddress), breakpoint);

        if (saveLoadAddressWriteResult.bytesWritten) {
            saveLoadPatched = true;
        }

        if (!saveLoadAddressWriteResult) {
            LogError("Respect", "Failed to intercept writer=save_load");
            Remove();
            return false;
        }

        Instruction nops{};
        nops.fill(0x90);
        for (std::size_t index = 0; index < kBlockedWriters.size(); ++index) {
            const auto writeResult = WriteExecutableMemory(
                reinterpret_cast<void*>(gameBase + kBlockedWriters[index].rva),
                nops);

            if (writeResult.bytesWritten) {
                patchedCount = index + 1;
            }

            if (!writeResult) {
                LogError("Respect", std::string("Failed to block writer=") +
                                        kBlockedWriters[index].name);
                Remove();
                return false;
            }
        }

        installed = true;
        return true;
    }

    void Remove() {
        if (patchedCount != 0) {
            Restore(patchedCount);
        }
        RestoreSaveLoadWriter();
        handlerActivity.Stop();
        while (!handlerActivity.IsIdle()) {
            Sleep(0);
        }
        active.store(nullptr, std::memory_order_release);
        if (exceptionHandler) {
            RemoveVectoredExceptionHandler(exceptionHandler);
            exceptionHandler = nullptr;
        }
        permittedLoadThread.store(0, std::memory_order_release);
        installed = false;
    }

    void PermitNextSaveRestore(std::uint32_t threadId) noexcept {
        permittedLoadThread.store(threadId, std::memory_order_release);
    }

    bool ActivateReceivedItem(const std::string_view itemName) {
        if (!installed || std::find(kRespectItems.begin(), kRespectItems.end(),
                                    itemName) == kRespectItems.end()) {
            return false;
        }

        const auto state = ReadRuntimeState();
        if (!state) {
            LogInfo(
                "Respect",
                "Player state unavailable; AP respect item must be retried");
            return false;
        }
        return ApplyOneBar(*state);
    }

    void Update() {
        if (!installed) {
            return;
        }
        const auto blocked =
            blockedCheckpointRestores.load(std::memory_order_acquire);
        if (blocked != reportedBlockedCheckpointRestores) {
            LogInfo("Respect", "Blocked checkpoint respect restores=" +
                                   std::to_string(blocked));
            reportedBlockedCheckpointRestores = blocked;
        }
    }

   private:
    static LONG CALLBACK HandleException(EXCEPTION_POINTERS* exception) {
        auto* const self = active.load(std::memory_order_acquire);
        auto handlerLease = handlerActivity.Acquire();
        if (!self || !handlerLease || !exception ||
            !exception->ExceptionRecord || !exception->ContextRecord ||
            exception->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT ||
            reinterpret_cast<std::uintptr_t>(
                exception->ExceptionRecord->ExceptionAddress) !=
                self->gameBase + addresses::kRespectLoadWriteRva) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        auto* const context = exception->ContextRecord;
        const auto threadId = static_cast<std::uint32_t>(GetCurrentThreadId());
        auto permittedThread = threadId;

        // Only want to allow respect restore when loading the save, not when
        // restarting an activity level
        const bool permitted =
            self->permittedLoadThread.compare_exchange_strong(
                permittedThread, 0, std::memory_order_acq_rel,
                std::memory_order_acquire);
        if (permitted) {
            *reinterpret_cast<volatile std::uint32_t*>(
                static_cast<std::uintptr_t>(context->Ebp) +
                addresses::kPlayerRespectOffset) = context->Edx;
        } else {
            self->blockedCheckpointRestores.fetch_add(
                1, std::memory_order_release);
        }
        context->Eip = static_cast<DWORD>(self->gameBase +
                                          addresses::kRespectLoadWriteRva +
                                          kInstructionSize);
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    std::optional<RuntimeState> ReadRuntimeState() const {
        auto playerState =
            ReadMemory<std::uint32_t>(gameBase + addresses::kPlayerGlobalRva);
        auto pointsPerBar = ReadMemory<std::uint32_t>(
            gameBase + addresses::kRespectPointsPerBarRva);

        if (!playerState || *playerState == 0 || !pointsPerBar ||
            *pointsPerBar == 0) {
            return std::nullopt;
        }

        RuntimeState state{};
        state.pointsPerBar = *pointsPerBar;
        state.player = *playerState;
        return state;
    }

    static bool ReadRespect(std::uint32_t player, std::uint32_t& value) {
        return SafeCopy(
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(player) +
                                          addresses::kPlayerRespectOffset),
            &value, sizeof(value));
    }

    static bool WriteRespect(std::uint32_t player, std::uint32_t value) {
        auto* const field =
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(player) +
                                    addresses::kPlayerRespectOffset);
        SIZE_T written{};
        return WriteProcessMemory(GetCurrentProcess(), field, &value,
                                  sizeof(value), &written) &&
               written == sizeof(value);
    }

    static std::uint32_t MaximumPoints(const RuntimeState& state) {
        return static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(state.pointsPerBar) *
            kMaximumRespectBars);
    }

    bool ApplyOneBar(const RuntimeState& state) {
        std::uint32_t current{};
        if (!ReadRespect(state.player, current)) {
            return false;
        }

        const auto desired = static_cast<std::uint32_t>(std::min<std::uint64_t>(
            static_cast<std::uint64_t>(current) + state.pointsPerBar,
            MaximumPoints(state)));

        if (!WriteRespect(state.player, desired)) {
            return false;
        }

        LogInfo("Respect",
                "Granted AP respect bar before=" + std::to_string(current) +
                    " after=" + std::to_string(desired));
        return true;
    }

    void Restore(std::size_t count) {
        Instruction nops{};
        nops.fill(0x90);
        for (std::size_t index = count; index > 0; --index) {
            const auto site = index - 1;
            auto const address = gameBase + kBlockedWriters[site].rva;
            std::optional<Instruction> actual =
                ReadMemoryIntoArray<std::uint8_t, kInstructionSize>(address);

            if (!actual || *actual != nops) {
                LogWarning(
                    "Respect",
                    std::string("Writer changed; not restoring writer=") +
                        kBlockedWriters[site].name);
                continue;
            }
            if (!WriteExecutableMemory(reinterpret_cast<void*>(address),
                                       kBlockedWriters[site].expected)) {
                LogError("Respect", std::string("Failed to restore writer=") +
                                        kBlockedWriters[site].name);
            }
        }
        patchedCount = 0;
    }

    void RestoreSaveLoadWriter() {
        if (!saveLoadPatched) {
            return;
        }
        Instruction breakpoint{};
        breakpoint.fill(0x90);
        breakpoint.front() = 0xCC;
        auto* const address =
            reinterpret_cast<void*>(gameBase + addresses::kRespectLoadWriteRva);
        std::optional<Instruction> actual =
            ReadMemoryIntoArray<std::uint8_t, kInstructionSize>(
                gameBase + addresses::kRespectLoadWriteRva);

        if (!actual || *actual != breakpoint) {
            LogWarning("Respect",
                       "Writer changed; not restoring writer=save_load");
        } else if (!WriteExecutableMemory(address, kSaveLoadWriter)) {
            LogError("Respect", "Failed to restore writer=save_load");
        }
        saveLoadPatched = false;
    }

    inline static std::atomic<Implementation*> active{};
    inline static HandlerActivity handlerActivity;
    std::uintptr_t gameBase{};
    std::atomic<std::uint32_t> permittedLoadThread;
    std::atomic<std::uint64_t> blockedCheckpointRestores;
    std::uint64_t reportedBlockedCheckpointRestores{};
    std::size_t patchedCount{};
    void* exceptionHandler{};
    bool saveLoadPatched{};
    bool installed{};
};

RespectController::RespectController() = default;
RespectController::~RespectController() = default;

bool RespectController::Install() {
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

void RespectController::PermitNextSaveRestore(std::uint32_t threadId) noexcept {
    if (implementation_) {
        implementation_->PermitNextSaveRestore(threadId);
    }
}

void RespectController::Remove() {
    implementation_.reset();
}

bool RespectController::ActivateReceivedItem(const std::string_view itemName) {
    return implementation_ && implementation_->ActivateReceivedItem(itemName);
}

void RespectController::Update() {
    if (implementation_) {
        implementation_->Update();
    }
}
}  // namespace sr2ap
