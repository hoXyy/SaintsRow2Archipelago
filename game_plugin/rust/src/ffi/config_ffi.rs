use std::{ffi::OsString, os::windows::ffi::OsStringExt, path::PathBuf};

use crate::config::{self, ConfigError};

#[cxx::bridge(namespace = "sr2ap::rust")]
pub(crate) mod bridge {
    struct Config {
        enabled: bool,
        debug_logging: bool,
        polling_interval_ms: u32,
        network_enabled: bool,
        network_port: u16,
        log_full_snapshots: bool,
        log_state_changes: bool,
        write_status_file: bool,
        enable_hotkeys: bool,
        module_report_hotkey: u32,
        snapshot_hotkey: u32,
        address_dump_hotkey: u32,
    }

    struct ConfigLoadResult {
        config: Config,
        file_found: bool,
        warnings: u32,
    }

    extern "Rust" {
        fn config_load(path: &[u16]) -> Result<ConfigLoadResult>;
    }
}

impl From<config::Config> for bridge::Config {
    fn from(config: config::Config) -> Self {
        Self {
            enabled: config.enabled,
            debug_logging: config.debug_logging,
            polling_interval_ms: config.polling_interval_ms,
            network_enabled: config.network_enabled,
            network_port: config.network_port,
            log_full_snapshots: config.log_full_snapshots,
            log_state_changes: config.log_state_changes,
            write_status_file: config.write_status_file,
            enable_hotkeys: config.enable_hotkeys,
            module_report_hotkey: config.module_report_hotkey,
            snapshot_hotkey: config.snapshot_hotkey,
            address_dump_hotkey: config.address_dump_hotkey,
        }
    }
}

fn config_load(path: &[u16]) -> Result<bridge::ConfigLoadResult, ConfigError> {
    let path = PathBuf::from(OsString::from_wide(path));
    let result = config::load(&path)?;

    Ok(bridge::ConfigLoadResult {
        config: result.config.into(),
        file_found: result.file_found,
        warnings: result.warnings,
    })
}
