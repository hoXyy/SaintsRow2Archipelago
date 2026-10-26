#include "Plugin.hpp"

#include <windows.h>

#include <atomic>

#include "game/ActivityListMonitor.hpp"
#include "game/DeathMonitor.hpp"
#include "game/GameState.hpp"
#include "game/GameThreadDispatcher.hpp"
#include "game/HudNotifications.hpp"
#include "game/JobLevelMonitor.hpp"
#include "game/MapReveal.hpp"
#include "game/MenuVersionOverlay.hpp"
#include "game/ModuleInfo.hpp"
#include "game/SaveRevisionMonitor.hpp"
#include "game/TextRenderer.hpp"
#include "progression/ProgressionMonitor.hpp"
#include "runtime/ItemNotifications.hpp"
#include "runtime/SessionRuntime.hpp"
#include "util/Config.hpp"
#include "util/Helpers.hpp"
#include "util/Logger.hpp"

#ifndef SR2AP_VERSION
#define SR2AP_VERSION "dev"
#endif

namespace sr2ap {
namespace {
std::atomic<bool> shutdownRequested{false};

ConnectionStatus ToConnectionStatus(const rust::ConnectionState state) {
    switch (state) {
        case rust::ConnectionState::WaitingForSession:
            return ConnectionStatus::WaitingForSession;
        case rust::ConnectionState::Active:
            return ConnectionStatus::Active;
        default:
            return ConnectionStatus::Disconnected;
    }
}

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

        GameThreadDispatcher dispatcher;
        if (gameSupported && !dispatcher.Install()) {
            LogWarning("Plugin",
                       "Game-thread dispatch unavailable; AP item delivery "
                       "and notifications disabled");
        }

        TextRenderer textRenderer;
        if (gameSupported) {
            if (textRenderer.Install()) {
                LogInfo("TextRenderer", "On-screen text installed");
            } else {
                LogWarning("TextRenderer", "On-screen text unavailable");
            }
        }
        MenuVersionOverlay menuOverlay{textRenderer};
        menuOverlay.Install();
        HudNotifications notifications{dispatcher};
        if (gameSupported && !notifications.Install()) {
            LogWarning("Notifications", "HUD notifications unavailable");
        }
        MapReveal mapReveal{dispatcher};
        if (gameSupported && config.enabled && !mapReveal.Install()) {
            LogWarning("MapReveal", "Map reveal unavailable");
        }
        const bool showConnection = config.enabled && config.networkEnabled;
        if (!showConnection) {
            menuOverlay.SetConnectionStatus(ConnectionStatus::Disabled);
        }

        const auto game = InspectSupportedGameModule();
        SessionRuntime session{
            gameSupported, pluginDirectory / L"SR2ArchipelagoRevisions.json",
            game, dispatcher};
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

        DeathMonitor deathMonitor;
        if (session.InstallDeathMonitoring(deathMonitor, config.enabled)) {
            LogInfo("DeathLink", "Player death monitoring installed");
        } else if (config.enabled && gameSupported) {
            LogWarning("DeathLink", "Player death monitoring unavailable");
        }

        ActivityListMonitor listMonitor;
        if (session.InstallListMonitoring(listMonitor, config.enabled)) {
            LogInfo("Lists", "Hitman/Chop Shop list monitoring installed");
        } else if (config.enabled && gameSupported) {
            LogWarning("Lists", "List monitoring unavailable; no list hints");
        }

        JobLevelMonitor jobMonitor;
        if (session.InstallJobMonitoring(jobMonitor, config.enabled)) {
            LogInfo("Jobs", "Job level monitoring installed");
        } else if (config.enabled && gameSupported) {
            LogWarning("Jobs",
                       "Job level monitoring unavailable; no job checks");
        }

        if (config.enabled) {
            if (config.debugLogging) {
                LogDebug("Progression",
                         "Polling enabled interval_ms=" +
                             std::to_string(config.pollingIntervalMs));
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

        bool moduleDown = false, snapshotDown = false, deathLinkDown = false;
        bool communicationsWereActive = false;
        ULONGLONG nextPoll = GetTickCount64();
        const auto statusPath = pluginDirectory / L"SR2ArchipelagoStatus.txt";
        ProgressionMonitor progression(
            statusPath,
            [&session](const ProgressionEvent& event) {
                session.SendProgression(event);
            },
            config.writeStatusFile);

        while (!shutdownRequested.load(std::memory_order_acquire)) {
            const auto gameContext = ReadGameContext(game);

            if (saveRevisionInstalled) {
                saveRevisions.Poll();
            }
            deathMonitor.Poll(gameContext);
            listMonitor.Poll();
            jobMonitor.Poll();
            session.ProcessGameEvents();

            session.UpdateReadiness(gameContext.readiness);

            if (config.enableHotkeys) {
                if (Pressed(config.moduleReportHotkey, moduleDown)) {
                    ReportAllModules(plugin);
                }
                if (Pressed(config.snapshotHotkey, snapshotDown)) {
                    progression.CaptureManualSnapshot(gameContext);
                }
                if (Pressed(config.deathLinkHotkey, deathLinkDown)) {
                    session.QueueTestDeathLink();
                }
            }
            // A new AP session has no record of earlier progress, so the
            // next poll re-reports everything the loaded save has.
            const bool communicationsActive = session.CommunicationsActive();
            if (communicationsActive && !communicationsWereActive) {
                progression.Reset();
            }
            communicationsWereActive = communicationsActive;

            const auto now = GetTickCount64();
            if (config.enabled &&
                (communicationsActive || config.writeStatusFile) &&
                now >= nextPoll) {
                nextPoll = now + config.pollingIntervalMs;
                progression.Poll(gameContext);
            }

            session.PollNetwork(gameContext);
            if (const auto death = session.UpdateDeathLink(gameContext)) {
                notifications.Post(
                    FormatDeathLinkNotification(death->source, death->cause));
            }
            while (const auto notification = session.NextNotification()) {
                if (const auto text = FormatItemNotification(*notification)) {
                    notifications.Post(*text);
                }
            }
            notifications.Update();
            mapReveal.Update(gameContext);
            if (showConnection) {
                menuOverlay.SetConnectionStatus(
                    ToConnectionStatus(session.ClientConnectionState()));
            }
            session.UpdateItemHandlers();

            Sleep(50);
        }

        LogInfo("Plugin", "SR2Archipelago shutting down");
        session.Shutdown();
        jobMonitor.Remove();
        listMonitor.Remove();
        deathMonitor.Remove();
        saveRevisions.Remove();
        mapReveal.Remove();
        notifications.Remove();
        menuOverlay.Remove();
        textRenderer.Remove();
        dispatcher.Remove();
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
