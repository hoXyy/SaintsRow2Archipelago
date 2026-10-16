use std::{
    fs::{self, File},
    io::{self, Write},
    path::{Path, PathBuf},
};

pub(crate) fn replace_atomically(path: &Path, contents: &[u8]) -> io::Result<()> {
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
    if unsafe { MoveFileExW(source.as_ptr(), destination.as_ptr(), 0x1 | 0x8) } == 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

#[cfg(not(windows))]
fn replace(source: &Path, destination: &Path) -> io::Result<()> {
    fs::rename(source, destination)
}

#[cfg(test)]
mod tests {
    use super::replace_atomically;
    use std::fs;

    fn temporary_path(name: &str) -> std::path::PathBuf {
        std::env::temp_dir().join(format!("sr2ap-fs-{}-{name}", std::process::id()))
    }

    #[test]
    fn replace_atomically_should_create_and_overwrite_the_file() {
        let path = temporary_path("replace.txt");
        replace_atomically(&path, b"first").unwrap();
        replace_atomically(&path, b"second").unwrap();
        assert_eq!(fs::read(&path).unwrap(), b"second");
        assert!(!path.with_extension("txt.tmp").exists());
        let _ = fs::remove_file(&path);
    }

    #[test]
    fn replace_atomically_should_fail_when_the_directory_is_missing() {
        let path = temporary_path("missing").join("replace.txt");
        assert!(replace_atomically(&path, b"contents").is_err());
    }
}
