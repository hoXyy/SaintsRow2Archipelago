use crate::protocol;

#[cxx::bridge(namespace = "sr2ap::rust")]
pub(crate) mod bridge {
    enum IncomingMessageKind {
        Invalid,
        Unknown,
        Item,
        SaveContext,
        SaveRevisionAcknowledgement,
        SessionReady,
        SessionEnd,
    }

    struct IncomingMessage {
        kind: IncomingMessageKind,
        error: String,
        index: u64,
        name: String,
        checksum: u32,
        next_index: u64,
        accepted: bool,
        protocol: u32,
        seed_name: String,
        team: u32,
        slot: u32,
        managed_unlockables: Vec<String>,
        managed_cheats: Vec<String>,
        exclusive_respect: bool,
        block_vanilla_unlockables: bool,
        notoriety_traps: bool,
        missions: bool,
        activities: bool,
        hitman: bool,
        chop_shop: bool,
        cds: bool,
        races: bool,
        style_level: bool,
    }

    extern "Rust" {
        fn protocol_parse_incoming(message: &[u8]) -> IncomingMessage;
        fn protocol_serialize_progression_event(
            kind: u8,
            key: &[u8],
            previous: u32,
            current: u32,
        ) -> String;
        fn protocol_serialize_item_acknowledgement(index: u64, accepted: bool) -> String;
        fn protocol_serialize_session_rejection(reason: &[u8], message: &[u8]) -> String;
        fn protocol_serialize_game_context(
            has_checksum: bool,
            checksum: u32,
            next_index: u64,
            provisional: bool,
            needs_cursor: bool,
        ) -> String;
        fn protocol_serialize_save_revision(checksum: u32, next_index: u64) -> String;
    }
}

fn protocol_parse_incoming(message: &[u8]) -> bridge::IncomingMessage {
    let message = protocol::parse_incoming(message);
    bridge::IncomingMessage {
        kind: match message.kind {
            protocol::IncomingKind::Invalid => bridge::IncomingMessageKind::Invalid,
            protocol::IncomingKind::Unknown => bridge::IncomingMessageKind::Unknown,
            protocol::IncomingKind::Item => bridge::IncomingMessageKind::Item,
            protocol::IncomingKind::SaveContext => bridge::IncomingMessageKind::SaveContext,
            protocol::IncomingKind::SaveRevisionAcknowledgement => {
                bridge::IncomingMessageKind::SaveRevisionAcknowledgement
            }
            protocol::IncomingKind::SessionReady => bridge::IncomingMessageKind::SessionReady,
            protocol::IncomingKind::SessionEnd => bridge::IncomingMessageKind::SessionEnd,
        },
        error: message.error,
        index: message.index,
        name: message.name,
        checksum: message.checksum,
        next_index: message.next_index,
        accepted: message.accepted,
        protocol: message.protocol,
        seed_name: message.seed_name,
        team: message.team,
        slot: message.slot,
        managed_unlockables: message.managed_unlockables,
        managed_cheats: message.managed_cheats,
        exclusive_respect: message.exclusive_respect,
        block_vanilla_unlockables: message.block_vanilla_unlockables,
        notoriety_traps: message.notoriety_traps,
        missions: message.missions,
        activities: message.activities,
        hitman: message.hitman,
        chop_shop: message.chop_shop,
        cds: message.cds,
        races: message.races,
        style_level: message.style_level,
    }
}

fn utf8_lossy(input: &[u8]) -> std::borrow::Cow<'_, str> {
    String::from_utf8_lossy(input)
}

fn protocol_serialize_progression_event(
    kind: u8,
    key: &[u8],
    previous: u32,
    current: u32,
) -> String {
    protocol::serialize_progression_event(kind, &utf8_lossy(key), previous, current)
}

fn protocol_serialize_item_acknowledgement(index: u64, accepted: bool) -> String {
    protocol::serialize_item_acknowledgement(index, accepted)
}

fn protocol_serialize_session_rejection(reason: &[u8], message: &[u8]) -> String {
    protocol::serialize_session_rejection(&utf8_lossy(reason), &utf8_lossy(message))
}

fn protocol_serialize_game_context(
    has_checksum: bool,
    checksum: u32,
    next_index: u64,
    provisional: bool,
    needs_cursor: bool,
) -> String {
    protocol::serialize_game_context(
        has_checksum.then_some(checksum),
        next_index,
        provisional,
        needs_cursor,
    )
}

fn protocol_serialize_save_revision(checksum: u32, next_index: u64) -> String {
    protocol::serialize_save_revision(checksum, next_index)
}
