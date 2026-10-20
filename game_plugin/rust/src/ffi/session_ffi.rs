use crate::session::{ConnectionState, GameplayRequest, Notification, SessionRuntime};
use std::{ffi::OsString, os::windows::ffi::OsStringExt, path::PathBuf};

#[cxx::bridge(namespace = "sr2ap::rust")]
pub(crate) mod bridge {
    enum GameplayRequestKind {
        None,
        InstallPolicies,
        ResetPersistentItems,
        ReplayPersistentItem,
        ActivateItem,
    }
    enum NotificationKind {
        None,
        ReceivedItem,
        SentItem,
    }
    struct SessionNotification {
        kind: NotificationKind,
        item: String,
        /// Sender for ReceivedItem (empty when found in this player's own
        /// world), receiver for SentItem.
        player: String,
        flags: u32,
    }
    enum ConnectionState {
        Disconnected,
        WaitingForSession,
        Active,
    }
    struct DeathLinkRequest {
        present: bool,
        source: String,
        cause: String,
    }
    struct SessionRequest {
        kind: GameplayRequestKind,
        id: u64,
        name: String,
        managed_cheats: Vec<String>,
        managed_unlockables: Vec<String>,
        persistent_items: Vec<String>,
        exclusive_respect: bool,
        block_vanilla_unlockables: bool,
        notoriety_traps: bool,
    }
    extern "Rust" {
        type SessionRuntime;
        fn session_new(game_supported: bool, path: &[u16]) -> Box<SessionRuntime>;
        fn session_set_save_monitoring(session: &mut SessionRuntime, installed: bool);
        fn session_connect(session: &mut SessionRuntime, port: u16) -> Result<()>;
        fn session_next_request(session: &mut SessionRuntime, interactive: bool) -> SessionRequest;
        fn session_report_result(session: &mut SessionRuntime, accepted: bool) -> Result<()>;
        fn session_complete_request(
            session: &mut SessionRuntime,
            id: u64,
            accepted: bool,
        ) -> Result<()>;
        fn session_update_readiness(session: &mut SessionRuntime, main_menu: bool, loaded: bool);
        fn session_save_loaded(session: &mut SessionRuntime, checksum: u32);
        fn session_save_written(session: &mut SessionRuntime, checksum: u32);
        fn session_progression(
            session: &mut SessionRuntime,
            category: &[u8],
            key: &[u8],
            previous: u32,
            current: u32,
        ) -> Result<()>;
        fn session_active(session: &SessionRuntime) -> bool;
        fn session_connection_state(session: &SessionRuntime) -> ConnectionState;
        fn session_next_notification(session: &mut SessionRuntime) -> SessionNotification;
        fn session_player_died(session: &mut SessionRuntime);
        fn session_next_death_link(session: &mut SessionRuntime) -> DeathLinkRequest;
        fn session_shutdown(session: &mut SessionRuntime);
    }
}
fn session_new(game_supported: bool, path: &[u16]) -> Box<SessionRuntime> {
    Box::new(SessionRuntime::new(
        game_supported,
        PathBuf::from(OsString::from_wide(path)),
    ))
}
fn session_set_save_monitoring(session: &mut SessionRuntime, installed: bool) {
    session.set_save_monitoring(installed);
}
fn session_connect(session: &mut SessionRuntime, port: u16) -> std::io::Result<()> {
    session.connect(port)
}
fn session_next_request(session: &mut SessionRuntime, interactive: bool) -> bridge::SessionRequest {
    let mut result = bridge::SessionRequest {
        kind: bridge::GameplayRequestKind::None,
        id: 0,
        name: String::new(),
        managed_cheats: Vec::new(),
        managed_unlockables: Vec::new(),
        persistent_items: Vec::new(),
        exclusive_respect: false,
        block_vanilla_unlockables: false,
        notoriety_traps: false,
    };
    match session.next_request(interactive) {
        Some(GameplayRequest::InstallPolicies(session)) => {
            result.kind = bridge::GameplayRequestKind::InstallPolicies;
            result.managed_cheats = session.managed_cheats;
            result.managed_unlockables = session.managed_unlockables;
            result.persistent_items = session.persistent_items;
            result.exclusive_respect = session.exclusive_respect;
            result.block_vanilla_unlockables = session.block_vanilla_unlockables;
            result.notoriety_traps = session.notoriety_traps;
        }
        Some(GameplayRequest::ActivateItem(name)) => {
            result.kind = bridge::GameplayRequestKind::ActivateItem;
            result.name = name;
        }
        Some(GameplayRequest::ResetPersistentItems) => {
            result.kind = bridge::GameplayRequestKind::ResetPersistentItems;
        }
        Some(GameplayRequest::ReplayPersistentItem(name)) => {
            result.kind = bridge::GameplayRequestKind::ReplayPersistentItem;
            result.name = name;
        }
        None => return result,
    }
    result.id = session.current_request_id();
    result
}
fn session_report_result(session: &mut SessionRuntime, accepted: bool) -> Result<(), &'static str> {
    session.report_result(accepted)
}
fn session_complete_request(
    session: &mut SessionRuntime,
    id: u64,
    accepted: bool,
) -> Result<(), &'static str> {
    session.complete_request(id, accepted)
}
fn session_update_readiness(session: &mut SessionRuntime, main_menu: bool, loaded: bool) {
    session.update_readiness(main_menu, loaded);
}
fn session_save_loaded(session: &mut SessionRuntime, checksum: u32) {
    session.save_loaded(checksum);
}
fn session_save_written(session: &mut SessionRuntime, checksum: u32) {
    session.save_written(checksum);
}
fn session_progression(
    session: &mut SessionRuntime,
    category: &[u8],
    key: &[u8],
    previous: u32,
    current: u32,
) -> Result<(), std::str::Utf8Error> {
    session.progression(
        std::str::from_utf8(category)?,
        std::str::from_utf8(key)?,
        previous,
        current,
    );
    Ok(())
}
fn session_active(session: &SessionRuntime) -> bool {
    session.communications_active()
}
fn session_connection_state(session: &SessionRuntime) -> bridge::ConnectionState {
    match session.connection_state() {
        ConnectionState::Disconnected => bridge::ConnectionState::Disconnected,
        ConnectionState::WaitingForSession => bridge::ConnectionState::WaitingForSession,
        ConnectionState::Active => bridge::ConnectionState::Active,
    }
}
fn session_next_notification(session: &mut SessionRuntime) -> bridge::SessionNotification {
    let (kind, item, player, flags) = match session.next_notification() {
        Some(Notification::ReceivedItem {
            item,
            sender,
            flags,
        }) => (bridge::NotificationKind::ReceivedItem, item, sender, flags),
        Some(Notification::SentItem {
            item,
            receiver,
            flags,
        }) => (bridge::NotificationKind::SentItem, item, receiver, flags),
        None => (
            bridge::NotificationKind::None,
            String::new(),
            String::new(),
            0,
        ),
    };
    bridge::SessionNotification {
        kind,
        item,
        player,
        flags,
    }
}
fn session_player_died(session: &mut SessionRuntime) {
    session.player_died();
}
fn session_next_death_link(session: &mut SessionRuntime) -> bridge::DeathLinkRequest {
    match session.next_death_link() {
        Some(death) => bridge::DeathLinkRequest {
            present: true,
            source: death.source,
            cause: death.cause,
        },
        None => bridge::DeathLinkRequest {
            present: false,
            source: String::new(),
            cause: String::new(),
        },
    }
}
fn session_shutdown(session: &mut SessionRuntime) {
    session.shutdown();
}
