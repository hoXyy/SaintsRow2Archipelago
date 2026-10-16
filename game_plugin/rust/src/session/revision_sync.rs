use crate::fs::replace_atomically;
use crate::protocol::IncomingMessage;
use crate::revision_journal::{RevisionJournal, SaveRevision, SaveSnapshot};
use std::{
    io,
    path::{Path, PathBuf},
};

pub(super) struct RevisionSync {
    pub journal: RevisionJournal,
    pub available: bool,
    pub pending: bool,
    path: PathBuf,
}

impl RevisionSync {
    pub fn new(path: PathBuf) -> Self {
        let mut journal = RevisionJournal::default();
        let available = match journal.load(&path) {
            Ok(()) => true,
            Err(error) => {
                log::error!(target: "SaveRevision", "Could not load durable revision journal: {error}; AP item delivery disabled");
                false
            }
        };
        Self {
            journal,
            available,
            pending: false,
            path,
        }
    }

    pub fn revisions(&self, session: &IncomingMessage) -> Vec<SaveRevision> {
        self.journal
            .pending(&session.seed_name, session.team, session.slot)
    }

    pub fn persist(&mut self) -> bool {
        let result = persist_journal(&self.path, &self.journal);
        if let Err(error) = result {
            self.available = false;
            log::error!(target: "SaveRevision", "Could not persist durable revision journal: {error}; AP item delivery disabled");
            return false;
        }
        true
    }

    pub fn recover_snapshot(
        &mut self,
        session: &IncomingMessage,
        checksum: u32,
        snapshot: SaveSnapshot,
    ) -> bool {
        let mut recovered = self.journal.clone();
        let result = recovered
            .store_snapshot(
                &session.seed_name,
                session.team,
                session.slot,
                checksum,
                snapshot,
            )
            .map_err(io::Error::other)
            .and_then(|()| persist_journal(&self.path, &recovered));
        if let Err(error) = result {
            self.available = false;
            log::error!(target: "SaveRevision", "Could not recover durable save snapshot: {error}; AP item delivery disabled");
            return false;
        }
        self.journal = recovered;
        true
    }
}

fn persist_journal(path: &Path, journal: &RevisionJournal) -> io::Result<()> {
    let contents = journal.serialize().map_err(io::Error::other)?;
    replace_atomically(path, contents.as_bytes())
}
