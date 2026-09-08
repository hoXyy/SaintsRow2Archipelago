use crate::session::{GameplayRequest, SessionRuntime};
use std::{ffi::OsString, os::windows::ffi::OsStringExt, path::PathBuf};

#[cxx::bridge(namespace = "sr2ap::rust")]
pub(crate) mod bridge {
    enum GameplayRequestKind {
        None,
        InstallPolicies,
        ActivateItem,
    }
    struct SessionRequest {
        kind: GameplayRequestKind,
        name: String,
        managed_cheats: Vec<String>,
        managed_unlockables: Vec<String>,
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
        fn session_update_readiness(session: &mut SessionRuntime, main_menu: bool, loaded: bool);
        fn session_save_loaded(session: &mut SessionRuntime, checksum: u32);
        fn session_save_written(session: &mut SessionRuntime, checksum: u32);
        fn session_progression(
            session: &mut SessionRuntime,
            kind: u8,
            key: &[u8],
            previous: u32,
            current: u32,
        ) -> Result<()>;
        fn session_active(session: &SessionRuntime) -> bool;
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
        name: String::new(),
        managed_cheats: Vec::new(),
        managed_unlockables: Vec::new(),
        exclusive_respect: false,
        block_vanilla_unlockables: false,
        notoriety_traps: false,
    };
    match session.next_request(interactive) {
        Some(GameplayRequest::InstallPolicies(session)) => {
            result.kind = bridge::GameplayRequestKind::InstallPolicies;
            result.managed_cheats = session.managed_cheats;
            result.managed_unlockables = session.managed_unlockables;
            result.exclusive_respect = session.exclusive_respect;
            result.block_vanilla_unlockables = session.block_vanilla_unlockables;
            result.notoriety_traps = session.notoriety_traps;
        }
        Some(GameplayRequest::ActivateItem(name)) => {
            result.kind = bridge::GameplayRequestKind::ActivateItem;
            result.name = name;
        }
        None => {}
    }
    result
}
fn session_report_result(session: &mut SessionRuntime, accepted: bool) -> Result<(), &'static str> {
    session.report_result(accepted)
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
    kind: u8,
    key: &[u8],
    previous: u32,
    current: u32,
) -> Result<(), std::str::Utf8Error> {
    session.progression(kind, std::str::from_utf8(key)?, previous, current);
    Ok(())
}
fn session_active(session: &SessionRuntime) -> bool {
    session.communications_active()
}
fn session_shutdown(session: &mut SessionRuntime) {
    session.shutdown();
}
