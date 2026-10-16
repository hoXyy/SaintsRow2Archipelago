use std::{ffi::OsString, os::windows::ffi::OsStringExt, path::PathBuf};

use crate::fs;

#[cxx::bridge(namespace = "sr2ap::rust")]
pub(crate) mod bridge {
    extern "Rust" {
        fn fs_replace_atomically(path: &[u16], contents: &[u8]) -> Result<()>;
    }
}

fn fs_replace_atomically(path: &[u16], contents: &[u8]) -> std::io::Result<()> {
    fs::replace_atomically(&PathBuf::from(OsString::from_wide(path)), contents)
}
