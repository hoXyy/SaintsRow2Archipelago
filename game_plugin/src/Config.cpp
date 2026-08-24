#include "sr2ap/Config.hpp"
#include <ini.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <string>

namespace sr2ap {
    namespace {
        struct ParseContext {
            Config config;
            std::size_t warnings{};
        };

        bool EqualsIgnoreCase(const std::string_view left, const std::string_view right) {
            return left.size() == right.size() &&
                   std::equal(left.begin(), left.end(), right.begin(), [](const char a, const char b) {
                       return std::tolower(static_cast<unsigned char>(a)) ==
                              std::tolower(static_cast<unsigned char>(b));
                   });
        }

        std::optional<std::uint32_t> ParseUnsigned(std::string_view value) {
            int base = 10;
            if (value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
                value.remove_prefix(2);
                base = 16;
            }
            if (value.empty()) {
                return std::nullopt;
            }

            std::uint32_t output{};
            const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), output, base);
            if (ec != std::errc{} || ptr != value.data() + value.size()) {
                return std::nullopt;
            }
            return output;
        }

        std::optional<bool> ParseBool(const std::string_view value) {
            if (value == "1" || EqualsIgnoreCase(value, "true") || EqualsIgnoreCase(value, "yes") ||
                EqualsIgnoreCase(value, "on")) {
                return true;
            }
            if (value == "0" || EqualsIgnoreCase(value, "false") || EqualsIgnoreCase(value, "no") ||
                EqualsIgnoreCase(value, "off")) {
                return false;
            }
            return std::nullopt;
        }

        void ParseBoolean(ParseContext& context, const std::string_view value, bool& destination) {
            if (const auto parsed = ParseBool(value)) {
                destination = *parsed;
            } else {
                ++context.warnings;
            }
        }

        void ParseNumber(ParseContext& context, const std::string_view value, std::uint32_t& destination) {
            if (const auto parsed = ParseUnsigned(value)) {
                destination = *parsed;
            } else {
                ++context.warnings;
            }
        }

        void ParseGeneral(ParseContext& context, const std::string_view key, const std::string_view value) {
            if (EqualsIgnoreCase(key, "Enabled")) {
                ParseBoolean(context, value, context.config.enabled);
            } else {
                ++context.warnings;
            }
        }

        void ParseProgression(ParseContext& context, const std::string_view key, const std::string_view value) {
            if (EqualsIgnoreCase(key, "PollingIntervalMs")) {
                ParseNumber(context, value, context.config.pollingIntervalMs);
            } else if (EqualsIgnoreCase(key, "LogFullSnapshots")) {
                ParseBoolean(context, value, context.config.logFullSnapshots);
            } else if (EqualsIgnoreCase(key, "LogStateChanges")) {
                ParseBoolean(context, value, context.config.logStateChanges);
            } else {
                ++context.warnings;
            }
        }

        void ParseDebug(ParseContext& context, const std::string_view key, const std::string_view value) {
            if (EqualsIgnoreCase(key, "DebugLogging")) {
                ParseBoolean(context, value, context.config.debugLogging);
            } else if (EqualsIgnoreCase(key, "WriteStatusFile")) {
                ParseBoolean(context, value, context.config.writeStatusFile);
            } else if (EqualsIgnoreCase(key, "EnableHotkeys")) {
                ParseBoolean(context, value, context.config.enableHotkeys);
            } else if (EqualsIgnoreCase(key, "ModuleReportHotkey")) {
                ParseNumber(context, value, context.config.moduleReportHotkey);
            } else if (EqualsIgnoreCase(key, "SnapshotHotkey")) {
                ParseNumber(context, value, context.config.snapshotHotkey);
            } else if (EqualsIgnoreCase(key, "AddressDumpHotkey")) {
                ParseNumber(context, value, context.config.addressDumpHotkey);
            } else {
                ++context.warnings;
            }
        }

        void ParseNetwork(ParseContext& context, const std::string_view key, const std::string_view value) {
            if (EqualsIgnoreCase(key, "Enabled")) {
                ParseBoolean(context, value, context.config.networkEnabled);
            } else if (EqualsIgnoreCase(key, "Port")) {
                const auto parsed = ParseUnsigned(value);
                if (parsed && *parsed <= std::numeric_limits<std::uint16_t>::max()) {
                    context.config.networkPort = static_cast<std::uint16_t>(*parsed);
                } else {
                    ++context.warnings;
                }
            } else {
                ++context.warnings;
            }
        }

        int HandleIniValue(void* user, const char* section, const char* name, const char* value) {
            auto& context = *static_cast<ParseContext*>(user);
            const std::string_view sectionName{section};
            const std::string_view key{name};
            const std::string_view text{value};

            if (EqualsIgnoreCase(sectionName, "General")) {
                ParseGeneral(context, key, text);
            } else if (EqualsIgnoreCase(sectionName, "Progression")) {
                ParseProgression(context, key, text);
            } else if (EqualsIgnoreCase(sectionName, "Debug")) {
                ParseDebug(context, key, text);
            } else if (EqualsIgnoreCase(sectionName, "Network")) {
                ParseNetwork(context, key, text);
            } else {
                ++context.warnings;
            }

            return 1;
        }

        void Validate(ParseContext& context) {
            if (context.config.pollingIntervalMs < 100) {
                context.config.pollingIntervalMs = 100;
                ++context.warnings;
            }
        }
    }  // namespace

    ConfigLoadResult ParseConfig(const std::string_view text) {
        ParseContext context;
        const int parseResult = ini_parse_string_length(text.data(), text.size(), HandleIniValue, &context);
        if (parseResult != 0) {
            ++context.warnings;
        }

        Validate(context);
        return {context.config, false, context.warnings};
    }

    ConfigLoadResult LoadConfig(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return {};
        }

        const std::string content{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
        auto result = ParseConfig(content);
        result.fileFound = true;
        return result;
    }
}  // namespace sr2ap
