#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sr2ap {
// Overwrites an instruction in place and puts the original bytes back when
// restored or destroyed. Restoring is skipped when something else changed the
// site after it was patched.
class BytePatch {
   public:
    BytePatch() = default;

    ~BytePatch() {
        Restore();
    }

    BytePatch(const BytePatch&) = delete;
    BytePatch& operator=(const BytePatch&) = delete;
    BytePatch(BytePatch&&) = delete;
    BytePatch& operator=(BytePatch&&) = delete;

    // Fails unless the site currently holds `original`. A write that reached
    // memory but could not restore protection or flush the instruction cache
    // still counts as applied, so it is undone by Restore().
    bool Apply(std::string_view subsystem, std::string_view name,
               std::uintptr_t address, std::span<const std::uint8_t> original,
               std::span<const std::uint8_t> replacement);
    void Restore();

    [[nodiscard]] bool IsApplied() const noexcept {
        return applied_;
    }

   private:
    std::string subsystem_;
    std::string name_;
    std::uintptr_t address_{};
    std::vector<std::uint8_t> original_;
    std::vector<std::uint8_t> replacement_;
    bool applied_{};
};
}  // namespace sr2ap
