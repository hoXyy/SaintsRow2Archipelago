use crate::protocol::IncomingMessage;
use crate::revision_journal::{RevisionJournal, SaveRevision, SaveSnapshot};
use std::{
    fs::{self, File},
    io::{self, Write},
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

fn replace_atomically(path: &Path, contents: &[u8]) -> io::Result<()> {
    let mut temporary = path.as_os_str().to_os_string();
    temporary.push(".tmp");
    let temporary = PathBuf::from(temporary);
    let result = (|| {
        let mut file = File::create(&temporary)?;
        file.write_all(contents)?;
        file.sync_all()?;
        drop(file);
        replace(&temporary, path)
    })();
    if result.is_err() {
        let _ = fs::remove_file(&temporary);
    }
    result
}

#[cfg(windows)]
fn replace(source: &Path, destination: &Path) -> io::Result<()> {
    use std::os::windows::ffi::OsStrExt;
    #[link(name = "kernel32")]
    extern "system" {
        fn MoveFileExW(existing: *const u16, replacement: *const u16, flags: u32) -> i32;
    }
    let source: Vec<u16> = source.as_os_str().encode_wide().chain(Some(0)).collect();
    let destination: Vec<u16> = destination
        .as_os_str()
        .encode_wide()
        .chain(Some(0))
        .collect();
    // SAFETY: Both paths are live, NUL-terminated UTF-16 buffers for this synchronous call.
    // REPLACE_EXISTING | WRITE_THROUGH preserves the previous C++ replacement behavior.
    if unsafe { MoveFileExW(source.as_ptr(), destination.as_ptr(), 0x1 | 0x8) } == 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

#[cfg(not(windows))]
fn replace(source: &Path, destination: &Path) -> io::Result<()> {
    fs::rename(source, destination)
}
