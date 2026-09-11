#pragma once

#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
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
std::optional<std::uintptr_t> ResolveRelativeCallTarget(std::uintptr_t address);
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

template <typename T>
std::optional<T> ReadMemory(std::uintptr_t address) {
    static_assert(std::is_trivially_copyable_v<T>);

    T value{};

    if (!SafeCopy(reinterpret_cast<const void*>(address), &value, sizeof(T))) {
        return std::nullopt;
    }

    return value;
}

template <typename T, std::size_t N>
std::optional<std::array<T, N>> ReadMemoryIntoArray(std::uintptr_t address) {
    std::array<T, N> value{};

    if (!SafeCopy(reinterpret_cast<const void*>(address), value.data(),
                  sizeof(T) * N)) {
        return std::nullopt;
    }

    return value;
}

template <std::size_t N>
bool MatchesBytes(std::uintptr_t address,
                  const std::array<std::uint8_t, N>& expected) {
    std::array<std::uint8_t, N> actual{};

    return SafeCopy(reinterpret_cast<const void*>(address), actual.data(), N) &&
           actual == expected;
}
}  // namespace sr2ap
