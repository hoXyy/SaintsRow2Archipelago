#include "sr2ap/Memory.hpp"
#include <cstring>
#include "sr2ap/ModuleInfo.hpp"

namespace sr2ap {
    namespace {
        bool HasAccess(DWORD protect, bool executable) {
            if ((protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0) {
                return false;
            }
            const auto basic = protect & 0xFF;
            if (executable) {
                return basic == PAGE_EXECUTE || basic == PAGE_EXECUTE_READ || basic == PAGE_EXECUTE_READWRITE ||
                       basic == PAGE_EXECUTE_WRITECOPY;
            }
            return basic == PAGE_READONLY || basic == PAGE_READWRITE || basic == PAGE_WRITECOPY ||
                   basic == PAGE_EXECUTE_READ || basic == PAGE_EXECUTE_READWRITE || basic == PAGE_EXECUTE_WRITECOPY;
        }
    }  // namespace

    bool IsReadableAddress(const void* address, std::size_t size) {
        if (!address || size == 0) {
            return false;
        }
        auto cursor = reinterpret_cast<std::uintptr_t>(address);
        const auto end = cursor + size;
        if (end < cursor) {
            return false;
        }
        while (cursor < end) {
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQuery(reinterpret_cast<const void*>(cursor), &info, sizeof(info)) || info.State != MEM_COMMIT ||
                !HasAccess(info.Protect, false)) {
                return false;
            }
            const auto regionEnd = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;
            if (regionEnd <= cursor) {
                return false;
            }
            cursor = regionEnd < end ? regionEnd : end;
        }
        return true;
    }

    bool IsExecutableAddress(const void* address) {
        MEMORY_BASIC_INFORMATION info{};
        return address && VirtualQuery(address, &info, sizeof(info)) && info.State == MEM_COMMIT &&
               HasAccess(info.Protect, true);
    }

    bool IsInsideModule(HMODULE module, const void* address) {
        const auto info = InspectModule(module);

        if (!info || !address) {
            return false;
        }

        const auto value = reinterpret_cast<std::uintptr_t>(address);
        return value >= info->base && value < info->base + info->imageSize;
    }

    bool SafeCopy(const void* address, void* destination, std::size_t size) {
        if (!destination || !IsReadableAddress(address, size)) {
            return false;
        }

        SIZE_T copied{};
        return ReadProcessMemory(GetCurrentProcess(), address, destination, size, &copied) && copied == size;
    }

    std::optional<std::string> ReadFixedString(std::uintptr_t address, std::size_t capacity) {
        std::string buffer(capacity, '\0');
        if (!SafeCopy(reinterpret_cast<const void*>(address), buffer.data(), buffer.size())) {
            return std::nullopt;
        }
        const auto terminator = buffer.find('\0');
        if (terminator == std::string::npos) {
            return std::nullopt;
        }
        buffer.resize(terminator);
        return buffer;
    }

    std::optional<std::vector<std::uint8_t>> CaptureBytes(const void* address, std::size_t size) {
        std::vector<std::uint8_t> result(size);

        if (!SafeCopy(address, result.data(), size)) {
            return std::nullopt;
        }

        return result;
    }

    DetourKind DetectDetour(const void* address) {
        std::uint8_t bytes[6]{};
        if (!SafeCopy(address, bytes, sizeof(bytes))) {
            return DetourKind::Unknown;
        }

        if (bytes[0] == 0xE9 || bytes[0] == 0xEB) {
            return DetourKind::RelativeJump;
        }

        if (bytes[0] == 0xE8) {
            return DetourKind::RelativeCall;
        }

        if (bytes[0] == 0xFF &&
            (bytes[1] == 0x25 || bytes[1] == 0xE0 || bytes[1] == 0xE1 || bytes[1] == 0xE2 || bytes[1] == 0xE3)) {
            return DetourKind::IndirectJump;
        }

        if (bytes[0] == 0x68 && bytes[5] == 0xC3) {
            return DetourKind::PushReturn;
        }

        return DetourKind::None;
    }

    const char* ToString(DetourKind kind) {
        switch (kind) {
            case DetourKind::None:
                return "none";
            case DetourKind::RelativeJump:
                return "relative_jump";
            case DetourKind::RelativeCall:
                return "relative_call";
            case DetourKind::IndirectJump:
                return "indirect_jump";
            case DetourKind::PushReturn:
                return "push_return";
            case DetourKind::Unknown:
                return "unknown";
        }

        return "unknown";
    }
}  // namespace sr2ap
