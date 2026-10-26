use std::collections::HashSet;

use serde::{Deserialize, Serialize};

pub const MAXIMUM_LIST_SIZE: usize = 256;
pub const MAXIMUM_NAME_SIZE: usize = 128;
/// Display-only text (player and item names in notifications) is cut to this
/// many UTF-8 bytes instead of rejecting the message.
pub const MAXIMUM_DISPLAY_SIZE: usize = 128;

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub enum IncomingKind {
    #[default]
    Invalid,
    Unknown,
    Item,
    ItemSent,
    SaveContext,
    SaveRevisionAcknowledgement,
    SessionReady,
    SessionEnd,
    DeathLink,
}

#[derive(Debug, Default, PartialEq, Eq)]
pub struct IncomingMessage {
    pub kind: IncomingKind,
    pub error: String,
    pub index: u64,
    pub name: String,
    /// Item: the player whose world the item was found in, empty when found
    /// in this player's own world. ItemSent: the receiving player. DeathLink:
    /// the player who died.
    pub player: String,
    /// DeathLink: why the other player died, empty when not given.
    pub cause: String,
    /// Archipelago item flags (progression, useful, trap).
    pub flags: u32,
    pub checksum: u32,
    pub next_index: u64,
    pub snapshot_known: bool,
    pub accepted: bool,
    pub protocol: u32,
    pub seed_name: String,
    pub team: u32,
    pub slot: u32,
    pub managed_unlockables: Vec<String>,
    pub managed_cheats: Vec<String>,
    pub persistent_items: Vec<String>,
    pub exclusive_respect: bool,
    pub block_vanilla_unlockables: bool,
    pub notoriety_traps: bool,
    pub death_link: bool,
    pub missions: bool,
    pub activities: bool,
    pub hitman: bool,
    pub chop_shop: bool,
    pub cds: bool,
    pub races: bool,
    pub style_level: bool,
    pub tags: bool,
    pub stunt_jumps: bool,
    pub taxi: bool,
    pub paramedic: bool,
    pub firefighter: bool,
    pub towing: bool,
    pub hoing: bool,
}

#[derive(Deserialize)]
struct Envelope {
    #[serde(rename = "type")]
    message_type: String,
}

#[derive(Deserialize)]
struct ItemMessage {
    #[serde(rename = "type")]
    _message_type: String,
    index: u64,
    name: String,
    #[serde(default)]
    sender: String,
    #[serde(default)]
    flags: u32,
}

#[derive(Deserialize)]
struct ItemSentMessage {
    #[serde(rename = "type")]
    _message_type: String,
    item: String,
    receiver: String,
    #[serde(default)]
    flags: u32,
}

#[derive(Deserialize)]
struct SaveContextMessage {
    #[serde(rename = "type")]
    _message_type: String,
    checksum: u32,
    next_index: u64,
    snapshot_known: bool,
    persistent_items: Vec<String>,
}

#[derive(Deserialize)]
struct SaveRevisionAcknowledgementMessage {
    #[serde(rename = "type")]
    _message_type: String,
    checksum: u32,
    next_index: u64,
    accepted: bool,
}

#[derive(Deserialize)]
struct DeathLinkMessage {
    #[serde(rename = "type")]
    _message_type: String,
    #[serde(default)]
    source: String,
    #[serde(default)]
    cause: String,
}

#[derive(Deserialize)]
struct Features {
    exclusive_respect: bool,
    block_vanilla_unlockables: bool,
    notoriety_traps: bool,
    death_link: bool,
}

#[derive(Deserialize)]
struct EnabledProgression {
    missions: bool,
    activities: bool,
    hitman: bool,
    chop_shop: bool,
    cds: bool,
    races: bool,
    #[serde(default)]
    style_level: bool,
    tags: bool,
    #[serde(default)]
    stunt_jumps: bool,
    #[serde(default)]
    taxi: bool,
    #[serde(default)]
    paramedic: bool,
    #[serde(default)]
    firefighter: bool,
    #[serde(default)]
    towing: bool,
    #[serde(default)]
    hoing: bool,
}

#[derive(Deserialize)]
struct SessionReadyMessage {
    #[serde(rename = "type")]
    _message_type: String,
    protocol: u32,
    seed_name: String,
    team: u32,
    slot: u32,
    managed_unlockables: Vec<String>,
    managed_cheats: Vec<String>,
    persistent_items: Vec<String>,
    features: Features,
    enabled_progression: EnabledProgression,
}

fn invalid(error: impl Into<String>) -> IncomingMessage {
    IncomingMessage {
        error: error.into(),
        ..IncomingMessage::default()
    }
}

fn validate_name(name: &str) -> Result<(), &'static str> {
    if name.is_empty() {
        return Err("name must not be empty");
    }
    if name.len() > MAXIMUM_NAME_SIZE {
        return Err("name exceeds 128 UTF-8 bytes");
    }
    Ok(())
}

fn display_text(mut text: String) -> String {
    if text.len() > MAXIMUM_DISPLAY_SIZE {
        let mut end = MAXIMUM_DISPLAY_SIZE;
        while !text.is_char_boundary(end) {
            end -= 1;
        }
        text.truncate(end);
    }
    text
}

fn validate_names(names: Vec<String>) -> Result<Vec<String>, &'static str> {
    if names.len() > MAXIMUM_LIST_SIZE {
        return Err("managed name list exceeds 256 entries");
    }
    let mut seen = HashSet::with_capacity(names.len());
    let mut result = Vec::with_capacity(names.len());
    for name in names {
        validate_name(&name)?;
        if seen.insert(name.clone()) {
            result.push(name);
        }
    }
    Ok(result)
}

fn decode<T: for<'de> Deserialize<'de>>(input: &str) -> Result<T, Box<IncomingMessage>> {
    serde_json::from_str(input).map_err(|error| Box::new(invalid(error.to_string())))
}

pub fn parse_incoming(input: &[u8]) -> IncomingMessage {
    let input = match std::str::from_utf8(input) {
        Ok(input) => input,
        Err(error) => return invalid(format!("message is not UTF-8: {error}")),
    };
    let envelope: Envelope = match decode(input) {
        Ok(envelope) => envelope,
        Err(error) => return *error,
    };

    match envelope.message_type.as_str() {
        "item" => match decode::<ItemMessage>(input) {
            Ok(item) => IncomingMessage {
                kind: IncomingKind::Item,
                index: item.index,
                name: item.name,
                player: display_text(item.sender),
                flags: item.flags,
                ..IncomingMessage::default()
            },
            Err(error) => *error,
        },
        "item_sent" => match decode::<ItemSentMessage>(input) {
            Ok(sent) => IncomingMessage {
                kind: IncomingKind::ItemSent,
                name: display_text(sent.item),
                player: display_text(sent.receiver),
                flags: sent.flags,
                ..IncomingMessage::default()
            },
            Err(error) => *error,
        },
        "save_context" => match decode::<SaveContextMessage>(input) {
            Ok(context) => {
                let persistent_items = match validate_names(context.persistent_items) {
                    Ok(names) => names,
                    Err(error) => return invalid(error),
                };
                IncomingMessage {
                    kind: IncomingKind::SaveContext,
                    checksum: context.checksum,
                    next_index: context.next_index,
                    snapshot_known: context.snapshot_known,
                    persistent_items,
                    ..IncomingMessage::default()
                }
            }
            Err(error) => *error,
        },
        "save_revision_ack" => match decode::<SaveRevisionAcknowledgementMessage>(input) {
            Ok(acknowledgement) => IncomingMessage {
                kind: IncomingKind::SaveRevisionAcknowledgement,
                checksum: acknowledgement.checksum,
                next_index: acknowledgement.next_index,
                accepted: acknowledgement.accepted,
                ..IncomingMessage::default()
            },
            Err(error) => *error,
        },
        "session_ready" => match decode::<SessionReadyMessage>(input) {
            Ok(session) => {
                if let Err(error) = validate_name(&session.seed_name) {
                    return invalid(error);
                }
                let managed_unlockables = match validate_names(session.managed_unlockables) {
                    Ok(names) => names,
                    Err(error) => return invalid(error),
                };
                let managed_cheats = match validate_names(session.managed_cheats) {
                    Ok(names) => names,
                    Err(error) => return invalid(error),
                };
                let persistent_items = match validate_names(session.persistent_items) {
                    Ok(names) => names,
                    Err(error) => return invalid(error),
                };
                IncomingMessage {
                    kind: IncomingKind::SessionReady,
                    protocol: session.protocol,
                    seed_name: session.seed_name,
                    team: session.team,
                    slot: session.slot,
                    managed_unlockables,
                    managed_cheats,
                    persistent_items,
                    exclusive_respect: session.features.exclusive_respect,
                    block_vanilla_unlockables: session.features.block_vanilla_unlockables,
                    notoriety_traps: session.features.notoriety_traps,
                    death_link: session.features.death_link,
                    missions: session.enabled_progression.missions,
                    activities: session.enabled_progression.activities,
                    hitman: session.enabled_progression.hitman,
                    chop_shop: session.enabled_progression.chop_shop,
                    cds: session.enabled_progression.cds,
                    races: session.enabled_progression.races,
                    style_level: session.enabled_progression.style_level,
                    tags: session.enabled_progression.tags,
                    stunt_jumps: session.enabled_progression.stunt_jumps,
                    taxi: session.enabled_progression.taxi,
                    paramedic: session.enabled_progression.paramedic,
                    firefighter: session.enabled_progression.firefighter,
                    towing: session.enabled_progression.towing,
                    hoing: session.enabled_progression.hoing,
                    ..IncomingMessage::default()
                }
            }
            Err(error) => *error,
        },
        "session_end" => IncomingMessage {
            kind: IncomingKind::SessionEnd,
            ..IncomingMessage::default()
        },
        "death_link" => match decode::<DeathLinkMessage>(input) {
            Ok(death) => IncomingMessage {
                kind: IncomingKind::DeathLink,
                player: display_text(death.source),
                cause: display_text(death.cause),
                ..IncomingMessage::default()
            },
            Err(error) => *error,
        },
        _ => IncomingMessage {
            kind: IncomingKind::Unknown,
            ..IncomingMessage::default()
        },
    }
}

#[derive(Serialize)]
struct ProgressionEvent<'category, 'a> {
    #[serde(rename = "type")]
    message_type: &'static str,
    category: &'category str,
    key: &'a str,
    previous: u32,
    current: u32,
}

pub fn serialize_progression_event(
    category: &str,
    key: &str,
    previous: u32,
    current: u32,
) -> String {
    serde_json::to_string(&ProgressionEvent {
        message_type: "progression",
        category,
        key,
        previous,
        current,
    })
    .unwrap_or_default()
}

#[derive(Serialize)]
struct ListOpened<'a> {
    #[serde(rename = "type")]
    message_type: &'static str,
    category: &'a str,
    list: u32,
}

/// A started Hitman or Chop Shop list was shown in game. `list` is one-based,
/// matching the `<list>` in `HITMAN_LOC_<list>_<n>` and
/// `CHOP_SHOP_TARGET_<list>_<n>`. The client hints that list's locations.
pub fn serialize_list_opened(category: &str, list: u32) -> String {
    serde_json::to_string(&ListOpened {
        message_type: "list_opened",
        category,
        list,
    })
    .unwrap_or_default()
}

#[derive(Serialize)]
struct ItemAcknowledgement {
    accepted: bool,
    index: u64,
    #[serde(rename = "type")]
    message_type: &'static str,
}

pub fn serialize_item_acknowledgement(index: u64, accepted: bool) -> String {
    serde_json::to_string(&ItemAcknowledgement {
        accepted,
        index,
        message_type: "item_ack",
    })
    .unwrap_or_default()
}

#[derive(Serialize)]
struct Death {
    #[serde(rename = "type")]
    message_type: &'static str,
}

/// The local player died. The client turns this into the Archipelago
/// DeathLink bounce.
pub fn serialize_death() -> String {
    serde_json::to_string(&Death {
        message_type: "death",
    })
    .unwrap_or_default()
}

#[derive(Serialize)]
struct SessionRejection<'a> {
    message: &'a str,
    reason: &'a str,
    #[serde(rename = "type")]
    message_type: &'static str,
}

pub fn serialize_session_rejection(reason: &str, message: &str) -> String {
    serde_json::to_string(&SessionRejection {
        message,
        reason,
        message_type: "session_rejected",
    })
    .unwrap_or_default()
}

#[derive(Serialize)]
struct GameContext {
    checksum: Option<u32>,
    needs_cursor: bool,
    next_index: u64,
    provisional: bool,
    snapshot_known: bool,
    persistent_items: Vec<String>,
    #[serde(rename = "type")]
    message_type: &'static str,
}

pub fn serialize_game_context_with_snapshot(
    checksum: Option<u32>,
    next_index: u64,
    provisional: bool,
    needs_cursor: bool,
    snapshot_known: bool,
    persistent_items: Vec<String>,
) -> String {
    serde_json::to_string(&GameContext {
        checksum,
        needs_cursor,
        next_index,
        provisional,
        snapshot_known,
        persistent_items,
        message_type: "game_context",
    })
    .unwrap_or_default()
}

#[derive(Serialize)]
struct SaveRevision {
    checksum: u32,
    next_index: u64,
    #[serde(rename = "type")]
    message_type: &'static str,
}

pub fn serialize_save_revision(checksum: u32, next_index: u64) -> String {
    serde_json::to_string(&SaveRevision {
        checksum,
        next_index,
        message_type: "save_revision",
    })
    .unwrap_or_default()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parser_should_read_item_sender_and_flags_with_defaults() {
        let item = parse_incoming(
            br#"{"type":"item","index":2,"name":"Shotgun","sender":"Player2","flags":1}"#,
        );
        assert_eq!(item.kind, IncomingKind::Item);
        assert_eq!(item.player, "Player2");
        assert_eq!(item.flags, 1);

        let own = parse_incoming(br#"{"type":"item","index":2,"name":"Shotgun"}"#);
        assert_eq!(own.kind, IncomingKind::Item);
        assert!(own.player.is_empty());
        assert_eq!(own.flags, 0);
    }

    #[test]
    fn parser_should_read_item_sent_and_truncate_display_text() {
        let long_item = "é".repeat(MAXIMUM_DISPLAY_SIZE);
        let input = format!(
            r#"{{"type":"item_sent","item":"{long_item}","receiver":"Player2","flags":4}}"#
        );
        let sent = parse_incoming(input.as_bytes());
        assert_eq!(sent.kind, IncomingKind::ItemSent);
        assert_eq!(sent.player, "Player2");
        assert_eq!(sent.flags, 4);
        assert_eq!(sent.name.len(), MAXIMUM_DISPLAY_SIZE);
        assert!(sent.name.chars().all(|c| c == 'é'));

        assert_eq!(
            parse_incoming(br#"{"type":"item_sent","item":"Shotgun"}"#).kind,
            IncomingKind::Invalid
        );
    }

    #[test]
    fn parser_should_reject_invalid_numeric_fields() {
        assert_eq!(
            parse_incoming(br#"{"type":"item","index":-1,"name":"x"}"#).kind,
            IncomingKind::Invalid
        );
        assert_eq!(
            parse_incoming(br#"{"type":"save_context","checksum":4294967296,"next_index":1}"#).kind,
            IncomingKind::Invalid
        );
    }

    #[test]
    fn parser_should_distinguish_unknown_and_malformed_messages() {
        assert_eq!(
            parse_incoming(br#"{"type":"future_message","value":1}"#).kind,
            IncomingKind::Unknown
        );
        let malformed = parse_incoming(b"not json");
        assert_eq!(malformed.kind, IncomingKind::Invalid);
        assert!(!malformed.error.is_empty());
        assert_eq!(parse_incoming(&[0xff]).kind, IncomingKind::Invalid);
    }

    #[test]
    fn parser_should_read_reconciled_save_snapshot() {
        let message = parse_incoming(
            br#"{
          "type":"save_context","checksum":42,"next_index":3,
          "snapshot_known":true,"persistent_items":["Tag Pass"]}
        "#,
        );

        assert_eq!(message.kind, IncomingKind::SaveContext);
        assert!(message.snapshot_known);
        assert_eq!(message.next_index, 3);
        assert_eq!(message.persistent_items, ["Tag Pass"]);
    }

    #[test]
    fn session_should_enforce_utf8_byte_and_list_limits() {
        assert!(validate_name(&"x".repeat(MAXIMUM_NAME_SIZE)).is_ok());
        assert!(validate_name(&"é".repeat(MAXIMUM_NAME_SIZE / 2)).is_ok());
        assert!(validate_name(&"é".repeat(MAXIMUM_NAME_SIZE / 2 + 1)).is_err());
        assert!(validate_names(vec!["x".to_owned(); MAXIMUM_LIST_SIZE]).is_ok());
        assert!(validate_names(vec!["x".to_owned(); MAXIMUM_LIST_SIZE + 1]).is_err());
    }

    #[test]
    fn parser_should_accept_unknown_fields_and_session_end_payloads() {
        assert_eq!(
            parse_incoming(br#"{"type":"session_end","future":true}"#).kind,
            IncomingKind::SessionEnd
        );
        let item =
            parse_incoming(br#"{"type":"item","index":18446744073709551615,"name":"","future":1}"#);
        assert_eq!(item.kind, IncomingKind::Item);
        assert_eq!(item.index, u64::MAX);
        assert!(item.name.is_empty());
    }

    #[test]
    fn session_should_deduplicate_names_in_first_seen_order() {
        let message = parse_incoming(br#"{
          "type":"session_ready","protocol":8,"seed_name":"seed","team":1,"slot":2,
          "managed_unlockables":["Taxi","Taxi","Boat"],"managed_cheats":[],
          "persistent_items":["Tag Pass","Tag Pass"],
          "features":{"exclusive_respect":true,"block_vanilla_unlockables":false,"notoriety_traps":true,"death_link":true},
          "enabled_progression":{"missions":true,"activities":false,"hitman":true,"chop_shop":false,"cds":true,"races":true,"tags":true}}
        "#);
        assert_eq!(message.kind, IncomingKind::SessionReady);
        assert_eq!(message.managed_unlockables, ["Taxi", "Boat"]);
        assert_eq!(message.persistent_items, ["Tag Pass"]);
        assert!(message.death_link);
    }

    #[test]
    fn session_ready_should_require_death_link_feature() {
        let message = parse_incoming(br#"{
          "type":"session_ready","protocol":8,"seed_name":"seed","team":1,"slot":2,
          "managed_unlockables":[],"managed_cheats":[],"persistent_items":[],
          "features":{"exclusive_respect":true,"block_vanilla_unlockables":false,"notoriety_traps":true},
          "enabled_progression":{"missions":true,"activities":false,"hitman":true,"chop_shop":false,"cds":true,"races":true,"tags":true}}
        "#);
        assert_eq!(message.kind, IncomingKind::Invalid);
    }

    #[test]
    fn parser_should_read_death_link_with_defaults_and_truncate_text() {
        let death = parse_incoming(
            br#"{"type":"death_link","source":"Player2","cause":"Player2 fell off a cliff"}"#,
        );
        assert_eq!(death.kind, IncomingKind::DeathLink);
        assert_eq!(death.player, "Player2");
        assert_eq!(death.cause, "Player2 fell off a cliff");

        let bare = parse_incoming(br#"{"type":"death_link"}"#);
        assert_eq!(bare.kind, IncomingKind::DeathLink);
        assert!(bare.player.is_empty());
        assert!(bare.cause.is_empty());

        let long_cause = "x".repeat(MAXIMUM_DISPLAY_SIZE * 2);
        let input = format!(r#"{{"type":"death_link","source":"P","cause":"{long_cause}"}}"#);
        assert_eq!(
            parse_incoming(input.as_bytes()).cause.len(),
            MAXIMUM_DISPLAY_SIZE
        );

        assert_eq!(
            parse_incoming(br#"{"type":"death_link","source":7}"#).kind,
            IncomingKind::Invalid
        );
    }

    #[test]
    fn serializers_should_emit_valid_expected_json() {
        let value: serde_json::Value = serde_json::from_str(&serialize_game_context_with_snapshot(
            None,
            4,
            true,
            false,
            false,
            Vec::new(),
        ))
        .unwrap();
        assert_eq!(value["type"], "game_context");
        assert!(value["checksum"].is_null());
        assert_eq!(value["next_index"], 4);
        assert_eq!(value["snapshot_known"], false);
        assert_eq!(value["persistent_items"], serde_json::json!([]));

        let rejection: serde_json::Value =
            serde_json::from_str(&serialize_session_rejection("reason", "message")).unwrap();
        assert_eq!(rejection["type"], "session_rejected");
        assert_eq!(rejection["reason"], "reason");

        let death: serde_json::Value = serde_json::from_str(&serialize_death()).unwrap();
        assert_eq!(death, serde_json::json!({"type": "death"}));

        let list: serde_json::Value =
            serde_json::from_str(&serialize_list_opened("chop_shop", 3)).unwrap();
        assert_eq!(
            list,
            serde_json::json!({"type": "list_opened", "category": "chop_shop", "list": 3})
        );
    }
}
