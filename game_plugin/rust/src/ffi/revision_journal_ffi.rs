use std::{ffi::OsString, os::windows::ffi::OsStringExt, path::PathBuf};

use crate::revision_journal::{self, JournalError, RevisionJournal};

#[cxx::bridge(namespace = "sr2ap::rust")]
pub(crate) mod bridge {
    struct SaveRevision {
        checksum: u32,
        next_index: u64,
    }

    extern "Rust" {
        type RevisionJournal;

        fn revision_journal_new() -> Box<RevisionJournal>;
        fn revision_journal_load(journal: &mut RevisionJournal, path: &[u16]) -> Result<()>;
        fn revision_journal_serialize(journal: &RevisionJournal) -> Result<String>;
        fn revision_journal_record(
            journal: &mut RevisionJournal,
            seed_name: &[u8],
            team: u32,
            slot: u32,
            checksum: u32,
            next_index: u64,
        ) -> Result<()>;
        fn revision_journal_acknowledge(
            journal: &mut RevisionJournal,
            seed_name: &[u8],
            team: u32,
            slot: u32,
            checksum: u32,
            next_index: u64,
        ) -> Result<bool>;
        fn revision_journal_pending(
            journal: &RevisionJournal,
            seed_name: &[u8],
            team: u32,
            slot: u32,
        ) -> Result<Vec<SaveRevision>>;
    }
}

fn revision_journal_new() -> Box<RevisionJournal> {
    Box::default()
}

fn revision_journal_load(journal: &mut RevisionJournal, path: &[u16]) -> Result<(), JournalError> {
    journal.load(&PathBuf::from(OsString::from_wide(path)))
}

fn revision_journal_serialize(journal: &RevisionJournal) -> Result<String, JournalError> {
    journal.serialize()
}

fn revision_journal_record(
    journal: &mut RevisionJournal,
    seed_name: &[u8],
    team: u32,
    slot: u32,
    checksum: u32,
    next_index: u64,
) -> Result<(), JournalError> {
    journal.record(
        revision_journal::decode_seed(seed_name)?,
        team,
        slot,
        checksum,
        next_index,
    );
    Ok(())
}

fn revision_journal_acknowledge(
    journal: &mut RevisionJournal,
    seed_name: &[u8],
    team: u32,
    slot: u32,
    checksum: u32,
    next_index: u64,
) -> Result<bool, JournalError> {
    Ok(journal.acknowledge(
        revision_journal::decode_seed(seed_name)?,
        team,
        slot,
        checksum,
        next_index,
    ))
}

fn revision_journal_pending(
    journal: &RevisionJournal,
    seed_name: &[u8],
    team: u32,
    slot: u32,
) -> Result<Vec<bridge::SaveRevision>, JournalError> {
    Ok(journal
        .pending(revision_journal::decode_seed(seed_name)?, team, slot)
        .into_iter()
        .map(|revision| bridge::SaveRevision {
            checksum: revision.checksum,
            next_index: revision.next_index,
        })
        .collect())
}
