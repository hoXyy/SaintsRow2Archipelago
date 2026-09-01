#pragma once

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace sr2ap {
struct MemoryWriteResult {
    bool bytesWritten{};
    bool cacheFlushed{};
    bool protectionRestored{};

    explicit operator bool() const noexcept {
        return bytesWritten && cacheFlushed && protectionRestored;
    }
};

bool IsReadableAddress(const void* address, std::size_t size);
bool IsExecutableAddress(const void* address);
bool IsInsideModule(HMODULE module, const void* address);
bool SafeCopy(const void* address, void* destination, std::size_t size);
MemoryWriteResult WriteExecutableMemory(void* destination,
                                        std::span<const std::uint8_t> bytes);
std::optional<std::string> ReadFixedString(std::uintptr_t address,
                                           std::size_t capacity);
std::optional<std::vector<std::uint8_t>> CaptureBytes(const void* address,
                                                      std::size_t size);
enum class DetourKind {
    None,
    RelativeJump,
    RelativeCall,
    IndirectJump,
    PushReturn,
    Unknown
};
DetourKind DetectDetour(const void* address);
const char* ToString(DetourKind kind);
}  // namespace sr2ap
