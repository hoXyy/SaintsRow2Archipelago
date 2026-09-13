#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ProgressionEvent.hpp"
#include "game/GameState.hpp"

namespace sr2ap {

struct ReaderUpdate {
    bool changed{};
    std::string statusSection;
    std::vector<ProgressionEvent> events;
};

class ProgressionReader {
   public:
    ProgressionReader() = default;
    virtual ~ProgressionReader() = default;

    ProgressionReader(const ProgressionReader&) = delete;
    ProgressionReader& operator=(const ProgressionReader&) = delete;
    ProgressionReader(ProgressionReader&&) = delete;
    ProgressionReader& operator=(ProgressionReader&&) = delete;

    [[nodiscard]] virtual std::string_view Id() const noexcept = 0;

    // Reads memory and updates the global progression event.
    [[nodiscard]] virtual ReaderUpdate Poll(const GameContext& context) = 0;

    // Reads memory and formats diagnostics without updating anything.
    [[nodiscard]] virtual std::string CaptureStatus(
        const GameContext& context) const = 0;
};

using ProgressionReaderPtr = std::unique_ptr<ProgressionReader>;
using ProgressionReaders = std::vector<ProgressionReaderPtr>;

}  // namespace sr2ap
