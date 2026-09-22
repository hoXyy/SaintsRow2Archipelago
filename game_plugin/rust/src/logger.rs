use std::{fmt, io::Write, path::Path, sync::OnceLock};

use flexi_logger::{DeferredNow, FileSpec, Logger, LoggerHandle};
use log::{Level, Record};

#[derive(Debug, thiserror::Error)]
pub(crate) enum LoggerError {
    #[error("logger is not initialized")]
    NotInitialized,
    #[error("could not install the global Rust logger")]
    Install,
    #[error(transparent)]
    Backend(#[from] flexi_logger::FlexiLoggerError),
    #[error("Invalid argument")]
    InvalidArgument,
}

static LOGGER: OnceLock<LoggerHandle> = OnceLock::new();

pub(crate) fn initialize(directory: &Path, debug_enabled: bool) -> Result<(), LoggerError> {
    let specification = if debug_enabled { "trace" } else { "info" };

    let handle = Logger::try_with_str(specification)?
        .log_to_file(FileSpec::try_from(directory.join("SR2Archipelago.log"))?)
        .format_for_files(format_record)
        .start()?;

    LOGGER.set(handle).map_err(|_| LoggerError::Install)
}

pub(crate) fn write(
    level: Level,
    subsystem: &str,
    message: impl fmt::Display,
) -> Result<(), LoggerError> {
    LOGGER.get().ok_or(LoggerError::NotInitialized)?;
    log::log!(target: subsystem, level, "{message}");
    Ok(())
}

pub(crate) fn flush() -> Result<(), LoggerError> {
    LOGGER.get().ok_or(LoggerError::NotInitialized)?.flush();
    Ok(())
}

pub(crate) fn shutdown() {
    if let Some(logger) = LOGGER.get() {
        logger.shutdown();
    }
}

fn level_name(level: Level) -> &'static str {
    match level {
        Level::Trace => "trace",
        Level::Debug => "debug",
        Level::Info => "info",
        Level::Warn => "warning",
        Level::Error => "error",
    }
}

fn format_record(
    writer: &mut dyn Write,
    now: &mut DeferredNow,
    record: &Record<'_>,
) -> std::io::Result<()> {
    writeln!(
        writer,
        "[{}] [{}] [{}] {}",
        now.format("%H:%M:%S%.3f"),
        level_name(record.level()),
        record.target(),
        record.args()
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn format_record_should_include_level_target_and_message() {
        let mut output = Vec::new();
        let mut now = DeferredNow::new();
        let record = Record::builder()
            .level(Level::Warn)
            .target("Test")
            .args(format_args!("record"))
            .build();

        format_record(&mut output, &mut now, &record).expect("format record");
        let output = String::from_utf8(output).expect("valid UTF-8 log record");

        assert!(output.contains("[warning] [Test] record"));
    }
}
