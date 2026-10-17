#include "BytePatch.hpp"

#include <algorithm>

#include "game/Memory.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
bool Holds(const std::uintptr_t address,
           const std::span<const std::uint8_t> expected) {
    std::vector<std::uint8_t> actual(expected.size());
    return SafeCopy(reinterpret_cast<const void*>(address), actual.data(),
                    actual.size()) &&
           std::ranges::equal(actual, expected);
}
}  // namespace

bool BytePatch::Apply(const std::string_view subsystem,
                      const std::string_view name, const std::uintptr_t address,
                      const std::span<const std::uint8_t> original,
                      const std::span<const std::uint8_t> replacement) {
    if (applied_ || original.size() != replacement.size()) {
        return false;
    }

    subsystem_ = subsystem;
    name_ = name;
    if (!Holds(address, original)) {
        LogError(subsystem_, "Unexpected bytes at " + name_);
        return false;
    }

    const auto result =
        WriteExecutableMemory(reinterpret_cast<void*>(address), replacement);
    if (result.bytesWritten) {
        address_ = address;
        original_.assign(original.begin(), original.end());
        replacement_.assign(replacement.begin(), replacement.end());
        applied_ = true;
    }
    if (!result) {
        LogError(subsystem_, "Failed to patch " + name_);
        return false;
    }
    return true;
}

void BytePatch::Restore() {
    if (!applied_) {
        return;
    }
    applied_ = false;

    if (!Holds(address_, replacement_)) {
        LogWarning(subsystem_, name_ + " changed; not restoring");
        return;
    }
    if (!WriteExecutableMemory(reinterpret_cast<void*>(address_), original_)) {
        LogError(subsystem_, "Failed to restore " + name_);
    }
}
}  // namespace sr2ap
