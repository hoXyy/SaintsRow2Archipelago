#include "sr2ap/Logger.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <windows.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace sr2ap::log {
    namespace {
        std::shared_ptr<spdlog::logger> logger;

        [[nodiscard]] constexpr spdlog::level::level_enum ToSpdlogLevel(Level level) noexcept {
            switch (level) {
                case Level::Trace:
                    return spdlog::level::trace;
                case Level::Debug:
                    return spdlog::level::debug;
                case Level::Info:
                    return spdlog::level::info;
                case Level::Warning:
                    return spdlog::level::warn;
                case Level::Error:
                    return spdlog::level::err;
                case Level::Critical:
                    return spdlog::level::critical;
            }

            return spdlog::level::off;
        };
    }  // namespace

    bool Initialize(const std::filesystem::path& directory, bool debug) {
        auto file =
            std::make_shared<spdlog::sinks::basic_file_sink_mt>((directory / "SR2Archipelago.log").string(), true);

        logger = std::make_shared<spdlog::logger>("SR2AP", file);
        logger->set_pattern("[%H:%M:%S.%e] [T:%t] [%l] %v");
        logger->set_level(debug ? spdlog::level::trace : spdlog::level::info);
        logger->flush_on(spdlog::level::trace);

        return true;
    }

    void Write(Level level, std::string_view subsystem, std::string_view message) {
        logger->log(ToSpdlogLevel(level), "[{}] {}", subsystem, message);
    }

    void Flush() {
        if (logger) {
            logger->flush();
        }
    }

    void Shutdown() {
        logger.reset();
        spdlog::shutdown();
    }
}  // namespace sr2ap::log
