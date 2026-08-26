use std::{
    collections::{btree_map::Entry, BTreeMap},
    fs,
    io::ErrorKind,
    path::Path,
};

use serde::{Deserialize, Serialize};

const JOURNAL_VERSION: u32 = 1;
const MAXIMUM_FILE_SIZE: u64 = 1024 * 1024;
const MAXIMUM_SESSIONS: usize = 1024;
const MAXIMUM_REVISIONS_PER_SESSION: usize = 65_536;
const MAXIMUM_SESSION_KEY_SIZE: usize = 512;

type Revisions = BTreeMap<u32, u64>;

#[derive(Debug, Default)]
pub(crate) struct RevisionJournal {
    sessions: BTreeMap<String, Revisions>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) struct SaveRevision {
    pub checksum: u32,
    pub next_index: u64,
}

#[derive(Debug, Deserialize)]
struct DiskJournal {
    version: u32,
    sessions: BTreeMap<String, BTreeMap<String, u64>>,
}

#[derive(Serialize)]
struct SerializableJournal<'a> {
    sessions: BTreeMap<&'a str, BTreeMap<String, u64>>,
    version: u32,
}

#[derive(Debug, thiserror::Error)]
pub(crate) enum JournalError {
    #[error("could not inspect revision journal: {0}")]
    Inspect(#[source] std::io::Error),
    #[error("could not read revision journal: {0}")]
    Read(#[source] std::io::Error),
    #[error("revision journal exceeds the {MAXIMUM_FILE_SIZE}-byte limit")]
    FileTooLarge,
    #[error("invalid revision journal JSON: {0}")]
    InvalidJson(#[source] serde_json::Error),
    #[error("unsupported revision journal version {0}")]
    UnsupportedVersion(u32),
    #[error("revision journal contains too many sessions")]
    TooManySessions,
    #[error("revision journal session key is empty or too long")]
    InvalidSessionKey,
    #[error("revision journal session contains too many revisions")]
    TooManyRevisions,
    #[error("invalid revision checksum `{0}`")]
    InvalidChecksum(String),
    #[error("duplicate normalized revision checksum `{0:08X}`")]
    DuplicateChecksum(u32),
    #[error("session seed is not valid UTF-8")]
    InvalidSeed,
    #[error("could not serialize revision journal: {0}")]
    Serialize(#[source] serde_json::Error),
}

fn session_key(seed_name: &str, team: u32, slot: u32) -> String {
    format!("{seed_name}|{team}|{slot}")
}

fn parse_checksum(text: &str) -> Result<u32, JournalError> {
    if text.is_empty() || !text.bytes().all(|byte| byte.is_ascii_hexdigit()) {
        return Err(JournalError::InvalidChecksum(text.to_owned()));
    }
    u32::from_str_radix(text, 16).map_err(|_| JournalError::InvalidChecksum(text.to_owned()))
}

impl RevisionJournal {
    pub(crate) fn load(&mut self, path: &Path) -> Result<(), JournalError> {
        let metadata = match fs::metadata(path) {
            Ok(metadata) => metadata,
            Err(error) if error.kind() == ErrorKind::NotFound => {
                self.sessions.clear();
                return Ok(());
            }
            Err(error) => return Err(JournalError::Inspect(error)),
        };
        if metadata.len() > MAXIMUM_FILE_SIZE {
            return Err(JournalError::FileTooLarge);
        }

        let contents = fs::read(path).map_err(JournalError::Read)?;
        if contents.len() as u64 > MAXIMUM_FILE_SIZE {
            return Err(JournalError::FileTooLarge);
        }
        let disk: DiskJournal =
            serde_json::from_slice(&contents).map_err(JournalError::InvalidJson)?;
        if disk.version != JOURNAL_VERSION {
            return Err(JournalError::UnsupportedVersion(disk.version));
        }
        if disk.sessions.len() > MAXIMUM_SESSIONS {
            return Err(JournalError::TooManySessions);
        }

        let mut sessions = BTreeMap::new();
        for (key, disk_revisions) in disk.sessions {
            if key.is_empty() || key.len() > MAXIMUM_SESSION_KEY_SIZE {
                return Err(JournalError::InvalidSessionKey);
            }
            if disk_revisions.len() > MAXIMUM_REVISIONS_PER_SESSION {
                return Err(JournalError::TooManyRevisions);
            }
            let mut revisions = BTreeMap::new();
            for (checksum_text, next_index) in disk_revisions {
                let checksum = parse_checksum(&checksum_text)?;
                match revisions.entry(checksum) {
                    Entry::Vacant(entry) => {
                        entry.insert(next_index);
                    }
                    Entry::Occupied(_) => return Err(JournalError::DuplicateChecksum(checksum)),
                }
            }
            sessions.insert(key, revisions);
        }

        self.sessions = sessions;
        Ok(())
    }

    pub(crate) fn serialize(&self) -> Result<String, JournalError> {
        let sessions = self
            .sessions
            .iter()
            .map(|(key, revisions)| {
                let revisions = revisions
                    .iter()
                    .map(|(checksum, next_index)| (format!("{checksum:08X}"), *next_index))
                    .collect();
                (key.as_str(), revisions)
            })
            .collect();
        serde_json::to_string_pretty(&SerializableJournal {
            sessions,
            version: JOURNAL_VERSION,
        })
        .map_err(JournalError::Serialize)
    }

    pub(crate) fn record(
        &mut self,
        seed_name: &str,
        team: u32,
        slot: u32,
        checksum: u32,
        next_index: u64,
    ) {
        self.sessions
            .entry(session_key(seed_name, team, slot))
            .or_default()
            .insert(checksum, next_index);
    }

    pub(crate) fn acknowledge(
        &mut self,
        seed_name: &str,
        team: u32,
        slot: u32,
        checksum: u32,
        next_index: u64,
    ) -> bool {
        let key = session_key(seed_name, team, slot);
        let Entry::Occupied(mut session) = self.sessions.entry(key) else {
            return false;
        };
        if session.get().get(&checksum) != Some(&next_index) {
            return false;
        }
        session.get_mut().remove(&checksum);
        if session.get().is_empty() {
            session.remove();
        }
        true
    }

    pub(crate) fn pending(&self, seed_name: &str, team: u32, slot: u32) -> Vec<SaveRevision> {
        self.sessions
            .get(&session_key(seed_name, team, slot))
            .into_iter()
            .flat_map(|revisions| revisions.iter())
            .map(|(&checksum, &next_index)| SaveRevision {
                checksum,
                next_index,
            })
            .collect()
    }
}

pub(crate) fn decode_seed(seed_name: &[u8]) -> Result<&str, JournalError> {
    std::str::from_utf8(seed_name).map_err(|_| JournalError::InvalidSeed)
}

#[cfg(test)]
mod tests {
    use std::time::{SystemTime, UNIX_EPOCH};

    use super::*;

    fn temporary_path(name: &str) -> std::path::PathBuf {
        let unique = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        std::env::temp_dir().join(format!("sr2ap_{name}_{unique}.json"))
    }

    #[test]
    fn journal_should_round_trip_and_acknowledge_exact_cursor() {
        let mut journal = RevisionJournal::default();
        journal.record("seed", 0, 1, 0x1234_5678, 17);
        journal.record("seed", 0, 1, 0xABCD_EF01, 22);
        journal.record("seed", 0, 2, 0x1234_5678, 99);
        let path = temporary_path("round_trip");
        fs::write(&path, journal.serialize().unwrap()).unwrap();

        let mut restored = RevisionJournal::default();
        restored.load(&path).unwrap();
        fs::remove_file(path).unwrap();
        assert_eq!(restored.pending("seed", 0, 1).len(), 2);
        assert!(!restored.acknowledge("seed", 0, 1, 0x1234_5678, 18));
        assert!(restored.acknowledge("seed", 0, 1, 0x1234_5678, 17));
        assert_eq!(restored.pending("seed", 0, 2)[0].next_index, 99);
    }

    #[test]
    fn failed_load_should_leave_existing_state_unchanged() {
        let path = temporary_path("transactional");
        fs::write(
            &path,
            r#"{"version":1,"sessions":{"seed|0|1":{"not-hex":2}}}"#,
        )
        .unwrap();
        let mut journal = RevisionJournal::default();
        journal.record("seed", 0, 1, 1, 2);
        assert!(journal.load(&path).is_err());
        fs::remove_file(path).unwrap();
        assert_eq!(journal.pending("seed", 0, 1).len(), 1);
    }

    #[test]
    fn parser_should_reject_ambiguous_normalized_checksums() {
        let path = temporary_path("duplicate");
        fs::write(
            &path,
            r#"{"version":1,"sessions":{"seed|0|1":{"A":1,"0000000a":2}}}"#,
        )
        .unwrap();
        let mut journal = RevisionJournal::default();
        assert!(matches!(
            journal.load(&path),
            Err(JournalError::DuplicateChecksum(10))
        ));
        fs::remove_file(path).unwrap();
    }

    #[test]
    fn missing_file_should_clear_the_journal_and_succeed() {
        let path = temporary_path("missing");
        let mut journal = RevisionJournal::default();
        journal.record("seed", 0, 1, 1, 2);
        journal.load(&path).unwrap();
        assert!(journal.pending("seed", 0, 1).is_empty());
    }
}
