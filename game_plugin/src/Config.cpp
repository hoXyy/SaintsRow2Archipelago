#include "sr2ap/Config.hpp"

#include <cstdint>
#include <system_error>

#include "rust/cxx.h"
#include "sr2ap/src/ffi/config_ffi.rs.h"

namespace sr2ap {
namespace {
[[nodiscard]] Config ToCppConfig(const rust::Config& config) noexcept {
    return {
        config.enabled,
        config.debug_logging,
        config.polling_interval_ms,
        config.network_enabled,
        config.network_port,
        config.log_full_snapshots,
        config.log_state_changes,
        config.write_status_file,
        config.enable_hotkeys,
        config.module_report_hotkey,
        config.snapshot_hotkey,
        config.address_dump_hotkey,
    };
}
}  // namespace

ConfigLoadResult LoadConfig(const std::filesystem::path& path) {
    const auto& native = path.native();

    try {
        const auto result = rust::config_load(
            {reinterpret_cast<const std::uint16_t*>(native.data()),
             native.size()});

        return {
            ToCppConfig(result.config),
            result.file_found,
            result.warnings,
        };
    } catch (const ::rust::Error&) {
        std::error_code error;
        const bool fileFound = std::filesystem::exists(path, error);
        return {Config{}, fileFound, 1};
    }
}
}  // namespace sr2ap
