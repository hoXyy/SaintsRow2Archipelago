use std::{
    collections::{btree_map::Entry, BTreeMap, BTreeSet},
    fs,
    io::ErrorKind,
    path::Path,
};

use serde::{Deserialize, Serialize};

const JOURNAL_VERSION: u32 = 2;
const LEGACY_JOURNAL_VERSION: u32 = 1;
const MAXIMUM_FILE_SIZE: u64 = 1024 * 1024;
const MAXIMUM_SESSIONS: usize = 1024;
const MAXIMUM_REVISIONS_PER_SESSION: usize = 65_536;
const MAXIMUM_SNAPSHOTS_PER_SESSION: usize = 65_536;
const MAXIMUM_SESSION_KEY_SIZE: usize = 512;
const MAXIMUM_PERSISTENT_ITEMS: usize = 256;
const MAXIMUM_ITEM_NAME_SIZE: usize = 128;

type Revisions = BTreeMap<u32, u64>;

#[derive(Debug, Clone, Default, PartialEq, Eq)]
struct SessionJournal {
    pending_revisions: Revisions,
    snapshots: BTreeMap<u32, SaveSnapshot>,
}

#[derive(Debug, Clone, Default)]
pub(crate) struct RevisionJournal {
    sessions: BTreeMap<String, SessionJournal>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) struct SaveRevision {
    pub checksum: u32,
    pub next_index: u64,
}

#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize, Deserialize)]
pub(crate) struct SaveSnapshot {
    pub next_index: u64,
    pub persistent_items: Vec<String>,
}

#[derive(Debug, Deserialize)]
struct DiskJournal {
    version: u32,
    sessions: BTreeMap<String, DiskSession>,
}

#[derive(Debug, Deserialize)]
#[serde(untagged)]
enum DiskSession {
    Legacy(BTreeMap<String, u64>),
    Current {
        pending_revisions: BTreeMap<String, u64>,
        snapshots: BTreeMap<String, SaveSnapshot>,
    },
}

#[derive(Serialize)]
struct SerializableJournal<'a> {
    sessions: BTreeMap<&'a str, SerializableSession<'a>>,
    version: u32,
}

#[derive(Serialize)]
struct SerializableSession<'a> {
    pending_revisions: BTreeMap<String, u64>,
    snapshots: BTreeMap<String, &'a SaveSnapshot>,
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
    #[error("revision journal version does not match its schema")]
    InvalidSchema,
    #[error("revision journal contains too many sessions")]
    TooManySessions,
    #[error("revision journal session key is empty or too long")]
    InvalidSessionKey,
    #[error("revision journal session contains too many revisions")]
    TooManyRevisions,
    #[error("revision journal session contains too many snapshots")]
    TooManySnapshots,
    #[error("snapshot contains too many persistent items")]
    TooManyPersistentItems,
    #[error("snapshot contains an empty or oversized persistent item name")]
    InvalidPersistentItem,
    #[error("snapshot contains duplicate persistent item `{0}`")]
    DuplicatePersistentItem(String),
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

fn parse_revisions(disk: BTreeMap<String, u64>) -> Result<Revisions, JournalError> {
    if disk.len() > MAXIMUM_REVISIONS_PER_SESSION {
        return Err(JournalError::TooManyRevisions);
    }

    let mut revisions = BTreeMap::new();
    for (checksum_text, next_index) in disk {
        let checksum = parse_checksum(&checksum_text)?;
        match revisions.entry(checksum) {
            Entry::Vacant(entry) => {
                entry.insert(next_index);
            }
            Entry::Occupied(_) => return Err(JournalError::DuplicateChecksum(checksum)),
        }
    }
    Ok(revisions)
}

fn validate_snapshot(snapshot: &SaveSnapshot) -> Result<(), JournalError> {
    if snapshot.persistent_items.len() > MAXIMUM_PERSISTENT_ITEMS {
        return Err(JournalError::TooManyPersistentItems);
    }

    let mut seen = BTreeSet::new();
    for item in &snapshot.persistent_items {
        if item.is_empty() || item.len() > MAXIMUM_ITEM_NAME_SIZE {
            return Err(JournalError::InvalidPersistentItem);
        }
        if !seen.insert(item) {
            return Err(JournalError::DuplicatePersistentItem(item.clone()));
        }
    }
    Ok(())
}

fn parse_snapshots(
    disk: BTreeMap<String, SaveSnapshot>,
) -> Result<BTreeMap<u32, SaveSnapshot>, JournalError> {
    if disk.len() > MAXIMUM_SNAPSHOTS_PER_SESSION {
        return Err(JournalError::TooManySnapshots);
    }

    let mut snapshots = BTreeMap::new();
    for (checksum_text, snapshot) in disk {
        let checksum = parse_checksum(&checksum_text)?;
        validate_snapshot(&snapshot)?;
        match snapshots.entry(checksum) {
            Entry::Vacant(entry) => {
                entry.insert(snapshot);
            }
            Entry::Occupied(_) => return Err(JournalError::DuplicateChecksum(checksum)),
        }
    }
    Ok(snapshots)
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
        if !matches!(disk.version, LEGACY_JOURNAL_VERSION | JOURNAL_VERSION) {
            return Err(JournalError::UnsupportedVersion(disk.version));
        }
        if disk.sessions.len() > MAXIMUM_SESSIONS {
            return Err(JournalError::TooManySessions);
        }

        let mut sessions = BTreeMap::new();
        for (key, disk_session) in disk.sessions {
            if key.is_empty() || key.len() > MAXIMUM_SESSION_KEY_SIZE {
                return Err(JournalError::InvalidSessionKey);
            }

            let session = match (disk.version, disk_session) {
                (LEGACY_JOURNAL_VERSION, DiskSession::Legacy(revisions)) => SessionJournal {
                    pending_revisions: parse_revisions(revisions)?,
                    snapshots: BTreeMap::new(),
                },
                (
                    JOURNAL_VERSION,
                    DiskSession::Current {
                        pending_revisions,
                        snapshots,
                    },
                ) => SessionJournal {
                    pending_revisions: parse_revisions(pending_revisions)?,
                    snapshots: parse_snapshots(snapshots)?,
                },
                _ => return Err(JournalError::InvalidSchema),
            };
            sessions.insert(key, session);
        }

        self.sessions = sessions;
        Ok(())
    }

    pub(crate) fn serialize(&self) -> Result<String, JournalError> {
        let sessions = self
            .sessions
            .iter()
            .map(|(key, session)| {
                let pending_revisions = session
                    .pending_revisions
                    .iter()
                    .map(|(checksum, next_index)| (format!("{checksum:08X}"), *next_index))
                    .collect();
                let snapshots = session
                    .snapshots
                    .iter()
                    .map(|(checksum, snapshot)| (format!("{checksum:08X}"), snapshot))
                    .collect();
                (
                    key.as_str(),
                    SerializableSession {
                        pending_revisions,
                        snapshots,
                    },
                )
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
        persistent_items: Vec<String>,
    ) {
        let session = self
            .sessions
            .entry(session_key(seed_name, team, slot))
            .or_default();
        session.pending_revisions.insert(checksum, next_index);
        session.snapshots.insert(
            checksum,
            SaveSnapshot {
                next_index,
                persistent_items,
            },
        );
    }

    pub(crate) fn store_snapshot(
        &mut self,
        seed_name: &str,
        team: u32,
        slot: u32,
        checksum: u32,
        snapshot: SaveSnapshot,
    ) -> Result<(), JournalError> {
        validate_snapshot(&snapshot)?;
        self.sessions
            .entry(session_key(seed_name, team, slot))
            .or_default()
            .snapshots
            .insert(checksum, snapshot);
        Ok(())
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
        if session.get().pending_revisions.get(&checksum) != Some(&next_index) {
            return false;
        }
        session.get_mut().pending_revisions.remove(&checksum);
        if session.get().pending_revisions.is_empty() && session.get().snapshots.is_empty() {
            session.remove();
        }
        true
    }

    pub(crate) fn pending(&self, seed_name: &str, team: u32, slot: u32) -> Vec<SaveRevision> {
        self.sessions
            .get(&session_key(seed_name, team, slot))
            .into_iter()
            .flat_map(|session| session.pending_revisions.iter())
            .map(|(&checksum, &next_index)| SaveRevision {
                checksum,
                next_index,
            })
            .collect()
    }

    pub(crate) fn snapshot(
        &self,
        seed_name: &str,
        team: u32,
        slot: u32,
        checksum: u32,
    ) -> Option<SaveSnapshot> {
        self.sessions
            .get(&session_key(seed_name, team, slot))?
            .snapshots
            .get(&checksum)
            .cloned()
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
    fn journal_should_preserve_snapshot_after_revision_acknowledgement() {
        let mut journal = RevisionJournal::default();
        journal.record(
            "seed",
            0,
            1,
            0x1234_5678,
            17,
            vec!["Tag pass".to_owned(), "Bike pass".to_owned()],
        );
        let path = temporary_path("round_trip");
        fs::write(&path, journal.serialize().unwrap()).unwrap();

        let mut restored = RevisionJournal::default();
        restored.load(&path).unwrap();
        fs::remove_file(path).unwrap();

        assert!(restored.acknowledge("seed", 0, 1, 0x1234_5678, 17));
        assert!(restored.pending("seed", 0, 1).is_empty());
        assert_eq!(
            restored.snapshot("seed", 0, 1, 0x1234_5678),
            Some(SaveSnapshot {
                next_index: 17,
                persistent_items: vec!["Tag pass".to_owned(), "Bike pass".to_owned()],
            })
        );
    }

    #[test]
    fn version_one_journal_should_migrate_with_empty_snapshots() {
        let path = temporary_path("migration");
        fs::write(
            &path,
            r#"{"version":1,"sessions":{"seed|0|1":{"12345678":17}}}"#,
        )
        .unwrap();

        let mut journal = RevisionJournal::default();
        journal.load(&path).unwrap();
        fs::remove_file(path).unwrap();

        assert_eq!(journal.pending("seed", 0, 1)[0].next_index, 17);
        assert_eq!(journal.snapshot("seed", 0, 1, 0x1234_5678), None);
        assert!(journal.serialize().unwrap().contains(r#""version": 2"#));
    }

    #[test]
    fn failed_load_should_leave_existing_state_unchanged() {
        let path = temporary_path("transactional");
        fs::write(
            &path,
            r#"{"version":2,"sessions":{"seed|0|1":{"pending_revisions":{"not-hex":2},"snapshots":{}}}}"#,
        )
        .unwrap();
        let mut journal = RevisionJournal::default();
        journal.record("seed", 0, 1, 1, 2, Vec::new());
        assert!(journal.load(&path).is_err());
        fs::remove_file(path).unwrap();
        assert_eq!(journal.pending("seed", 0, 1).len(), 1);
    }

    #[test]
    fn parser_should_reject_ambiguous_normalized_checksums() {
        let path = temporary_path("duplicate");
        fs::write(
            &path,
            r#"{"version":2,"sessions":{"seed|0|1":{"pending_revisions":{"A":1,"0000000a":2},"snapshots":{}}}}"#,
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
        journal.record("seed", 0, 1, 1, 2, Vec::new());
        journal.load(&path).unwrap();
        assert!(journal.pending("seed", 0, 1).is_empty());
    }
}
