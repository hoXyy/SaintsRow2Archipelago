use std::{fs, path::Path};

use serde::Deserialize;

#[derive(Clone, Copy, Debug)]
pub(crate) struct Config {
    pub enabled: bool,
    pub debug_logging: bool,
    pub polling_interval_ms: u32,
    pub network_enabled: bool,
    pub network_port: u16,
    pub log_full_snapshots: bool,
    pub log_state_changes: bool,
    pub write_status_file: bool,
    pub enable_hotkeys: bool,
    pub module_report_hotkey: u32,
    pub snapshot_hotkey: u32,
    pub address_dump_hotkey: u32,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            enabled: true,
            debug_logging: false,
            polling_interval_ms: 1_000,
            network_enabled: true,
            network_port: 38_282,
            log_full_snapshots: false,
            log_state_changes: false,
            write_status_file: false,
            enable_hotkeys: false,
            module_report_hotkey: 0x76,
            snapshot_hotkey: 0x77,
            address_dump_hotkey: 0x78,
        }
    }
}

#[derive(Debug)]
pub(crate) struct ConfigLoadResult {
    pub config: Config,
    pub file_found: bool,
    pub warnings: u32,
}

impl Default for ConfigLoadResult {
    fn default() -> Self {
        Self {
            config: Config::default(),
            file_found: false,
            warnings: 0,
        }
    }
}

#[derive(Debug, thiserror::Error)]
pub(crate) enum ConfigError {
    #[error("could not read configuration: {0}")]
    Io(#[from] std::io::Error),

    #[error("invalid TOML configuration: {0}")]
    Parse(#[from] toml::de::Error),
}

#[derive(Debug, Default, Deserialize)]
#[serde(default, deny_unknown_fields)]
struct FileConfig {
    general: GeneralConfig,
    progression: ProgressionConfig,
    network: NetworkConfig,
    debug: DebugConfig,
}

#[derive(Debug, Deserialize)]
#[serde(default, deny_unknown_fields)]
struct GeneralConfig {
    enabled: bool,
}

impl Default for GeneralConfig {
    fn default() -> Self {
        Self { enabled: true }
    }
}

#[derive(Debug, Deserialize)]
#[serde(default, deny_unknown_fields)]
struct ProgressionConfig {
    polling_interval_ms: u32,
    log_full_snapshots: bool,
    log_state_changes: bool,
}

impl Default for ProgressionConfig {
    fn default() -> Self {
        Self {
            polling_interval_ms: 1_000,
            log_full_snapshots: false,
            log_state_changes: false,
        }
    }
}

#[derive(Debug, Deserialize)]
#[serde(default, deny_unknown_fields)]
struct NetworkConfig {
    enabled: bool,
    port: u16,
}

impl Default for NetworkConfig {
    fn default() -> Self {
        Self {
            enabled: true,
            port: 38_282,
        }
    }
}

#[derive(Debug, Default, Deserialize)]
#[serde(default, deny_unknown_fields)]
struct DebugConfig {
    debug_logging: bool,
    write_status_file: bool,
    hotkeys: HotkeyConfig,
}

#[derive(Debug, Deserialize)]
#[serde(default, deny_unknown_fields)]
struct HotkeyConfig {
    enable_hotkeys: bool,
    module_report_hotkey: u32,
    snapshot_hotkey: u32,
    address_dump_hotkey: u32,
}

impl Default for HotkeyConfig {
    fn default() -> Self {
        Self {
            enable_hotkeys: false,
            module_report_hotkey: 0x76,
            snapshot_hotkey: 0x77,
            address_dump_hotkey: 0x78,
        }
    }
}

pub(crate) fn parse(text: &str) -> Result<ConfigLoadResult, ConfigError> {
    let file: FileConfig = toml::from_str(text)?;
    let mut warnings = 0;

    let polling_interval_ms = if file.progression.polling_interval_ms < 100 {
        warnings += 1;
        100
    } else {
        file.progression.polling_interval_ms
    };

    Ok(ConfigLoadResult {
        config: Config {
            enabled: file.general.enabled,
            debug_logging: file.debug.debug_logging,
            polling_interval_ms,
            network_enabled: file.network.enabled,
            network_port: file.network.port,
            log_full_snapshots: file.progression.log_full_snapshots,
            log_state_changes: file.progression.log_state_changes,
            write_status_file: file.debug.write_status_file,
            enable_hotkeys: file.debug.hotkeys.enable_hotkeys,
            module_report_hotkey: file.debug.hotkeys.module_report_hotkey,
            snapshot_hotkey: file.debug.hotkeys.snapshot_hotkey,
            address_dump_hotkey: file.debug.hotkeys.address_dump_hotkey,
        },
        file_found: false,
        warnings,
    })
}

pub(crate) fn load(path: &Path) -> Result<ConfigLoadResult, ConfigError> {
    let text = match fs::read_to_string(path) {
        Ok(text) => text,
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => {
            return Ok(ConfigLoadResult::default());
        }
        Err(error) => return Err(error.into()),
    };

    let mut result = parse(&text)?;
    result.file_found = true;
    Ok(result)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_should_use_defaults_for_empty_document() {
        let result = parse("").expect("parse empty configuration");

        assert_eq!(result.config.polling_interval_ms, 1_000);
        assert_eq!(result.config.network_port, 38_282);
        assert!(result.config.enabled);
    }

    #[test]
    fn parse_should_read_full_configuration() {
        let result = parse(
            r#"
[general]
enabled = false

[progression]
polling_interval_ms = 250
log_full_snapshots = true
log_state_changes = true

[network]
enabled = false
port = 12345

[debug]
debug_logging = true
write_status_file = true

[debug.hotkeys]
enable_hotkeys = true
module_report_hotkey = 0x70
snapshot_hotkey = 0x71
address_dump_hotkey = 0x72
"#,
        )
        .expect("parse full configuration");

        assert!(!result.config.enabled);
        assert_eq!(result.config.polling_interval_ms, 250);
        assert_eq!(result.config.network_port, 12_345);
        assert_eq!(result.config.snapshot_hotkey, 0x71);
    }

    #[test]
    fn parse_should_reject_unknown_fields() {
        let error = parse(
            r#"
[network]
prot = 12345
"#,
        )
        .expect_err("reject misspelled field");

        assert!(matches!(error, ConfigError::Parse(_)));
    }

    #[test]
    fn parse_should_clamp_short_polling_interval() {
        let result = parse(
            r#"
[progression]
polling_interval_ms = 99
"#,
        )
        .expect("parse configuration");

        assert_eq!(result.config.polling_interval_ms, 100);
        assert_eq!(result.warnings, 1);
    }

    #[test]
    fn load_should_return_defaults_when_file_is_missing() {
        let path =
            std::env::temp_dir().join(format!("sr2ap-missing-config-{}.toml", std::process::id()));

        let result = load(&path).expect("load missing configuration");

        assert!(!result.file_found);
    }
}
