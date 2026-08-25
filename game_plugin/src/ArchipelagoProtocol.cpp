#include "sr2ap/ArchipelagoProtocol.hpp"

#include <cstdint>

#include "rust/cxx.h"
#include "sr2ap/src/ffi/protocol_ffi.rs.h"

namespace sr2ap {
namespace {
::rust::Slice<const std::uint8_t> Bytes(const std::string_view value) {
    return {reinterpret_cast<const std::uint8_t*>(value.data()), value.size()};
}

std::string ToString(const ::rust::String& value) {
    return {value.data(), value.size()};
}

std::vector<std::string> ToStrings(const ::rust::Vec<::rust::String>& values) {
    std::vector<std::string> result;
    result.reserve(values.size());
    for (const auto& value : values) {
        result.push_back(ToString(value));
    }
    return result;
}

std::uint8_t ToWireKind(const ProgressionKind kind) noexcept {
    switch (kind) {
        case ProgressionKind::Hitman:
            return 0;
        case ProgressionKind::ChopShop:
            return 1;
        case ProgressionKind::Mission:
            return 2;
        case ProgressionKind::Activity:
            return 3;
        case ProgressionKind::Racing:
            return 4;
        case ProgressionKind::Cd:
            return 5;
    }
    return 0xFF;
}
}  // namespace

IncomingMessage ParseIncomingMessage(const std::string_view message) {
    auto parsed = rust::protocol_parse_incoming(Bytes(message));
    IncomingMessage result;
    result.error = ToString(parsed.error);

    switch (parsed.kind) {
        case rust::IncomingMessageKind::Invalid:
            result.kind = IncomingMessageKind::Invalid;
            break;
        case rust::IncomingMessageKind::Unknown:
            result.kind = IncomingMessageKind::Unknown;
            break;
        case rust::IncomingMessageKind::Item:
            result.kind = IncomingMessageKind::Item;
            result.item =
                ReceivedItemMessage{parsed.index, ToString(parsed.name)};
            break;
        case rust::IncomingMessageKind::SaveContext:
            result.kind = IncomingMessageKind::SaveContext;
            result.saveContext =
                SaveContextMessage{parsed.checksum, parsed.next_index};
            break;
        case rust::IncomingMessageKind::SaveRevisionAcknowledgement:
            result.kind = IncomingMessageKind::SaveRevisionAcknowledgement;
            result.saveRevisionAcknowledgement =
                SaveRevisionAcknowledgementMessage{
                    parsed.checksum, parsed.next_index, parsed.accepted};
            break;
        case rust::IncomingMessageKind::SessionReady:
            result.kind = IncomingMessageKind::SessionReady;
            result.session = SessionReadyMessage{
                parsed.protocol,
                ToString(parsed.seed_name),
                parsed.team,
                parsed.slot,
                ToStrings(parsed.managed_unlockables),
                ToStrings(parsed.managed_cheats),
                parsed.exclusive_respect,
                parsed.block_vanilla_unlockables,
                parsed.notoriety_traps,
                parsed.missions,
                parsed.activities,
                parsed.hitman,
                parsed.chop_shop,
                parsed.cds,
                parsed.races,
            };
            break;
        case rust::IncomingMessageKind::SessionEnd:
            result.kind = IncomingMessageKind::SessionEnd;
            break;
        default:
            result.kind = IncomingMessageKind::Invalid;
            result.error =
                "Rust protocol parser returned an invalid message kind";
            break;
    }
    return result;
}

std::string SerializeProgressionEvent(const ProgressionEvent& event) {
    return ToString(rust::protocol_serialize_progression_event(
        ToWireKind(event.kind), Bytes(event.key), event.previous,
        event.current));
}

std::optional<ReceivedItemMessage> ParseReceivedItemMessage(
    const std::string_view message) {
    return ParseIncomingMessage(message).item;
}

std::string SerializeItemAcknowledgement(const std::uint64_t index,
                                         const bool accepted) {
    return ToString(
        rust::protocol_serialize_item_acknowledgement(index, accepted));
}

std::string SerializeSessionReject(const std::string_view reason,
                                   const std::string_view message) {
    return ToString(rust::protocol_serialize_session_rejection(Bytes(reason),
                                                               Bytes(message)));
}

std::optional<SaveContextMessage> ParseSaveContextMessage(
    const std::string_view message) {
    return ParseIncomingMessage(message).saveContext;
}

std::optional<SaveRevisionAcknowledgementMessage>
ParseSaveRevisionAcknowledgementMessage(const std::string_view message) {
    return ParseIncomingMessage(message).saveRevisionAcknowledgement;
}

std::optional<SessionReadyMessage> ParseSessionReadyMessage(
    const std::string_view message) {
    return ParseIncomingMessage(message).session;
}

bool IsSessionEndMessage(const std::string_view message) {
    return ParseIncomingMessage(message).kind ==
           IncomingMessageKind::SessionEnd;
}

std::string SerializeGameContext(const std::optional<std::uint32_t> checksum,
                                 const std::uint64_t nextIndex,
                                 const bool provisional,
                                 const bool needsCursor) {
    return ToString(rust::protocol_serialize_game_context(
        checksum.has_value(), checksum.value_or(0), nextIndex, provisional,
        needsCursor));
}

std::string SerializeSaveRevision(const std::uint32_t checksum,
                                  const std::uint64_t nextIndex) {
    return ToString(
        rust::protocol_serialize_save_revision(checksum, nextIndex));
}
}  // namespace sr2ap
