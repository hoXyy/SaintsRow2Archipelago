#pragma once

#include <filesystem>
#include <string_view>

namespace sr2ap::log {
    enum class Level {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    bool Initialize(const std::filesystem::path& preferredDirectory, bool debugEnabled);
    void Shutdown();
    void Write(Level level, std::string_view subsystem, std::string_view message);
    void Flush();
}  // namespace sr2ap::log

namespace sr2ap {
    inline void LogTrace(std::string_view subsystem, std::string_view message) {
        log::Write(log::Level::Trace, subsystem, message);
    }

    inline void LogDebug(std::string_view subsystem, std::string_view message) {
        log::Write(log::Level::Debug, subsystem, message);
    }

    inline void LogInfo(std::string_view subsystem, std::string_view message) {
        log::Write(log::Level::Info, subsystem, message);
    }

    inline void LogWarning(std::string_view subsystem, std::string_view message) {
        log::Write(log::Level::Warning, subsystem, message);
    }

    inline void LogError(std::string_view subsystem, std::string_view message) {
        log::Write(log::Level::Error, subsystem, message);
    }

    inline void LogCritical(std::string_view subsystem, std::string_view message) {
        log::Write(log::Level::Critical, subsystem, message);
    }
}  // namespace sr2ap
