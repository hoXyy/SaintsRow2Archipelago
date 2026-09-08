#include "sr2ap/Plugin.hpp"

#include <windows.h>

#include <atomic>

#include "runtime/SessionRuntime.hpp"
#include "sr2ap/Config.hpp"
#include "sr2ap/GameState.hpp"
#include "sr2ap/Helpers.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/ModuleInfo.hpp"
#include "sr2ap/ProgressionMonitor.hpp"
#include "sr2ap/SaveRevisionMonitor.hpp"

#ifndef SR2AP_VERSION
#define SR2AP_VERSION "dev"
#endif

namespace sr2ap {
namespace {
std::atomic<bool> shutdownRequested{false};

}  // namespace

void RequestShutdown() {
    shutdownRequested.store(true, std::memory_order_release);
}

DWORD WINAPI PluginThread(void* parameter) {
    try {
        const auto plugin = static_cast<HMODULE>(parameter);
        const auto pluginInfo = InspectModule(plugin);
        const auto pluginDirectory = pluginInfo && !pluginInfo->path.empty()
                                         ? pluginInfo->path.parent_path()
                                         : std::filesystem::current_path();
        const auto configPath = pluginDirectory / L"SR2Archipelago.toml";
        const auto [config, fileFound, warnings] = LoadConfig(configPath);
        log::Initialize(pluginDirectory, config.debugLogging);
        LogInfo("Plugin",
                std::string("SR2Archipelago loaded version=") + SR2AP_VERSION);
        LogInfo("Plugin", "compiler=MSVC " + std::to_string(_MSC_VER));
#ifdef NDEBUG
        LogInfo("Plugin", "build_configuration=Release architecture=x86");
#else
        LogInfo("Plugin", "build_configuration=Debug architecture=x86");
#endif
        LogInfo("Config",
                "path=" + Narrow(configPath) +
                    (fileFound ? " loaded=1" : " loaded=0 defaults_used=1"));
        if (warnings) {
            LogWarning("Config", "Malformed values ignored/clamped: " +
                                     std::to_string(warnings));
        }

        ReportAllModules(plugin);
        const auto executable = InspectModule(GetModuleHandleW(nullptr));
        const bool gameSupported =
            executable && IsSupportedExecutable(*executable);
        if (!gameSupported) {
            LogWarning(
                "Plugin",
                "Unsupported executable; game-specific hooks are disabled.");
        } else {
            LogInfo("Plugin", "Supported executable found.");
        }

        SessionRuntime session{
            gameSupported, pluginDirectory / L"SR2ArchipelagoRevisions.json"};
        SaveRevisionMonitor saveRevisions;
        const bool saveRevisionInstalled =
            session.InstallSaveMonitoring(saveRevisions, config.enabled);
        if (saveRevisionInstalled) {
            LogInfo("SaveRevision", "Checksum load/write monitoring installed");
        } else if (config.enabled && gameSupported) {
            LogWarning(
                "SaveRevision",
                "Checksum monitoring unavailable; AP item delivery disabled");
        }

        if (config.enabled) {
            if (config.debugLogging) {
                LogDebug("Hitman",
                         "Polling enabled interval_ms=" +
                             std::to_string(config.pollingIntervalMs));
                LogDebug("ChopShop",
                         "Polling enabled interval_ms=" +
                             std::to_string(config.pollingIntervalMs));
                LogDebug("Missions",
                         "Polling enabled interval_ms=" +
                             std::to_string(config.pollingIntervalMs) +
                             " base_game_missions=56 pc_dlc_missions=excluded");
                LogDebug("Activities",
                         "Polling enabled interval_ms=" +
                             std::to_string(config.pollingIntervalMs) +
                             " expected_instances=24");
                LogDebug("CDs", "Polling enabled interval_ms=" +
                                    std::to_string(config.pollingIntervalMs) +
                                    " target=50");
            }
            if (config.networkEnabled) {
                session.Connect(config.networkPort);
                LogInfo("Network", "Connecting to AP client on 127.0.0.1:" +
                                       std::to_string(config.networkPort));
            }
        } else {
            LogInfo("Plugin",
                    "Plugin disabled by configuration; diagnostic hotkeys "
                    "remain available");
        }

        bool moduleDown = false, snapshotDown = false, dumpDown = false;
        ULONGLONG nextPoll = GetTickCount64();
        const auto statusPath = pluginDirectory / L"SR2ArchipelagoStatus.txt";
        ProgressionMonitor progression(
            statusPath,
            [&session](const ProgressionEvent& event) {
                session.SendProgression(event);
            },
            config.writeStatusFile);
        while (!shutdownRequested.load(std::memory_order_acquire)) {
            if (saveRevisionInstalled) {
                saveRevisions.Poll();
            }

            const auto readiness = GetGameReadiness();
            session.UpdateReadiness(readiness);

            if (config.enableHotkeys) {
                if (Pressed(config.moduleReportHotkey, moduleDown)) {
                    ReportAllModules(plugin);
                }
                if (Pressed(config.snapshotHotkey, snapshotDown)) {
                    progression.CaptureManualSnapshot(config.logFullSnapshots);
                }
                if (Pressed(config.addressDumpHotkey, dumpDown)) {
                    progression.DumpCompactSnapshot();
                }
            }
            const auto now = GetTickCount64();
            if (config.enabled &&
                (session.CommunicationsActive() || config.writeStatusFile) &&
                now >= nextPoll) {
                nextPoll = now + config.pollingIntervalMs;
                progression.Poll();
            }

            session.PollNetwork();
            session.UpdateControllers();

            Sleep(50);
        }

        LogInfo("Plugin", "SR2Archipelago shutting down");
        session.Shutdown();
        saveRevisions.Remove();
        log::Shutdown();
    } catch (const std::exception& error) {
        LogCritical(
            "Plugin",
            std::string("Unhandled initialization exception: ") + error.what());
        log::Flush();
    } catch (...) {
        LogCritical("Plugin",
                    "Unhandled non-standard initialization exception");
        log::Flush();
    }
    return 0;
}
}  // namespace sr2ap
