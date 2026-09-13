#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "game/GameState.hpp"
#include "progression/ProgressionEvent.hpp"
#include "progression/ProgressionReader.hpp"

namespace sr2ap {

using ProgressionEventSink = std::function<void(const ProgressionEvent&)>;

class ProgressionMonitor {
   public:
    explicit ProgressionMonitor(std::filesystem::path statusPath,
                                ProgressionEventSink eventSink = {},
                                bool writeStatusFile = false);

    void CaptureManualSnapshot(const GameContext& context) const;
    void Poll(const GameContext& context);

   private:
    [[nodiscard]] std::string CaptureStatus(const GameContext& context) const;

    std::filesystem::path statusPath_;
    ProgressionEventSink eventSink_;
    bool writeStatusFile_{};
    ProgressionReaders readers_;
};

}  // namespace sr2ap
