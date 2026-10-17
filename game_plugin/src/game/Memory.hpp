#pragma once

#include <windows.h>

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace sr2ap {
struct ModuleInfo;

struct MemoryWriteResult {
    bool bytesWritten{};
    bool cacheFlushed{};
    bool protectionRestored{};

    explicit operator bool() const noexcept {
        return bytesWritten && cacheFlushed && protectionRestored;
    }
};

bool IsReadableAddress(std::uintptr_t address, std::size_t size);
bool IsExecutableAddress(std::uintptr_t address);
bool IsInsideModule(const ModuleInfo& module, const void* address);
bool SafeCopy(const void* address, void* destination, std::size_t size);
MemoryWriteResult WriteExecutableMemory(void* destination,
                                        std::span<const std::uint8_t> bytes);
std::optional<std::string> ReadFixedString(std::uintptr_t address,
                                           std::size_t capacity);
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
std::optional<std::string> ReadValidatedString(std::uintptr_t address,
                                               std::size_t capacity,
                                               std::string_view prefix);

template <typename T>
std::optional<T> ReadMemory(std::uintptr_t address) {
    static_assert(std::is_trivially_copyable_v<T>);

    T value{};

    if (!SafeCopy(reinterpret_cast<const void*>(address), &value, sizeof(T))) {
        return std::nullopt;
    }

    return value;
}

template <typename T>
bool ReadMemoryIntoValue(std::uintptr_t address, T* destination) {
    static_assert(std::is_trivially_copyable_v<T>);

    return destination != nullptr &&
           SafeCopy(reinterpret_cast<const void*>(address), destination,
                    sizeof(T));
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
bool MatchesBytes(const std::uintptr_t address,
                  const std::array<std::uint8_t, N>& expected) {
    std::array<std::uint8_t, N> actual{};

    return SafeCopy(reinterpret_cast<const void*>(address), actual.data(), N) &&
           actual == expected;
}

// A function the plugin hooks or calls: inside the module, executable, and
// not already detoured by another mod.
bool ValidateFunctionTarget(const ModuleInfo& module, std::uintptr_t address);

template <std::size_t N>
bool ValidateHookTarget(const ModuleInfo& module, const std::uintptr_t address,
                        const std::array<std::uint8_t, N>& expected) {
    return ValidateFunctionTarget(module, address) &&
           MatchesBytes(address, expected);
}

// An instruction the plugin overwrites in place: inside the module,
// executable, and holding the expected bytes.
template <std::size_t N>
bool ValidatePatchSite(const ModuleInfo& module, const std::uintptr_t address,
                       const std::array<std::uint8_t, N>& expected) {
    return IsInsideModule(module, reinterpret_cast<const void*>(address)) &&
           IsExecutableAddress(address) && MatchesBytes(address, expected);
}

}  // namespace sr2ap
