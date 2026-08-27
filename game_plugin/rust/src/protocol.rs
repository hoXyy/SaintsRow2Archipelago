use std::collections::HashSet;

use serde::{Deserialize, Serialize};

pub const MAXIMUM_LIST_SIZE: usize = 256;
pub const MAXIMUM_NAME_SIZE: usize = 128;

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub enum IncomingKind {
    #[default]
    Invalid,
    Unknown,
    Item,
    SaveContext,
    SaveRevisionAcknowledgement,
    SessionReady,
    SessionEnd,
}

#[derive(Debug, Default, PartialEq, Eq)]
pub struct IncomingMessage {
    pub kind: IncomingKind,
    pub error: String,
    pub index: u64,
    pub name: String,
    pub checksum: u32,
    pub next_index: u64,
    pub accepted: bool,
    pub protocol: u32,
    pub seed_name: String,
    pub team: u32,
    pub slot: u32,
    pub managed_unlockables: Vec<String>,
    pub managed_cheats: Vec<String>,
    pub exclusive_respect: bool,
    pub block_vanilla_unlockables: bool,
    pub notoriety_traps: bool,
    pub missions: bool,
    pub activities: bool,
    pub hitman: bool,
    pub chop_shop: bool,
    pub cds: bool,
    pub races: bool,
    pub style_level: bool,
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
}

#[derive(Deserialize)]
struct SaveContextMessage {
    #[serde(rename = "type")]
    _message_type: String,
    checksum: u32,
    next_index: u64,
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
struct Features {
    exclusive_respect: bool,
    block_vanilla_unlockables: bool,
    notoriety_traps: bool,
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
                ..IncomingMessage::default()
            },
            Err(error) => *error,
        },
        "save_context" => match decode::<SaveContextMessage>(input) {
            Ok(context) => IncomingMessage {
                kind: IncomingKind::SaveContext,
                checksum: context.checksum,
                next_index: context.next_index,
                ..IncomingMessage::default()
            },
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
                IncomingMessage {
                    kind: IncomingKind::SessionReady,
                    protocol: session.protocol,
                    seed_name: session.seed_name,
                    team: session.team,
                    slot: session.slot,
                    managed_unlockables,
                    managed_cheats,
                    exclusive_respect: session.features.exclusive_respect,
                    block_vanilla_unlockables: session.features.block_vanilla_unlockables,
                    notoriety_traps: session.features.notoriety_traps,
                    missions: session.enabled_progression.missions,
                    activities: session.enabled_progression.activities,
                    hitman: session.enabled_progression.hitman,
                    chop_shop: session.enabled_progression.chop_shop,
                    cds: session.enabled_progression.cds,
                    races: session.enabled_progression.races,
                    style_level: session.enabled_progression.style_level,
                    ..IncomingMessage::default()
                }
            }
            Err(error) => *error,
        },
        "session_end" => IncomingMessage {
            kind: IncomingKind::SessionEnd,
            ..IncomingMessage::default()
        },
        _ => IncomingMessage {
            kind: IncomingKind::Unknown,
            ..IncomingMessage::default()
        },
    }
}

#[derive(Serialize)]
struct ProgressionEvent<'a> {
    #[serde(rename = "type")]
    message_type: &'static str,
    category: &'static str,
    key: &'a str,
    previous: u32,
    current: u32,
}

pub fn serialize_progression_event(kind: u8, key: &str, previous: u32, current: u32) -> String {
    let category = match kind {
        0 => "hitman",
        1 => "chop_shop",
        2 => "mission",
        3 => "activity",
        4 => "racing",
        5 => "cd",
        6 => "style_level",
        _ => "unknown",
    };
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
    #[serde(rename = "type")]
    message_type: &'static str,
}

pub fn serialize_game_context(
    checksum: Option<u32>,
    next_index: u64,
    provisional: bool,
    needs_cursor: bool,
) -> String {
    serde_json::to_string(&GameContext {
        checksum,
        needs_cursor,
        next_index,
        provisional,
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
          "type":"session_ready","protocol":3,"seed_name":"seed","team":1,"slot":2,
          "managed_unlockables":["Taxi","Taxi","Boat"],"managed_cheats":[],
          "features":{"exclusive_respect":true,"block_vanilla_unlockables":false,"notoriety_traps":true},
          "enabled_progression":{"missions":true,"activities":false,"hitman":true,"chop_shop":false,"cds":true,"races":true}}
        "#);
        assert_eq!(message.kind, IncomingKind::SessionReady);
        assert_eq!(message.managed_unlockables, ["Taxi", "Boat"]);
    }

    #[test]
    fn serializers_should_emit_valid_expected_json() {
        let value: serde_json::Value =
            serde_json::from_str(&serialize_game_context(None, 4, true, false)).unwrap();
        assert_eq!(value["type"], "game_context");
        assert!(value["checksum"].is_null());
        assert_eq!(value["next_index"], 4);

        let rejection: serde_json::Value =
            serde_json::from_str(&serialize_session_rejection("reason", "message")).unwrap();
        assert_eq!(rejection["type"], "session_rejected");
        assert_eq!(rejection["reason"], "reason");
    }
}
