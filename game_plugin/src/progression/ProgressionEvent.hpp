#pragma once

#include <cstdint>
#include <string>

namespace sr2ap {
struct ProgressionEvent {
    std::string category;
    std::string key;
    std::uint32_t previous{};
    std::uint32_t current{};
};
}  // namespace sr2ap