#include "sr2ap/SaveRevisionMonitor.hpp"

#include "sr2ap/Addresses.hpp"
#include "sr2ap/HandlerActivity.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <windows.h>
#include <array>
#include <atomic>
#include <cstring>
#include <string>
#include <utility>

namespace sr2ap {
    namespace {
        constexpr std::size_t kCallSize{5};

        bool WriteByte(void* destination, std::uint8_t value) {
            DWORD oldProtection{};
            if (!VirtualProtect(destination, 1, PAGE_EXECUTE_READWRITE, &oldProtection)) {
                return false;
            }
            SIZE_T written{};
            const bool copied =
                WriteProcessMemory(GetCurrentProcess(), destination, &value, 1, &written) && written == 1;
            if (copied) {
                FlushInstructionCache(GetCurrentProcess(), destination, 1);
            }
            DWORD ignored{};
            return VirtualProtect(destination, 1, oldProtection, &ignored) && copied;
        }

        std::optional<std::uintptr_t> CallTarget(std::uintptr_t address) {
            std::array<std::uint8_t, kCallSize> bytes{};
            if (!SafeCopy(reinterpret_cast<const void*>(address), bytes.data(), bytes.size()) || bytes[0] != 0xE8) {
                return std::nullopt;
            }
            std::int32_t displacement{};
            std::memcpy(&displacement, bytes.data() + 1, sizeof(displacement));
            return address + kCallSize + displacement;
        }

        std::optional<std::uint32_t> ReadChecksum(std::uintptr_t object) {
            std::array<char, 8> kind{};
            std::uint32_t checksum{};
            if (!object || !SafeCopy(reinterpret_cast<const void*>(object), &checksum, sizeof(checksum)) ||
                !SafeCopy(reinterpret_cast<const void*>(object + 0x0C), kind.data(), kind.size()) ||
                std::memcmp(kind.data(), "sr2_city", kind.size()) != 0) {
                return std::nullopt;
            }
            return checksum;
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

        bool Install(Callback onLoaded, Callback onSaved, LoadStartingCallback onLoadStarting) {
            const auto game = InspectSupportedGameModule();
            if (!game) {
                return false;
            }
            base = game->base;
            loadCall = base + addresses::kSaveLoadInternalCallRva;
            saveCall = base + addresses::kSaveWriteOpenCallRva;
            loadTarget = base + addresses::kSaveLoadAllRva;
            saveTarget = base + addresses::kCFileOpenRva;
            if (CallTarget(loadCall) != loadTarget || CallTarget(saveCall) != saveTarget) {
                LogError("SaveRevision", "Unexpected save/load call targets");
                return false;
            }
            loaded = std::move(onLoaded);
            saved = std::move(onSaved);
            loadStarting = std::move(onLoadStarting);
            handler = AddVectoredExceptionHandler(1, &HandleException);
            if (!handler) {
                return false;
            }
            handlerActivity.Start();
            active.store(this, std::memory_order_release);
            installed = true;
            if (!WriteByte(reinterpret_cast<void*>(loadCall), 0xCC) ||
                !WriteByte(reinterpret_cast<void*>(saveCall), 0xCC)) {
                Remove();
                return false;
            }
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
        }

        std::optional<std::uint32_t> CurrentChecksum() const {
            const auto value = currentChecksum.load(std::memory_order_acquire);
            return value == 0 ? std::nullopt : std::optional<std::uint32_t>{value};
        }

        void Remove() {
            if (installed) {
                RestoreCall(loadCall, "load");
                RestoreCall(saveCall, "save");
                installed = false;
            }
            handlerActivity.Stop();
            while (!handlerActivity.IsIdle()) {
                Sleep(0);
            }
            active.store(nullptr, std::memory_order_release);
            if (handler) {
                RemoveVectoredExceptionHandler(handler);
                handler = nullptr;
            }
        }

       private:
        void RestoreCall(std::uintptr_t address, const char* name) {
            std::uint8_t value{};
            if (!SafeCopy(reinterpret_cast<const void*>(address), &value, 1) || value != 0xCC ||
                !WriteByte(reinterpret_cast<void*>(address), 0xE8)) {
                LogWarning("SaveRevision", std::string("Could not restore ") + name + " call probe");
            }
        }

        static LONG CALLBACK HandleException(EXCEPTION_POINTERS* exception) {
            auto* self = active.load(std::memory_order_acquire);
            auto handlerLease = handlerActivity.Acquire();
            if (!self || !handlerLease || !exception || !exception->ExceptionRecord || !exception->ContextRecord ||
                exception->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT) {
                return EXCEPTION_CONTINUE_SEARCH;
            }
            const auto address = reinterpret_cast<std::uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
            if (address != self->loadCall && address != self->saveCall) {
                return EXCEPTION_CONTINUE_SEARCH;
            }
            auto* context = exception->ContextRecord;
            if (address == self->loadCall) {
                if (self->loadStarting) {
                    self->loadStarting(static_cast<std::uint32_t>(GetCurrentThreadId()));
                }
                self->saveObject.store(context->Eax, std::memory_order_relaxed);
                if (const auto checksum = ReadChecksum(context->Eax)) {
                    self->loadChecksum.store(*checksum, std::memory_order_relaxed);
                    self->loadSequence.fetch_add(1, std::memory_order_release);
                }
            } else if (const auto checksum = ReadChecksum(self->saveObject.load(std::memory_order_relaxed))) {
                self->saveChecksum.store(*checksum, std::memory_order_relaxed);
                self->saveSequence.fetch_add(1, std::memory_order_release);
            }
            const auto oldStack = static_cast<std::uintptr_t>(context->Esp);
            const auto newStack = oldStack - sizeof(std::uint32_t);
            const auto returnAddress = static_cast<std::uint32_t>(address + kCallSize);
            SIZE_T written{};
            if (!WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(newStack), &returnAddress,
                                    sizeof(returnAddress), &written) ||
                written != sizeof(returnAddress)) {
                return EXCEPTION_CONTINUE_SEARCH;
            }
            context->Esp = static_cast<DWORD>(newStack);
            context->Eip = static_cast<DWORD>(address == self->loadCall ? self->loadTarget : self->saveTarget);
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        inline static std::atomic<Implementation*> active{};
        inline static HandlerActivity handlerActivity;
        Callback loaded;
        Callback saved;
        LoadStartingCallback loadStarting;
        std::atomic<std::uintptr_t> saveObject;
        std::atomic<std::uint32_t> loadChecksum;
        std::atomic<std::uint32_t> saveChecksum;
        std::atomic<std::uint32_t> currentChecksum;
        std::atomic<std::uint64_t> loadSequence;
        std::atomic<std::uint64_t> saveSequence;
        std::uint64_t reportedLoad{};
        std::uint64_t reportedSave{};
        std::uintptr_t base{}, loadCall{}, saveCall{}, loadTarget{}, saveTarget{};
        void* handler{};
        bool installed{};
    };

    SaveRevisionMonitor::SaveRevisionMonitor() = default;
    SaveRevisionMonitor::~SaveRevisionMonitor() = default;

    bool SaveRevisionMonitor::Install(Callback loaded, Callback saved, LoadStartingCallback loadStarting) {
        implementation_ = std::make_unique<Implementation>();
        if (!implementation_->Install(std::move(loaded), std::move(saved), std::move(loadStarting))) {
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
