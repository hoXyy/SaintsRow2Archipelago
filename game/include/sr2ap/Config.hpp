#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace sr2ap {

    struct Config {
        bool enabled{true};
        bool debugLogging{false};
        std::uint32_t pollingIntervalMs{1000};
        bool networkEnabled{true};
        std::uint16_t networkPort{38282};
        bool blockVanillaUnlockables{true};
        bool logFullSnapshots{false};
        bool logStateChanges{false};
        bool writeStatusFile{false};
        bool enableHotkeys{false};
        std::uint32_t moduleReportHotkey{0x76};
        std::uint32_t snapshotHotkey{0x77};
        std::uint32_t addressDumpHotkey{0x78};
    };

    struct ConfigLoadResult {
        Config config;
        bool fileFound{false};
        std::size_t warnings{0};
    };

    ConfigLoadResult ParseConfig(std::string_view text);
    ConfigLoadResult LoadConfig(const std::filesystem::path& path);

}  // namespace sr2ap
