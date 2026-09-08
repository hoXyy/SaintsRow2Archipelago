mod delivery;
mod revision_sync;

use crate::{
    protocol::{self, IncomingKind, IncomingMessage},
    tcp_client::{NetworkEvent, TcpClient},
};
use delivery::{Context, Delivery};
use revision_sync::RevisionSync;
use std::{collections::VecDeque, io, path::PathBuf};

pub(crate) enum GameplayRequest {
    InstallPolicies(IncomingMessage),
    ActivateItem(String),
}

enum Pending {
    Policies(IncomingMessage),
    Item(u64),
}

pub(crate) struct SessionRuntime {
    client: Option<TcpClient>,
    events: VecDeque<NetworkEvent>,
    session: Option<IncomingMessage>,
    delivery: Delivery,
    revisions: RevisionSync,
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
        self.send(protocol::serialize_game_context(
            self.delivery.checksum,
            self.delivery.next_index,
            self.delivery.context == Context::Provisional,
            self.delivery.context == Context::AwaitingCursor,
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
        if main_menu && self.delivery.context != Context::Waiting {
            self.delivery = Delivery::default();
            self.cancel_item();
            log::info!(target: "Items", "Gameplay ended; delivery context cleared");
        } else if loaded && self.delivery.context == Context::Waiting {
            self.delivery.context = Context::Provisional;
            log::info!(target: "Items", "Gameplay ready without save load; provisional cursor initialized");
            self.game_context();
        }
    }
    fn cancel_item(&mut self) {
        if matches!(self.pending, Some(Pending::Item { .. })) {
            self.pending = None;
        }
    }
    pub fn save_loaded(&mut self, checksum: u32) {
        self.cancel_item();
        self.delivery.load(checksum);
        log::info!(target: "SaveRevision", "Loaded checksum={checksum:08X}; awaiting AP cursor");
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
        );
        if self.revisions.persist() && self.active {
            self.send_revisions();
        }
    }
    pub fn progression(&mut self, kind: u8, key: &str, previous: u32, current: u32) {
        let Some(session) = &self.session else {
            return;
        };
        let enabled = match kind {
            0 => session.hitman,
            1 => session.chop_shop,
            2 => session.missions,
            3 => session.activities,
            4 => session.races,
            5 => session.cds,
            6 => session.style_level,
            _ => false,
        };
        if self.active && enabled {
            self.send(protocol::serialize_progression_event(
                kind, key, previous, current,
            ));
        }
    }

    pub fn next_request(&mut self, interactive: bool) -> Option<GameplayRequest> {
        if self.pending.is_some() {
            return None;
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
                if message.protocol != 3 || !self.game_supported || !self.revisions.available {
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
                    self.delivery.next_index = message.next_index;
                    self.delivery.context = Context::ActiveRevision;
                    log::info!(target: "Items", "Save context ready checksum={:08X} next_index={}", message.checksum, message.next_index);
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
                {
                    log::debug!(target: "Items", "Deferred item until delivery is ready index={}", message.index);
                    self.acknowledge(message.index, false);
                } else if message.index < self.delivery.next_index {
                    log::info!(target: "Items", "Acknowledged duplicate index={}", message.index);
                    self.acknowledge(message.index, true);
                } else if message.index != self.delivery.next_index || message.index == u64::MAX {
                    self.acknowledge(message.index, false);
                } else {
                    self.pending = Some(Pending::Item(message.index));
                    return Some(GameplayRequest::ActivateItem(message.name));
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
            Some(Pending::Item(index)) => {
                if accepted {
                    self.delivery.next_index = index + 1;
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
