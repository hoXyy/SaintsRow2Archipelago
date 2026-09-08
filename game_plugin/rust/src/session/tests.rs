use super::*;
use std::sync::atomic::{AtomicU64, Ordering};

struct Fixture {
    _logger_guard: std::sync::MutexGuard<'static, ()>,
    runtime: SessionRuntime,
    directory: PathBuf,
}
impl Fixture {
    fn new() -> Self {
        let logger_guard = crate::logger::TEST_LOCK.lock().unwrap();
        static SEQUENCE: AtomicU64 = AtomicU64::new(0);
        let directory = std::env::temp_dir().join(format!(
            "sr2ap_session_{}_{}",
            std::process::id(),
            SEQUENCE.fetch_add(1, Ordering::Relaxed)
        ));
        std::fs::create_dir_all(&directory).unwrap();
        let runtime = SessionRuntime::new(true, directory.join("journal.json"));
        Self {
            runtime,
            directory,
            _logger_guard: logger_guard,
        }
    }
}
impl Drop for Fixture {
    fn drop(&mut self) {
        let _ = std::fs::remove_dir_all(&self.directory);
    }
}
fn session(seed: &str) -> IncomingMessage {
    IncomingMessage {
        kind: IncomingKind::SessionReady,
        protocol: 3,
        seed_name: seed.into(),
        slot: 1,
        missions: true,
        ..IncomingMessage::default()
    }
}
fn activate(runtime: &mut SessionRuntime) {
    let Some(GameplayRequest::InstallPolicies(_)) = runtime.handle_message(session("seed"), true)
    else {
        panic!("expected policies")
    };
    runtime.report_result(true).unwrap();
    runtime.update_readiness(false, true);
    runtime.outgoing.clear();
}
fn item(index: u64) -> IncomingMessage {
    IncomingMessage {
        kind: IncomingKind::Item,
        index,
        name: "item".into(),
        ..IncomingMessage::default()
    }
}
fn last_message(runtime: &SessionRuntime) -> serde_json::Value {
    serde_json::from_str(runtime.outgoing.last().unwrap()).unwrap()
}

#[test]
fn session_latches_only_after_policy_acceptance_and_reconnect_keeps_configuration() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    let Some(GameplayRequest::InstallPolicies(_)) = runtime.handle_message(session("seed"), true)
    else {
        panic!()
    };
    assert!(!runtime.communications_active());
    runtime.report_result(false).unwrap();
    assert!(runtime.session.is_none());
    activate(runtime);
    runtime.handle_message(
        IncomingMessage {
            kind: IncomingKind::SessionEnd,
            ..IncomingMessage::default()
        },
        true,
    );
    assert!(!runtime.active);
    let mut resumed = session("seed");
    resumed.missions = false;
    assert!(runtime.handle_message(resumed, true).is_none());
    assert!(runtime.active);
    assert!(runtime.session.as_ref().unwrap().missions);
    assert!(runtime.handle_message(session("different"), true).is_none());
    assert_eq!(last_message(runtime)["type"], "session_rejected");
    assert_eq!(runtime.session.as_ref().unwrap().seed_name, "seed");
}

#[test]
fn delivery_advances_only_on_acceptance_and_duplicates_do_not_activate() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    let Some(GameplayRequest::ActivateItem(_)) = runtime.handle_message(item(0), true) else {
        panic!()
    };
    assert_eq!(runtime.delivery.next_index, 0);
    runtime.report_result(false).unwrap();
    assert_eq!(runtime.delivery.next_index, 0);
    assert_eq!(last_message(runtime)["accepted"], false);
    let Some(GameplayRequest::ActivateItem(_)) = runtime.handle_message(item(0), true) else {
        panic!()
    };
    runtime.report_result(true).unwrap();
    assert_eq!(runtime.delivery.next_index, 1);
    assert!(runtime.handle_message(item(0), true).is_none());
    assert_eq!(last_message(runtime)["accepted"], true);
    assert!(runtime.handle_message(item(2), true).is_none());
    assert_eq!(last_message(runtime)["accepted"], false);
    assert!(runtime.handle_message(item(1), false).is_none());
    assert_eq!(last_message(runtime)["accepted"], false);
}

#[test]
fn save_load_invalidates_pending_result_and_requires_matching_cursor() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    runtime.set_save_monitoring(true);
    let Some(GameplayRequest::ActivateItem(_)) = runtime.handle_message(item(0), true) else {
        panic!()
    };
    runtime.save_loaded(42);
    assert!(runtime.report_result(true).is_err());
    assert!(runtime.handle_message(item(0), true).is_none());
    for checksum in [41, 42] {
        runtime.handle_message(
            IncomingMessage {
                kind: IncomingKind::SaveContext,
                checksum,
                next_index: 7,
                ..IncomingMessage::default()
            },
            true,
        );
        assert_eq!(runtime.delivery.ready(), checksum == 42);
    }
    assert_eq!(runtime.delivery.next_index, 7);
    runtime.update_readiness(true, false);
    assert_eq!(runtime.delivery.context, Context::Waiting);
    assert_eq!(runtime.delivery.checksum, None);
    runtime.update_readiness(false, true);
    assert_eq!(runtime.delivery.next_index, 0);
    assert_eq!(runtime.delivery.context, Context::Provisional);
}

#[test]
fn pending_revisions_block_delivery_until_exact_ack_is_persisted() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    runtime.delivery.next_index = 3;
    runtime.save_written(42);
    assert!(fixture.directory.join("journal.json").exists());
    runtime.begin_sync();
    assert!(runtime.revisions.pending);
    assert!(runtime.handle_message(item(3), true).is_none());
    for next_index in [2, 3] {
        runtime.handle_message(
            IncomingMessage {
                kind: IncomingKind::SaveRevisionAcknowledgement,
                checksum: 42,
                next_index,
                accepted: true,
                ..IncomingMessage::default()
            },
            true,
        );
        assert_eq!(runtime.revisions.pending, next_index != 3);
    }
    let mut restored = crate::revision_journal::RevisionJournal::default();
    restored
        .load(&fixture.directory.join("journal.json"))
        .unwrap();
    assert!(restored.pending("seed", 0, 1).is_empty());
}

#[test]
fn journal_failure_disables_delivery_and_session_acceptance() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    std::fs::create_dir(fixture.directory.join("journal.json.tmp")).unwrap();
    runtime.save_written(12);
    assert!(!runtime.revisions.available);
    assert!(runtime.handle_message(item(0), true).is_none());
    assert_eq!(last_message(runtime)["accepted"], false);
    assert!(runtime.handle_message(session("seed"), true).is_none());
}

#[test]
fn network_messages_wait_for_gameplay_result_and_disconnect_order_is_preserved() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    runtime.events.extend([
        NetworkEvent::Line(r#"{"type":"item","index":0,"name":"first"}"#.into()),
        NetworkEvent::Line(r#"{"type":"item","index":1,"name":"second"}"#.into()),
        NetworkEvent::Disconnected,
        NetworkEvent::Line(r#"{"type":"item","index":2,"name":"third"}"#.into()),
    ]);
    for expected in [0, 1] {
        let Some(GameplayRequest::ActivateItem(_)) = runtime.next_request(true) else {
            panic!()
        };
        assert!(runtime.next_request(true).is_none());
        runtime.report_result(true).unwrap();
        assert_eq!(runtime.delivery.next_index, expected + 1);
    }
    assert!(runtime.next_request(true).is_none());
    assert!(!runtime.active);
    assert_eq!(last_message(runtime)["accepted"], false);
}

#[test]
fn progression_is_filtered_by_latched_session_and_communications() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    runtime.progression(0, "target", 0, 1);
    assert!(runtime.outgoing.is_empty());
    runtime.progression(2, "mission", 0, 1);
    assert_eq!(last_message(runtime)["category"], "mission");
    runtime.active = false;
    runtime.progression(2, "mission", 0, 1);
    assert_eq!(runtime.outgoing.len(), 1);
}

#[test]
fn invalid_journal_and_unsupported_executable_reject_policy_installation() {
    let mut fixture = Fixture::new();
    fixture.runtime.game_supported = false;
    assert!(fixture
        .runtime
        .handle_message(session("seed"), true)
        .is_none());
    std::fs::write(fixture.directory.join("journal.json"), "invalid").unwrap();
    let mut runtime = SessionRuntime::new(true, fixture.directory.join("journal.json"));
    assert!(runtime.handle_message(session("seed"), true).is_none());
}

#[test]
fn maximum_cursor_is_rejected_without_execution_or_wraparound() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    runtime.delivery.next_index = u64::MAX;
    assert!(runtime.handle_message(item(u64::MAX), true).is_none());
    assert_eq!(runtime.delivery.next_index, u64::MAX);
    assert_eq!(last_message(runtime)["accepted"], false);
}
