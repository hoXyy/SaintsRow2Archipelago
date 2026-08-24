use std::{
    ffi::OsString,
    os::windows::ffi::OsStringExt,
    panic::{catch_unwind, AssertUnwindSafe},
    path::PathBuf,
};

use crate::logger::{self, LogLevel, LoggerError};

pub type Sr2apLogLevel = u32;

pub const SR2AP_LOG_LEVEL_TRACE: Sr2apLogLevel = 0;
pub const SR2AP_LOG_LEVEL_DEBUG: Sr2apLogLevel = 1;
pub const SR2AP_LOG_LEVEL_INFO: Sr2apLogLevel = 2;
pub const SR2AP_LOG_LEVEL_WARNING: Sr2apLogLevel = 3;
pub const SR2AP_LOG_LEVEL_ERROR: Sr2apLogLevel = 4;
pub const SR2AP_LOG_LEVEL_CRITICAL: Sr2apLogLevel = 5;

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Sr2apResult {
    Ok = 0,
    InvalidArgument = 1,
    NotInitialized = 2,
    IoError = 3,
    InternalError = 4,
}

impl From<LoggerError> for Sr2apResult {
    fn from(error: LoggerError) -> Self {
        match error {
            LoggerError::NotInitialized => Self::NotInitialized,
            LoggerError::Install => Self::InternalError,
            LoggerError::Io(_) => Self::IoError,
        }
    }
}

/// Initializes the logger.
///
/// `directory` is borrowed UTF-16 data and is only read for this call.
///
#[no_mangle]
pub unsafe extern "C" fn sr2ap_log_initialize(
    directory: *const u16,
    directory_len: usize,
    debug_enabled: u8,
) -> Sr2apResult {
    ffi_boundary(|| {
        if directory_len == 0 {
            return Err(Sr2apResult::InvalidArgument);
        }
        if directory.is_null() {
            return Err(Sr2apResult::InvalidArgument);
        }

        let directory = unsafe { std::slice::from_raw_parts(directory, directory_len) };

        let directory = PathBuf::from(OsString::from_wide(directory));
        logger::initialize(&directory, debug_enabled != 0).map_err(Into::into)
    })
}

/// Writes one record to the logger.
#[no_mangle]
pub unsafe extern "C" fn sr2ap_log_write(
    level: Sr2apLogLevel,
    subsystem: *const u8,
    subsystem_len: usize,
    message: *const u8,
    message_len: usize,
) -> Sr2apResult {
    ffi_boundary(|| {
        let level = parse_level(level).ok_or(Sr2apResult::InvalidArgument)?;
        if (subsystem.is_null() && subsystem_len != 0) || (message.is_null() && message_len != 0) {
            return Err(Sr2apResult::InvalidArgument);
        }

        let subsystem = if subsystem_len == 0 {
            &[]
        } else {
            unsafe { std::slice::from_raw_parts(subsystem, subsystem_len) }
        };
        let message = if message_len == 0 {
            &[]
        } else {
            unsafe { std::slice::from_raw_parts(message, message_len) }
        };
        let subsystem = String::from_utf8_lossy(subsystem);
        let message = String::from_utf8_lossy(message);

        logger::write(level, &subsystem, &message).map_err(Into::into)
    })
}

#[no_mangle]
pub extern "C" fn sr2ap_log_flush() -> Sr2apResult {
    ffi_boundary(|| logger::flush().map_err(Into::into))
}

#[no_mangle]
pub extern "C" fn sr2ap_log_shutdown() {
    let _ = catch_unwind(AssertUnwindSafe(logger::shutdown));
}

fn ffi_boundary(operation: impl FnOnce() -> Result<(), Sr2apResult>) -> Sr2apResult {
    match catch_unwind(AssertUnwindSafe(operation)) {
        Ok(Ok(())) => Sr2apResult::Ok,
        Ok(Err(error)) => error,
        Err(_) => Sr2apResult::InternalError,
    }
}

fn parse_level(level: u32) -> Option<LogLevel> {
    match level {
        SR2AP_LOG_LEVEL_TRACE => Some(LogLevel::Trace),
        SR2AP_LOG_LEVEL_DEBUG => Some(LogLevel::Debug),
        SR2AP_LOG_LEVEL_INFO => Some(LogLevel::Info),
        SR2AP_LOG_LEVEL_WARNING => Some(LogLevel::Warning),
        SR2AP_LOG_LEVEL_ERROR => Some(LogLevel::Error),
        SR2AP_LOG_LEVEL_CRITICAL => Some(LogLevel::Critical),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use std::os::windows::ffi::OsStrExt;

    use super::*;

    #[test]
    fn rejects_unknown_log_levels() {
        assert_eq!(
            unsafe { sr2ap_log_write(u32::MAX, [].as_ptr(), 0, [].as_ptr(), 0) },
            Sr2apResult::InvalidArgument
        );
    }

    #[test]
    fn initialize_rejects_an_empty_directory() {
        assert_eq!(
            unsafe { sr2ap_log_initialize(std::ptr::null(), 0, 0) },
            Sr2apResult::InvalidArgument
        );
    }

    #[test]
    fn write_rejects_a_null_pointer_with_nonzero_length() {
        assert_eq!(
            unsafe {
                sr2ap_log_write(
                    SR2AP_LOG_LEVEL_INFO,
                    std::ptr::null(),
                    1,
                    std::ptr::null(),
                    0,
                )
            },
            Sr2apResult::InvalidArgument
        );
    }

    #[test]
    fn flush_reports_when_the_logger_is_not_initialized() {
        let _test_guard = logger::TEST_LOCK.lock().expect("lock logger test");
        logger::shutdown();

        assert_eq!(sr2ap_log_flush(), Sr2apResult::NotInitialized);
    }

    #[test]
    fn ffi_boundary_converts_panics_to_internal_errors() {
        assert_eq!(
            ffi_boundary(|| -> Result<(), Sr2apResult> { panic!("test panic") }),
            Sr2apResult::InternalError
        );
    }

    #[test]
    fn parse_level_maps_every_c_log_level() {
        let actual = [
            SR2AP_LOG_LEVEL_TRACE,
            SR2AP_LOG_LEVEL_DEBUG,
            SR2AP_LOG_LEVEL_INFO,
            SR2AP_LOG_LEVEL_WARNING,
            SR2AP_LOG_LEVEL_ERROR,
            SR2AP_LOG_LEVEL_CRITICAL,
        ]
        .map(parse_level);
        let expected = [
            Some(LogLevel::Trace),
            Some(LogLevel::Debug),
            Some(LogLevel::Info),
            Some(LogLevel::Warning),
            Some(LogLevel::Error),
            Some(LogLevel::Critical),
        ];

        assert_eq!(actual, expected);
    }

    #[test]
    fn initialize_reports_file_open_failures() {
        let _test_guard = logger::TEST_LOCK.lock().expect("lock logger test");
        logger::shutdown();
        let directory = std::env::temp_dir().join(format!(
            "sr2ap-missing-parent-{}-{}",
            std::process::id(),
            "child"
        ));
        let wide: Vec<_> = directory.as_os_str().encode_wide().collect();

        assert_eq!(
            unsafe { sr2ap_log_initialize(wide.as_ptr(), wide.len(), 0) },
            Sr2apResult::IoError
        );
    }

    #[test]
    fn write_replaces_invalid_utf8() {
        let _test_guard = logger::TEST_LOCK.lock().expect("lock logger test");
        let directory =
            std::env::temp_dir().join(format!("sr2ap-ffi-utf8-test-{}", std::process::id()));
        std::fs::create_dir_all(&directory).expect("create test log directory");
        let wide: Vec<_> = directory.as_os_str().encode_wide().collect();
        assert_eq!(
            unsafe { sr2ap_log_initialize(wide.as_ptr(), wide.len(), 0) },
            Sr2apResult::Ok
        );

        let invalid = [0xff];
        assert_eq!(
            unsafe {
                sr2ap_log_write(
                    SR2AP_LOG_LEVEL_INFO,
                    b"Utf8".as_ptr(),
                    b"Utf8".len(),
                    invalid.as_ptr(),
                    invalid.len(),
                )
            },
            Sr2apResult::Ok
        );
        sr2ap_log_shutdown();

        let path = directory.join("SR2Archipelago.log");
        let contents = std::fs::read_to_string(&path).expect("read test log");
        std::fs::remove_file(path).expect("remove test log");
        std::fs::remove_dir(directory).expect("remove test log directory");

        assert!(contents.contains("[Utf8] �"));
    }
}
