#include "ProgressionMonitor.hpp"

#include "ReaderRegistry.hpp"
#include "game/GameState.hpp"
#include "util/AtomicFile.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
void AppendStatusSection(std::string& output, std::string_view section) {
    if (!output.empty() && output.back() != '\n') {
        output.push_back('\n');
    }

    output.append(section);

    if (!output.empty() && output.back() != '\n') {
        output.push_back('\n');
    }
}

}  // namespace

ProgressionMonitor::ProgressionMonitor(std::filesystem::path statusPath,
                                       ProgressionEventSink eventSink,
                                       bool writeStatusFile)
    : statusPath_{std::move(statusPath)},
      eventSink_{std::move(eventSink)},
      writeStatusFile_{writeStatusFile},
      readers_{CreateProgressionReaders()} {
}

void ProgressionMonitor::CaptureManualSnapshot(
    const GameContext& context) const {
    const auto status = CaptureStatus(context);

    if (!ReplaceFileAtomically(statusPath_, status)) {
        LogWarning("Status", "Unable to replace diagnostic status file");
    }
}

std::string ProgressionMonitor::CaptureStatus(
    const GameContext& context) const {
    std::string status;

    for (const auto& reader : readers_) {
        AppendStatusSection(status, reader->CaptureStatus(context));
    }

    return status;
}

void ProgressionMonitor::Poll(const GameContext& context) {
    bool changed = false;
    std::string status;

    for (auto& reader : readers_) {
        ReaderUpdate update = reader->Poll(context);

        changed |= update.changed;
        AppendStatusSection(status, update.statusSection);

        if (!eventSink_) {
            continue;
        }

        for (const auto& event : update.events) {
            eventSink_(event);
        }
    }

    if (!changed || !writeStatusFile_) {
        return;
    }

    if (!ReplaceFileAtomically(statusPath_, status)) {
        LogWarning("Status", "Unable to replace diagnostic status file");
    }
}
}  // namespace sr2ap
