#pragma once

#include <cstdint>
#include <string>

namespace sr2ap {
    enum class ProgressionKind {
        Hitman,
        ChopShop,
        Mission,
        Activity,
        Racing,
        Cd
    };

    struct ProgressionEvent {
        ProgressionKind kind;
        std::string key;
        std::uint32_t previous{};
        std::uint32_t current{};
    };
}  // namespace sr2ap
