use std::{ffi::OsString, os::windows::ffi::OsStringExt, path::PathBuf};

use crate::logger::{self, LoggerError};

#[cxx::bridge(namespace = "sr2ap::rust")]
pub(crate) mod bridge {
    enum LogLevel {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical,
    }

    extern "Rust" {
        fn log_initialize(directory: &[u16], debug_enabled: bool) -> Result<()>;

        fn log_write(level: LogLevel, subsystem: &[u8], message: &[u8]) -> Result<()>;

        fn log_flush() -> Result<()>;

        fn log_shutdown();
    }
}

fn log_initialize(directory: &[u16], debug_enabled: bool) -> Result<(), LoggerError> {
    if directory.is_empty() {
        return Err(LoggerError::InvalidArgument);
    }

    let directory = PathBuf::from(OsString::from_wide(directory));
    logger::initialize(&directory, debug_enabled)
}

fn log_write(level: bridge::LogLevel, subsystem: &[u8], message: &[u8]) -> Result<(), LoggerError> {
    let level = match level {
        bridge::LogLevel::Trace => logger::LogLevel::Trace,
        bridge::LogLevel::Debug => logger::LogLevel::Debug,
        bridge::LogLevel::Info => logger::LogLevel::Info,
        bridge::LogLevel::Warning => logger::LogLevel::Warning,
        bridge::LogLevel::Error => logger::LogLevel::Error,
        bridge::LogLevel::Critical => logger::LogLevel::Critical,
        _ => return Err(LoggerError::InvalidArgument),
    };

    let subsystem = String::from_utf8_lossy(subsystem);
    let message = String::from_utf8_lossy(message);

    logger::write(level, &subsystem, &message)
}

fn log_flush() -> Result<(), LoggerError> {
    logger::flush()
}

fn log_shutdown() {
    logger::shutdown();
}
