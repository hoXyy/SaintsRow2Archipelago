mod delivery;
mod revision_sync;

use crate::{
    protocol::{self, IncomingKind, IncomingMessage},
    revision_journal::SaveSnapshot,
    tcp_client::{NetworkEvent, TcpClient},
};
use delivery::{Context, Delivery};
use revision_sync::RevisionSync;
use std::{
    collections::{BTreeSet, VecDeque},
    io,
    path::PathBuf,
};

const SESSION_PROTOCOL: u32 = 5;

pub(crate) enum GameplayRequest {
    InstallPolicies(IncomingMessage),
    ResetPersistentItems,
    ReplayPersistentItem(String),
    ActivateItem(String),
}

enum Pending {
    Policies(IncomingMessage),
    ResetPersistentItems,
    ReplayPersistentItem(String),
    Item { index: u64, name: String },
}

#[derive(Default)]
enum RestoreState {
    #[default]
    None,
    AwaitingSnapshot,
    NeedsReset(VecDeque<String>),
    Replaying(VecDeque<String>),
    Failed,
}

pub(crate) struct SessionRuntime {
    client: Option<TcpClient>,
    events: VecDeque<NetworkEvent>,
    session: Option<IncomingMessage>,
    delivery: Delivery,
    revisions: RevisionSync,
    persistent_items: BTreeSet<String>,
    restore: RestoreState,
    restore_checksum: Option<u32>,
    save_load_pending: bool,
    defer_restore_once: bool,
    game_supported: bool,
    save_monitoring: bool,
    active: bool,
    pending: Option<Pending>,
    #[cfg(test)]
    outgoing: Vec<String>,
}

impl SessionRuntime {
    pub fn new(game_supported: bool, path: PathBuf) -> Self {
        Self {
            client: None,
            events: VecDeque::new(),
            session: None,
            delivery: Delivery::default(),
            revisions: RevisionSync::new(path),
            persistent_items: BTreeSet::new(),
            restore: RestoreState::None,
            restore_checksum: None,
            save_load_pending: false,
            defer_restore_once: false,
            game_supported,
            save_monitoring: false,
            active: false,
            pending: None,
            #[cfg(test)]
            outgoing: Vec::new(),
        }
    }

    pub fn set_save_monitoring(&mut self, installed: bool) {
        self.save_monitoring = installed;
    }

    pub fn communications_active(&self) -> bool {
        self.active
    }

    pub fn connect(&mut self, port: u16) -> io::Result<()> {
        self.client = Some(TcpClient::new(port)?);
        Ok(())
    }

    pub fn shutdown(&mut self) {
        self.client = None;
        self.events.clear();
        self.pending = None;
        self.active = false;
    }

    fn send(&mut self, line: String) {
        #[cfg(test)]
        self.outgoing.push(line.clone());
        if let Some(client) = &self.client {
            if let Err(error) = client.send_line(&line) {
                log::error!(target: "Network", "Could not queue outgoing message: {error}");
            }
        }
    }

    fn acknowledge(&mut self, index: u64, accepted: bool) {
        self.send(protocol::serialize_item_acknowledgement(index, accepted));
    }

    fn game_context(&mut self) {
        if !self.active || self.revisions.pending || self.delivery.context == Context::Waiting {
            return;
        }
        let snapshot = if self.delivery.context == Context::AwaitingCursor {
            self.session.as_ref().and_then(|session| {
                self.delivery.checksum.and_then(|checksum| {
                    self.revisions.journal.snapshot(
                        &session.seed_name,
                        session.team,
                        session.slot,
                        checksum,
                    )
                })
            })
        } else {
            None
        };
        let next_index = snapshot
            .as_ref()
            .map_or(self.delivery.next_index, |snapshot| snapshot.next_index);
        let snapshot_known = snapshot.is_some();
        let persistent_items = snapshot
            .map(|snapshot| snapshot.persistent_items)
            .unwrap_or_default();
        self.send(protocol::serialize_game_context_with_snapshot(
            self.delivery.checksum,
            next_index,
            self.delivery.context == Context::Provisional,
            self.delivery.context == Context::AwaitingCursor,
            snapshot_known,
            persistent_items,
        ));
    }

    fn send_revisions(&mut self) {
        let Some(session) = &self.session else {
            return;
        };
        for revision in self.revisions.revisions(session) {
            self.send(protocol::serialize_save_revision(
                revision.checksum,
                revision.next_index,
            ));
        }
    }

    fn begin_sync(&mut self) {
        let Some(session) = &self.session else {
            return;
        };
        let count = self.revisions.revisions(session).len();
        self.revisions.pending = count != 0;
        if self.revisions.pending {
            log::info!(target: "SaveRevision", "Synchronizing {count} durable revision(s) before item delivery");
        }
        self.send_revisions();
        self.game_context();
    }

    pub fn update_readiness(&mut self, main_menu: bool, loaded: bool) {
        if loaded && self.save_load_pending && self.delivery.context != Context::AwaitingCursor {
            self.save_load_pending = false;
        }

        if main_menu && !self.save_load_pending && self.delivery.context != Context::Waiting {
            self.delivery = Delivery::default();
            self.cancel_gameplay_request();
            self.persistent_items.clear();
            self.restore_checksum = None;
            self.restore = if self.session.is_some() {
                RestoreState::NeedsReset(VecDeque::new())
            } else {
                RestoreState::None
            };
            log::info!(target: "Items", "Gameplay ended; delivery context cleared");
        } else if loaded && self.delivery.context == Context::Waiting {
            self.delivery.context = Context::Provisional;
            log::info!(target: "Items", "Gameplay ready without save load; provisional cursor initialized");
            self.game_context();
        }
    }

    fn cancel_gameplay_request(&mut self) {
        if matches!(
            self.pending,
            Some(
                Pending::Item { .. }
                    | Pending::ResetPersistentItems
                    | Pending::ReplayPersistentItem(_)
            )
        ) {
            self.pending = None;
        }
    }

    fn prepare_restore(&mut self, items: Vec<String>) {
        self.persistent_items = items.iter().cloned().collect();
        self.restore = RestoreState::NeedsReset(items.into());
        self.defer_restore_once = false;
    }

    pub fn save_loaded(&mut self, checksum: u32) {
        self.cancel_gameplay_request();
        self.delivery.load(checksum);
        self.restore_checksum = Some(checksum);
        self.restore = RestoreState::AwaitingSnapshot;
        self.save_load_pending = true;
        log::info!(target: "SaveRevision", "Loaded checksum={checksum:08X}; awaiting AP cursor and persistent-item restore");
        self.game_context();
    }

    pub fn save_written(&mut self, checksum: u32) {
        if !self.delivery.ready() {
            log::warn!(target: "SaveRevision", "Generated save checksum before AP cursor was established");
            return;
        }
        self.delivery.checksum = Some(checksum);
        self.delivery.context = Context::ActiveRevision;
        let Some(session) = &self.session else {
            return;
        };
        if !self.revisions.available {
            return;
        }
        self.revisions.journal.record(
            &session.seed_name,
            session.team,
            session.slot,
            checksum,
            self.delivery.next_index,
            self.persistent_items.iter().cloned().collect(),
        );
        if self.revisions.persist() && self.active {
            self.send_revisions();
        }
    }

    pub fn progression(&mut self, category: &str, key: &str, previous: u32, current: u32) {
        let Some(session) = &self.session else {
            return;
        };

        let enabled = match category {
            "hitman" => session.hitman,
            "chop_shop" => session.chop_shop,
            "mission" => session.missions,
            "activity" => session.activities,
            "racing" => session.races,
            "cd" => session.cds,
            "style_level" => session.style_level,
            _ => false,
        };

        if self.active && enabled {
            self.send(protocol::serialize_progression_event(
                category, key, previous, current,
            ));
        }
    }

    fn next_restore_request(&mut self, interactive: bool) -> Option<GameplayRequest> {
        if !interactive || self.session.is_none() {
            return None;
        }
        if self.defer_restore_once {
            self.defer_restore_once = false;
            return None;
        }

        match &self.restore {
            RestoreState::NeedsReset(_) => {
                self.pending = Some(Pending::ResetPersistentItems);
                Some(GameplayRequest::ResetPersistentItems)
            }
            RestoreState::Replaying(items) => {
                let Some(item) = items.front().cloned() else {
                    self.restore = RestoreState::None;
                    log::info!(target: "Items", "Persistent-item restore completed");
                    return None;
                };
                self.pending = Some(Pending::ReplayPersistentItem(item.clone()));
                Some(GameplayRequest::ReplayPersistentItem(item))
            }
            RestoreState::None | RestoreState::AwaitingSnapshot | RestoreState::Failed => None,
        }
    }

    fn restore_blocks_delivery(&self) -> bool {
        !matches!(self.restore, RestoreState::None)
    }

    pub fn next_request(&mut self, interactive: bool) -> Option<GameplayRequest> {
        if self.pending.is_some() {
            return None;
        }
        if let Some(request) = self.next_restore_request(interactive) {
            return Some(request);
        }
        if let Some(client) = &mut self.client {
            self.events.extend(client.drain_events());
        }
        while let Some(event) = self.events.pop_front() {
            match event {
                NetworkEvent::Connected => {}
                NetworkEvent::Disconnected => {
                    self.active = false;
                    log::info!(target: "Session", "TCP disconnected; gameplay policy remains latched");
                }
                NetworkEvent::Line(line) => {
                    if let Some(request) =
                        self.handle_message(protocol::parse_incoming(line.as_bytes()), interactive)
                    {
                        return Some(request);
                    }
                }
            }
        }
        None
    }

    fn handle_message(
        &mut self,
        message: IncomingMessage,
        interactive: bool,
    ) -> Option<GameplayRequest> {
        match message.kind {
            IncomingKind::SessionReady => {
                if message.protocol != SESSION_PROTOCOL
                    || !self.game_supported
                    || !self.revisions.available
                {
                    log::warn!(target: "Session", "Rejected unsupported session protocol, executable, or revision journal");
                    return None;
                }
                if let Some(session) = &self.session {
                    if (session.seed_name.as_str(), session.team, session.slot)
                        != (message.seed_name.as_str(), message.team, message.slot)
                    {
                        self.send(protocol::serialize_session_rejection("session_reject", "Conflicting AP session detected by game integration plugin, please restart Saints Row 2."));
                        return None;
                    }
                    self.active = true;
                    log::info!(target: "Session", "Authenticated AP session resumed");
                    self.begin_sync();
                } else {
                    let policy = IncomingMessage {
                        managed_cheats: message.managed_cheats.clone(),
                        managed_unlockables: message.managed_unlockables.clone(),
                        persistent_items: message.persistent_items.clone(),
                        exclusive_respect: message.exclusive_respect,
                        block_vanilla_unlockables: message.block_vanilla_unlockables,
                        notoriety_traps: message.notoriety_traps,
                        ..IncomingMessage::default()
                    };
                    self.pending = Some(Pending::Policies(message));
                    return Some(GameplayRequest::InstallPolicies(policy));
                }
            }
            IncomingKind::SessionEnd => {
                self.active = false;
                log::info!(target: "Session", "AP communications ended; gameplay policy remains latched");
            }
            IncomingKind::SaveContext => {
                if self.active
                    && self.save_monitoring
                    && self.delivery.context == Context::AwaitingCursor
                    && self.delivery.checksum == Some(message.checksum)
                {
                    let session = self.session.as_ref()?;
                    if message
                        .persistent_items
                        .iter()
                        .any(|item| !session.persistent_items.contains(item))
                    {
                        log::error!(target: "SaveRevision", "Rejected save snapshot with an item outside the persistent-item policy checksum={:08X}", message.checksum);
                        return None;
                    }
                    let client_snapshot = SaveSnapshot {
                        next_index: message.next_index,
                        persistent_items: message.persistent_items,
                    };
                    let journal_snapshot = self.revisions.journal.snapshot(
                        &session.seed_name,
                        session.team,
                        session.slot,
                        message.checksum,
                    );
                    if let Some(snapshot) = journal_snapshot {
                        if !message.snapshot_known || snapshot != client_snapshot {
                            log::error!(target: "SaveRevision", "Rejected conflicting save snapshot checksum={:08X} journal_cursor={} client_cursor={}", message.checksum, snapshot.next_index, client_snapshot.next_index);
                            return None;
                        }
                    } else {
                        if !message.snapshot_known
                            && (client_snapshot.next_index != 0
                                || !client_snapshot.persistent_items.is_empty())
                        {
                            log::error!(target: "SaveRevision", "Rejected invalid unknown save snapshot checksum={:08X}", message.checksum);
                            return None;
                        }
                        if !self.revisions.recover_snapshot(
                            session,
                            message.checksum,
                            client_snapshot.clone(),
                        ) {
                            return None;
                        }
                        log::info!(target: "SaveRevision", "Recovered missing plugin save snapshot checksum={:08X} next_index={}", message.checksum, client_snapshot.next_index);
                    }
                    self.delivery.next_index = message.next_index;
                    self.delivery.context = Context::ActiveRevision;
                    self.prepare_restore(client_snapshot.persistent_items);
                    log::info!(target: "Items", "Save context ready checksum={:08X} next_index={}", message.checksum, message.next_index);
                    self.game_context();
                } else {
                    log::warn!(target: "Items", "Rejected save context for inactive checksum={:08X}", message.checksum);
                }
            }
            IncomingKind::SaveRevisionAcknowledgement => {
                let Some(session) = &self.session else {
                    return None;
                };
                if !message.accepted
                    || !self.revisions.journal.acknowledge(
                        &session.seed_name,
                        session.team,
                        session.slot,
                        message.checksum,
                        message.next_index,
                    )
                {
                    log::warn!(target: "SaveRevision", "Rejected or mismatched revision acknowledgement checksum={:08X}", message.checksum);
                    return None;
                }
                if self.revisions.persist() {
                    if let Some(session) = &self.session {
                        if self.revisions.pending && self.revisions.revisions(session).is_empty() {
                            self.revisions.pending = false;
                            self.game_context();
                        }
                    }
                }
            }
            IncomingKind::Item => {
                if !self.active
                    || !self.delivery.ready()
                    || !interactive
                    || !self.revisions.available
                    || self.revisions.pending
                    || self.restore_blocks_delivery()
                {
                    log::debug!(target: "Items", "Deferred item until delivery and restore are ready index={}", message.index);
                    self.acknowledge(message.index, false);
                } else if message.index < self.delivery.next_index {
                    log::info!(target: "Items", "Acknowledged duplicate index={}", message.index);
                    self.acknowledge(message.index, true);
                } else if message.index != self.delivery.next_index || message.index == u64::MAX {
                    self.acknowledge(message.index, false);
                } else {
                    let index = message.index;
                    let name = message.name;
                    self.pending = Some(Pending::Item {
                        index,
                        name: name.clone(),
                    });
                    return Some(GameplayRequest::ActivateItem(name));
                }
            }
            IncomingKind::Invalid => {
                log::warn!(target: "Protocol", "Rejected malformed AP message: {}", message.error)
            }
            IncomingKind::Unknown => {}
        }
        None
    }

    pub fn report_result(&mut self, accepted: bool) -> Result<(), &'static str> {
        match self.pending.take() {
            Some(Pending::Policies(session)) if accepted => {
                log::info!(target: "Session", "AP gameplay policy activated seed={} team={} slot={}", session.seed_name, session.team, session.slot);
                self.session = Some(session);
                self.active = true;
                self.begin_sync();
            }
            Some(Pending::Policies(_)) => {
                log::error!(target: "Session", "AP gameplay policy activation failed; session rejected")
            }
            Some(Pending::ResetPersistentItems) => {
                let RestoreState::NeedsReset(items) = std::mem::take(&mut self.restore) else {
                    return Err("persistent reset completed in an invalid state");
                };
                if accepted {
                    self.restore = RestoreState::Replaying(items);
                } else {
                    self.restore = RestoreState::NeedsReset(items);
                    self.defer_restore_once = true;
                    log::debug!(target: "Items", "Persistent reset was not ready; retrying on a later update");
                }
            }
            Some(Pending::ReplayPersistentItem(item)) => {
                let RestoreState::Replaying(items) = &mut self.restore else {
                    return Err("persistent replay completed in an invalid state");
                };
                if !accepted || items.front() != Some(&item) {
                    self.restore = RestoreState::Failed;
                    log::error!(target: "Items", "Persistent item replay failed for `{item}`; new item delivery blocked");
                } else {
                    items.pop_front();
                    if items.is_empty() {
                        self.restore = RestoreState::None;
                        log::info!(target: "Items", "Persistent-item restore completed");
                    }
                }
            }
            Some(Pending::Item { index, name }) => {
                if accepted {
                    self.delivery.next_index = index + 1;
                    if self
                        .session
                        .as_ref()
                        .is_some_and(|session| session.persistent_items.contains(&name))
                    {
                        self.persistent_items.insert(name);
                    }
                } else {
                    log::warn!(target: "Items", "Controller rejected received item index={index}");
                }
                self.acknowledge(index, accepted);
            }
            None => return Err("no pending gameplay request"),
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests;
