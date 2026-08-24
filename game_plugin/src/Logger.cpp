#include "sr2ap/Logger.hpp"

#include "sr2ap/sr2ap_rust.h"

#include <cstdint>
#include <string_view>

namespace sr2ap::log {
    namespace {
        [[nodiscard]] constexpr Sr2apLogLevel ToRustLevel(Level level) noexcept {
            switch (level) {
                case Level::Trace:
                    return SR2AP_LOG_LEVEL_TRACE;
                case Level::Debug:
                    return SR2AP_LOG_LEVEL_DEBUG;
                case Level::Info:
                    return SR2AP_LOG_LEVEL_INFO;
                case Level::Warning:
                    return SR2AP_LOG_LEVEL_WARNING;
                case Level::Error:
                    return SR2AP_LOG_LEVEL_ERROR;
                case Level::Critical:
                    return SR2AP_LOG_LEVEL_CRITICAL;
            }

            return SR2AP_LOG_LEVEL_ERROR;
        }

        [[nodiscard]] const std::uint8_t* Bytes(std::string_view value) noexcept {
            return reinterpret_cast<const std::uint8_t*>(value.data());
        }
    }  // namespace

    bool Initialize(const std::filesystem::path& directory, bool debug) {
        const auto& native = directory.native();
        return sr2ap_log_initialize(reinterpret_cast<const std::uint16_t*>(native.data()), native.size(),
                                    debug ? std::uint8_t{1} : std::uint8_t{0}) == SR2AP_RESULT_OK;
    }

    void Write(Level level, std::string_view subsystem, std::string_view message) {
        static_cast<void>(sr2ap_log_write(ToRustLevel(level), Bytes(subsystem), subsystem.size(), Bytes(message),
                                          message.size()));
    }

    void Flush() {
        static_cast<void>(sr2ap_log_flush());
    }

    void Shutdown() {
        sr2ap_log_shutdown();
    }
}  // namespace sr2ap::log
