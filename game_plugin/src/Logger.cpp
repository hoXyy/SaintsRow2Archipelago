#include "sr2ap/Logger.hpp"

#include "rust/cxx.h"
#include "sr2ap/src/ffi/logger_ffi.rs.h"

#include <cstdint>
#include <string_view>

namespace sr2ap::log {
    namespace {
        [[nodiscard]] constexpr rust::LogLevel ToRustLevel(
            const Level level) noexcept {
            switch (level) {
                case Level::Trace:
                    return rust::LogLevel::Trace;
                case Level::Debug:
                    return rust::LogLevel::Debug;
                case Level::Info:
                    return rust::LogLevel::Info;
                case Level::Warning:
                    return rust::LogLevel::Warning;
                case Level::Error:
                    return rust::LogLevel::Error;
                case Level::Critical:
                    return rust::LogLevel::Critical;
            }

            return rust::LogLevel::Error;
        }

        [[nodiscard]] ::rust::Slice<const std::uint8_t> Bytes(
            const std::string_view value) noexcept {
            return {
                reinterpret_cast<const std::uint8_t*>(value.data()),
                value.size(),
            };
        }
    }

    bool Initialize(
        const std::filesystem::path& directory,
        const bool debugEnabled) {
        const auto& native = directory.native();

        try {
            rust::log_initialize(
                {reinterpret_cast<const std::uint16_t*>(native.data()),
                 native.size()},
                debugEnabled);
            return true;
        } catch (const ::rust::Error&) {
            return false;
        }
    }

    void Write(
        const Level level,
        const std::string_view subsystem,
        const std::string_view message) {
        try {
            rust::log_write(
                ToRustLevel(level),
                Bytes(subsystem),
                Bytes(message));
        } catch (const ::rust::Error&) {
            // Logging cannot report a logging failure through the logger itself.
        }
    }

    void Flush() {
        try {
            rust::log_flush();
        } catch (const ::rust::Error&) {
        }
    }

    void Shutdown() {
        rust::log_shutdown();
    }
}
