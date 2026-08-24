use std::{
    fmt,
    fs::{File, OpenOptions},
    io::{BufWriter, Write},
    path::Path,
    sync::{Mutex, OnceLock},
};

use chrono::Local;
use log::{Level, LevelFilter, Log, Metadata, Record};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub(crate) enum LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
}

#[derive(Debug, thiserror::Error)]
pub(crate) enum LoggerError {
    #[error("logger is not initialized")]
    NotInitialized,
    #[error("could not install the global Rust logger")]
    Install,
    #[error(transparent)]
    Io(#[from] std::io::Error),
}

impl LogLevel {
    fn filter(self) -> LevelFilter {
        match self {
            Self::Trace => LevelFilter::Trace,
            Self::Debug => LevelFilter::Debug,
            Self::Info => LevelFilter::Info,
            Self::Warning => LevelFilter::Warn,
            Self::Error | Self::Critical => LevelFilter::Error,
        }
    }
}

impl From<Level> for LogLevel {
    fn from(level: Level) -> Self {
        match level {
            Level::Error => Self::Error,
            Level::Warn => Self::Warning,
            Level::Info => Self::Info,
            Level::Debug => Self::Debug,
            Level::Trace => Self::Trace,
        }
    }
}

struct LoggerState {
    writer: BufWriter<File>,
    minimum_level: LevelFilter,
}

struct GlobalLogger;

static STATE: Mutex<Option<LoggerState>> = Mutex::new(None);
static LOGGER_INSTALLED: OnceLock<bool> = OnceLock::new();
static GLOBAL_LOGGER: GlobalLogger = GlobalLogger;
#[cfg(test)]
pub(crate) static TEST_LOCK: Mutex<()> = Mutex::new(());

impl Log for GlobalLogger {
    fn enabled(&self, metadata: &Metadata<'_>) -> bool {
        let state = lock_state();
        state
            .as_ref()
            .is_some_and(|state| metadata.level().to_level_filter() <= state.minimum_level)
    }

    fn log(&self, record: &Record<'_>) {
        if self.enabled(record.metadata()) {
            let _ = write(record.level().into(), record.target(), record.args());
        }
    }

    fn flush(&self) {
        let _ = flush();
    }
}

pub(crate) fn initialize(directory: &Path, debug_enabled: bool) -> Result<(), LoggerError> {
    let file = OpenOptions::new()
        .create(true)
        .truncate(true)
        .write(true)
        .open(directory.join("SR2Archipelago.log"))?;

    let installed = *LOGGER_INSTALLED.get_or_init(|| log::set_logger(&GLOBAL_LOGGER).is_ok());
    if !installed {
        return Err(LoggerError::Install);
    }

    log::set_max_level(LevelFilter::Trace);
    let minimum_level = if debug_enabled {
        LevelFilter::Trace
    } else {
        LevelFilter::Info
    };
    *lock_state() = Some(LoggerState {
        writer: BufWriter::new(file),
        minimum_level,
    });
    Ok(())
}

pub(crate) fn write(
    level: LogLevel,
    subsystem: &str,
    message: impl fmt::Display,
) -> Result<(), LoggerError> {
    let mut state = lock_state();
    let state = state.as_mut().ok_or(LoggerError::NotInitialized)?;
    if level.filter() > state.minimum_level {
        return Ok(());
    }

    let timestamp = Local::now().format("%H:%M:%S%.3f");
    let thread = std::thread::current().id();
    writeln!(
        state.writer,
        "[{timestamp}] [T:{thread:?}] [{}] [{subsystem}] {message}",
        level_name(level)
    )
    .and_then(|()| state.writer.flush())
    .map_err(LoggerError::Io)
}

pub(crate) fn flush() -> Result<(), LoggerError> {
    let mut state = lock_state();
    state
        .as_mut()
        .ok_or(LoggerError::NotInitialized)?
        .writer
        .flush()
        .map_err(LoggerError::Io)
}

pub(crate) fn shutdown() {
    let mut state = lock_state();
    if let Some(mut logger) = state.take() {
        let _ = logger.writer.flush();
    }
}

fn lock_state() -> std::sync::MutexGuard<'static, Option<LoggerState>> {
    STATE
        .lock()
        .unwrap_or_else(|poisoned| poisoned.into_inner())
}

fn level_name(level: LogLevel) -> &'static str {
    match level {
        LogLevel::Trace => "trace",
        LogLevel::Debug => "debug",
        LogLevel::Info => "info",
        LogLevel::Warning => "warning",
        LogLevel::Error => "error",
        LogLevel::Critical => "critical",
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn run_with_logger(test_name: &str, debug_enabled: bool, operation: impl FnOnce()) -> String {
        let _test_guard = TEST_LOCK.lock().expect("lock logger test");
        let directory = std::env::temp_dir().join(format!(
            "sr2ap-logger-test-{}-{test_name}",
            std::process::id()
        ));
        std::fs::create_dir_all(&directory).expect("create test log directory");

        initialize(&directory, debug_enabled).expect("initialize logger");
        operation();
        shutdown();

        let path = directory.join("SR2Archipelago.log");
        let contents = std::fs::read_to_string(&path).expect("read test log");
        std::fs::remove_file(path).expect("remove test log");
        std::fs::remove_dir(directory).expect("remove test log directory");
        contents
    }

    #[test]
    fn write_should_preserve_critical_level_and_cpp_subsystem() {
        let contents = run_with_logger("cpp-record", false, || {
            write(LogLevel::Critical, "CppTest", "critical record").expect("write C++ record");
        });

        assert!(contents.contains("[critical] [CppTest] critical record"));
    }

    #[test]
    fn log_facade_should_use_the_same_backend() {
        let contents = run_with_logger("rust-record", false, || {
            log::info!(target: "RustTest", "Rust record");
        });

        assert!(contents.contains("[info] [RustTest] Rust record"));
    }

    #[test]
    fn write_should_filter_debug_records_when_debug_is_disabled() {
        let contents = run_with_logger("debug-disabled", false, || {
            write(LogLevel::Debug, "Test", "filtered").expect("filter debug record");
        });

        assert!(!contents.contains("filtered"));
    }

    #[test]
    fn write_should_include_debug_records_when_debug_is_enabled() {
        let contents = run_with_logger("debug-enabled", true, || {
            write(LogLevel::Debug, "Test", "included").expect("write debug record");
        });

        assert!(contents.contains("[debug] [Test] included"));
    }

    #[test]
    fn write_should_serialize_concurrent_records() {
        let contents = run_with_logger("concurrent", false, || {
            let threads: Vec<_> = (0..4)
                .map(|index| {
                    std::thread::spawn(move || {
                        write(LogLevel::Info, "Thread", format_args!("record {index}"))
                            .expect("write concurrent record");
                    })
                })
                .collect();

            for thread in threads {
                thread.join().expect("join logging thread");
            }
        });

        assert_eq!(contents.lines().count(), 4);
    }

    #[test]
    fn initialize_should_allow_reinitialization_after_shutdown() {
        let _test_guard = TEST_LOCK.lock().expect("lock logger test");
        let directory = std::env::temp_dir().join(format!(
            "sr2ap-logger-test-{}-reinitialize",
            std::process::id()
        ));
        std::fs::create_dir_all(&directory).expect("create test log directory");

        initialize(&directory, false).expect("initialize logger first time");
        shutdown();
        initialize(&directory, false).expect("initialize logger second time");
        write(LogLevel::Info, "Test", "after restart").expect("write after restart");
        shutdown();

        let path = directory.join("SR2Archipelago.log");
        let contents = std::fs::read_to_string(&path).expect("read test log");
        std::fs::remove_file(path).expect("remove test log");
        std::fs::remove_dir(directory).expect("remove test log directory");

        assert!(contents.contains("[info] [Test] after restart"));
    }
}
