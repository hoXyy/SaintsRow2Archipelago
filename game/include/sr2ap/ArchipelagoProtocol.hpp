#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "sr2ap/ProgressionEventSink.hpp"

namespace sr2ap {
    struct ReceivedItemMessage {
        std::uint64_t index{};
        std::string name;
    };

    struct SaveContextMessage {
        std::uint32_t checksum{};
        std::uint64_t nextIndex{};
    };

    struct SaveRevisionAcknowledgementMessage {
        std::uint32_t checksum{};
        std::uint64_t nextIndex{};
        bool accepted{};
    };

    struct SessionReadyMessage {
        std::uint32_t protocol{};
        std::string seedName;
        std::uint32_t team{};
        std::uint32_t slot{};
        std::vector<std::string> managedUnlockables;
        std::vector<std::string> managedCheats;
        bool exclusiveRespect{};
        bool blockVanillaUnlockables{};
        bool notorietyTraps{};
        bool missions{};
        bool activities{};
        bool hitman{};
        bool chopShop{};
        bool cds{};
    };

    [[nodiscard]] std::string SerializeProgressionEvent(const ProgressionEvent& event);
    [[nodiscard]] std::optional<ReceivedItemMessage> ParseReceivedItemMessage(std::string_view message);
    [[nodiscard]] std::string SerializeItemAcknowledgement(std::uint64_t index, bool accepted);
    [[nodiscard]] std::optional<SaveContextMessage> ParseSaveContextMessage(std::string_view message);
    [[nodiscard]] std::optional<SaveRevisionAcknowledgementMessage> ParseSaveRevisionAcknowledgementMessage(
        std::string_view message);
    [[nodiscard]] std::optional<SessionReadyMessage> ParseSessionReadyMessage(std::string_view message);
    [[nodiscard]] bool IsSessionEndMessage(std::string_view message);
    [[nodiscard]] std::string SerializeGameContext(std::optional<std::uint32_t> checksum,
                                                   std::uint64_t nextIndex,
                                                   bool provisional,
                                                   bool needsCursor);
    [[nodiscard]] std::string SerializeSaveRevision(std::uint32_t checksum, std::uint64_t nextIndex);
    [[nodiscard]] std::string SerializeSessionReject(const std::string_view reason, const std::string_view message);
}  // namespace sr2ap
