use super::*;
use std::sync::atomic::{AtomicU64, Ordering};

struct Fixture {
    runtime: SessionRuntime,
    directory: PathBuf,
}
impl Fixture {
    fn new() -> Self {
        static SEQUENCE: AtomicU64 = AtomicU64::new(0);
        let directory = std::env::temp_dir().join(format!(
            "sr2ap_session_{}_{}",
            std::process::id(),
            SEQUENCE.fetch_add(1, Ordering::Relaxed)
        ));
        std::fs::create_dir_all(&directory).unwrap();
        let runtime = SessionRuntime::new(true, directory.join("journal.json"));
        Self { runtime, directory }
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
        protocol: SESSION_PROTOCOL,
        seed_name: seed.into(),
        slot: 1,
        missions: true,
        persistent_items: vec!["persistent".into()],
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

fn named_item(index: u64, name: &str) -> IncomingMessage {
    IncomingMessage {
        kind: IncomingKind::Item,
        index,
        name: name.into(),
        ..IncomingMessage::default()
    }
}

fn item(index: u64) -> IncomingMessage {
    named_item(index, "item")
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
    runtime.update_readiness(true, false);
    assert_eq!(runtime.delivery.context, Context::AwaitingCursor);
    for checksum in [41, 42] {
        runtime.handle_message(
            IncomingMessage {
                kind: IncomingKind::SaveContext,
                checksum,
                next_index: 7,
                snapshot_known: true,
                ..IncomingMessage::default()
            },
            true,
        );
        assert_eq!(runtime.delivery.ready(), checksum == 42);
    }
    assert_eq!(runtime.delivery.next_index, 7);
    runtime.update_readiness(true, false);
    assert_eq!(runtime.delivery.context, Context::ActiveRevision);
    runtime.update_readiness(false, true);
    assert_eq!(runtime.delivery.next_index, 7);
    assert_eq!(runtime.delivery.context, Context::ActiveRevision);
    runtime.update_readiness(true, false);
    assert_eq!(runtime.delivery.context, Context::Waiting);
    assert_eq!(runtime.delivery.checksum, None);
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
    runtime.progression("hitman", "target", 0, 1);
    assert!(runtime.outgoing.is_empty());
    runtime.progression("mission", "mission", 0, 1);
    assert_eq!(last_message(runtime)["category"], "mission");
    runtime.active = false;
    runtime.progression("mission", "mission", 0, 1);
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

#[test]
fn missing_plugin_snapshot_should_be_recovered_before_context_activation() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    runtime.set_save_monitoring(true);
    runtime.save_loaded(42);

    runtime.handle_message(
        IncomingMessage {
            kind: IncomingKind::SaveContext,
            checksum: 42,
            next_index: 1,
            snapshot_known: true,
            persistent_items: vec!["persistent".into()],
            ..IncomingMessage::default()
        },
        true,
    );

    assert_eq!(runtime.delivery.context, Context::ActiveRevision);
    assert_eq!(last_message(runtime)["needs_cursor"], false);
    assert_eq!(
        runtime
            .revisions
            .journal
            .snapshot("seed", 0, 1, 42)
            .unwrap(),
        crate::revision_journal::SaveSnapshot {
            next_index: 1,
            persistent_items: vec!["persistent".into()],
        }
    );
    assert!(runtime.revisions.journal.pending("seed", 0, 1).is_empty());
}

#[test]
fn failed_plugin_snapshot_recovery_should_not_activate_or_mutate_memory() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    runtime.set_save_monitoring(true);
    runtime.save_loaded(42);
    std::fs::create_dir(fixture.directory.join("journal.json.tmp")).unwrap();

    runtime.handle_message(
        IncomingMessage {
            kind: IncomingKind::SaveContext,
            checksum: 42,
            next_index: 1,
            snapshot_known: true,
            persistent_items: vec!["persistent".into()],
            ..IncomingMessage::default()
        },
        true,
    );

    assert_eq!(runtime.delivery.context, Context::AwaitingCursor);
    assert_eq!(runtime.revisions.journal.snapshot("seed", 0, 1, 42), None);
}

#[test]
fn conflicting_recovered_snapshot_should_leave_context_awaiting_resolution() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    runtime.delivery.next_index = 1;
    runtime.persistent_items.insert("persistent".into());
    runtime.save_written(42);
    runtime.set_save_monitoring(true);
    runtime.save_loaded(42);
    let outgoing_count = runtime.outgoing.len();

    runtime.handle_message(
        IncomingMessage {
            kind: IncomingKind::SaveContext,
            checksum: 42,
            next_index: 2,
            snapshot_known: true,
            persistent_items: vec!["persistent".into()],
            ..IncomingMessage::default()
        },
        true,
    );

    assert_eq!(runtime.delivery.context, Context::AwaitingCursor);
    assert_eq!(runtime.outgoing.len(), outgoing_count);
}

#[test]
fn persistent_items_should_round_trip_through_a_save_snapshot() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);

    let Some(GameplayRequest::ActivateItem(name)) =
        runtime.handle_message(named_item(0, "persistent"), true)
    else {
        panic!()
    };
    assert_eq!(name, "persistent");
    runtime.report_result(true).unwrap();
    runtime.save_written(42);

    let snapshot = runtime
        .revisions
        .journal
        .snapshot("seed", 0, 1, 42)
        .unwrap();
    assert_eq!(snapshot.next_index, 1);
    assert_eq!(snapshot.persistent_items, ["persistent"]);

    runtime.set_save_monitoring(true);
    runtime.save_loaded(42);
    runtime.handle_message(
        IncomingMessage {
            kind: IncomingKind::SaveContext,
            checksum: 42,
            next_index: 1,
            snapshot_known: true,
            persistent_items: vec!["persistent".into()],
            ..IncomingMessage::default()
        },
        true,
    );

    assert!(matches!(
        runtime.next_request(true),
        Some(GameplayRequest::ResetPersistentItems)
    ));
    runtime.report_result(true).unwrap();

    let Some(GameplayRequest::ReplayPersistentItem(name)) = runtime.next_request(true) else {
        panic!()
    };
    assert_eq!(name, "persistent");
    runtime.report_result(true).unwrap();

    assert_eq!(runtime.delivery.next_index, 1);
    assert!(matches!(runtime.restore, RestoreState::None));
}

#[test]
fn rejected_restore_should_retry_reset_but_block_after_replay_failure() {
    let mut fixture = Fixture::new();
    let runtime = &mut fixture.runtime;
    activate(runtime);
    runtime.persistent_items.insert("persistent".into());
    runtime.save_written(42);
    runtime.set_save_monitoring(true);
    runtime.save_loaded(42);
    runtime.handle_message(
        IncomingMessage {
            kind: IncomingKind::SaveContext,
            checksum: 42,
            snapshot_known: true,
            persistent_items: vec!["persistent".into()],
            ..IncomingMessage::default()
        },
        true,
    );

    assert!(matches!(
        runtime.next_request(true),
        Some(GameplayRequest::ResetPersistentItems)
    ));
    runtime.report_result(false).unwrap();
    assert!(runtime.next_request(true).is_none());
    assert!(matches!(
        runtime.next_request(true),
        Some(GameplayRequest::ResetPersistentItems)
    ));
    runtime.report_result(true).unwrap();

    assert!(matches!(
        runtime.next_request(true),
        Some(GameplayRequest::ReplayPersistentItem(_))
    ));
    runtime.report_result(false).unwrap();
    assert!(matches!(runtime.restore, RestoreState::Failed));

    runtime.delivery.context = Context::ActiveRevision;
    runtime.delivery.next_index = 1;
    assert!(runtime.handle_message(item(1), true).is_none());
    assert_eq!(last_message(runtime)["accepted"], false);
}
