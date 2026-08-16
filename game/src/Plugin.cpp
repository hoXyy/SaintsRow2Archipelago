#include "sr2ap/Plugin.hpp"
#include "sr2ap/ArchipelagoProtocol.hpp"
#include "sr2ap/ArchipelagoTcpClient.hpp"
#include "sr2ap/AtomicFile.hpp"
#include "sr2ap/Cheats.hpp"
#include "sr2ap/Config.hpp"
#include "sr2ap/GameState.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/ModuleInfo.hpp"
#include "sr2ap/Notoriety.hpp"
#include "sr2ap/ProgressionMonitor.hpp"
#include "sr2ap/Respect.hpp"
#include "sr2ap/RevisionJournal.hpp"
#include "sr2ap/SaveRevisionMonitor.hpp"
#include "sr2ap/Unlockables.hpp"

#include <windows.h>
#include <atomic>
#include <iomanip>
#include <sstream>

#ifndef SR2AP_VERSION
#define SR2AP_VERSION "dev"
#endif

namespace sr2ap {
    namespace {
        std::atomic<bool> shutdownRequested{false};

        enum class DeliveryContextState {
            waitingForGameplay,
            provisional,
            awaitingCursor,
            activeRevision,
        };

        bool IsDeliveryReady(const DeliveryContextState state) {
            return state == DeliveryContextState::provisional || state == DeliveryContextState::activeRevision;
        }

        std::string Narrow(const std::filesystem::path& path) {
            const auto wide = path.wstring();
            if (wide.empty()) {
                return {};
            }
            const auto needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (needed <= 1) {
                return {};
            }
            std::string result(static_cast<std::size_t>(needed), '\0');
            WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, result.data(), needed, nullptr, nullptr);
            result.resize(static_cast<std::size_t>(needed - 1));
            return result;
        }

        std::string Hex(std::uintptr_t value, int width = 8) {
            std::ostringstream stream;
            stream << "0x" << std::uppercase << std::hex << std::setw(width) << std::setfill('0') << value;
            return stream.str();
        }

        void ReportModule(const char* subsystem, const ModuleInfo& info) {
            LogInfo(subsystem, "path=" + Narrow(info.path));
            LogInfo(subsystem, "base=" + Hex(info.base) + " image_size=" + Hex(info.imageSize) +
                                   " pe_timestamp=" + Hex(info.peTimestamp));
        }

        void ReportAllModules(HMODULE plugin) {
            if (const auto executable = InspectModule(GetModuleHandleW(nullptr))) {
                ReportModule("Module", *executable);
            } else {
                LogError("Module", "Unable to inspect SR2_pc.exe");
            }
            if (const auto juiced = FindLoadedModule(L"DFEngine.dll")) {
                ReportModule("Juiced", *juiced);
            } else {
                LogInfo("Juiced", "DFEngine.dll not loaded");
            }
            if (const auto self = InspectModule(plugin)) {
                ReportModule("Plugin", *self);
            }
        }

        bool Pressed(std::uint32_t key, bool& previous) {
            const bool current = key <= 0xFF && (GetAsyncKeyState(static_cast<int>(key)) & 0x8000) != 0;
            const bool edge = current && !previous;
            previous = current;
            return edge;
        }

        class SessionRuntime {
           public:
            SessionRuntime(const Config& config, bool gameSupported, std::filesystem::path revisionJournalPath)
                : revisionJournalPath_{std::move(revisionJournalPath)},
                  blockVanillaUnlockables_{config.blockVanillaUnlockables},
                  gameSupported_{gameSupported} {
                revisionJournalAvailable_ = revisionJournal_.Load(revisionJournalPath_);
                if (!revisionJournalAvailable_) {
                    LogError("SaveRevision", "Could not load durable revision journal; AP item delivery disabled");
                }
            }

            bool InstallSaveMonitoring(SaveRevisionMonitor& monitor, bool enabled) {
                saveMonitoringInstalled_ =
                    enabled && gameSupported_ &&
                    monitor.Install([this](std::uint32_t checksum) { OnSaveLoaded(checksum); },
                                    [this](std::uint32_t checksum) { OnSaveWritten(checksum); },
                                    [this](std::uint32_t threadId) { respect_.PermitNextSaveRestore(threadId); });
                return saveMonitoringInstalled_;
            }

            void Connect(std::uint16_t port) {
                client_.emplace(port, [this](std::string_view message) { HandleMessage(message); });
            }

            void PollNetwork() {
                if (!client_) {
                    return;
                }
                client_->Poll();
                const bool connected = client_->IsConnected();
                if (!connected && networkWasConnected_) {
                    communicationsActive_ = false;
                    LogInfo("Session", "TCP disconnected; gameplay policy remains latched");
                }
                networkWasConnected_ = connected;
            }

            void UpdateReadiness(GameReadiness previous, GameReadiness current) {
                if (previous == GameReadiness::GameplayReady && current == GameReadiness::MainMenu) {
                    activeSaveChecksum_.reset();
                    nextItemIndex_ = 0;
                    deliveryState_ = DeliveryContextState::waitingForGameplay;
                    LogInfo("Items", "Gameplay ended; delivery context cleared");
                } else if (current == GameReadiness::GameplayReady &&
                           deliveryState_ == DeliveryContextState::waitingForGameplay) {
                    activeSaveChecksum_.reset();
                    nextItemIndex_ = 0;
                    deliveryState_ = DeliveryContextState::provisional;
                    LogInfo("Items", "Gameplay ready without save load; provisional cursor initialized");
                    SendGameContext();
                }
            }

            void SendProgression(const ProgressionEvent& event) {
                if (client_ && communicationsActive_ && sessionConfiguration_ && ProgressionEnabled(event.kind)) {
                    client_->SendLine(SerializeProgressionEvent(event));
                }
            }

            bool CommunicationsActive() const noexcept {
                return communicationsActive_;
            }

            void UpdateControllers() {
                if (respectInstalled_) {
                    respect_.Update();
                }
            }

            void Shutdown() {
                client_.reset();
                unlockables_.Remove();
                respect_.Remove();
                notoriety_.Remove();
                cheats_.Remove();
            }

           private:
            void SendGameContext() {
                if (!client_ || !communicationsActive_ || revisionSyncPending_ ||
                    deliveryState_ == DeliveryContextState::waitingForGameplay) {
                    return;
                }
                client_->SendLine(SerializeGameContext(activeSaveChecksum_, nextItemIndex_,
                                                       deliveryState_ == DeliveryContextState::provisional,
                                                       deliveryState_ == DeliveryContextState::awaitingCursor));
            }

            RevisionSession CurrentRevisionSession() const {
                return {sessionConfiguration_->seedName, sessionConfiguration_->team, sessionConfiguration_->slot};
            }

            bool PersistRevisionJournal() {
                if (ReplaceFileAtomically(revisionJournalPath_, revisionJournal_.Serialize())) {
                    return true;
                }
                revisionJournalAvailable_ = false;
                LogError("SaveRevision", "Could not persist durable revision journal; AP item delivery disabled");
                return false;
            }

            void SendPendingRevisions() {
                if (!client_ || !sessionConfiguration_) {
                    return;
                }
                for (const auto& revision : revisionJournal_.Pending(CurrentRevisionSession())) {
                    client_->SendLine(SerializeSaveRevision(revision.checksum, revision.nextIndex));
                }
            }

            void BeginRevisionSync() {
                revisionSyncPending_ = true;
                const auto pending = revisionJournal_.Pending(CurrentRevisionSession());
                SendPendingRevisions();
                if (pending.empty()) {
                    revisionSyncPending_ = false;
                    SendGameContext();
                } else {
                    LogInfo("SaveRevision", "Synchronizing " + std::to_string(pending.size()) +
                                                " durable revision(s) before item delivery");
                }
            }

            void HandleSaveRevisionAcknowledgement(const SaveRevisionAcknowledgementMessage& acknowledgement) {
                if (!sessionConfiguration_ || !acknowledgement.accepted) {
                    LogError("SaveRevision", "Client rejected revision checksum=" + Hex(acknowledgement.checksum) +
                                                 " next_index=" + std::to_string(acknowledgement.nextIndex));
                    return;
                }
                if (!revisionJournal_.Acknowledge(CurrentRevisionSession(), acknowledgement.checksum,
                                                  acknowledgement.nextIndex)) {
                    LogWarning("SaveRevision", "Ignored stale or mismatched revision acknowledgement checksum=" +
                                                   Hex(acknowledgement.checksum));
                    return;
                }
                if (!PersistRevisionJournal()) {
                    return;
                }
                LogInfo("SaveRevision", "Revision persisted by client checksum=" + Hex(acknowledgement.checksum) +
                                            " next_index=" + std::to_string(acknowledgement.nextIndex));
                if (revisionSyncPending_ && revisionJournal_.Pending(CurrentRevisionSession()).empty()) {
                    revisionSyncPending_ = false;
                    SendGameContext();
                }
            }

            void OnSaveLoaded(std::uint32_t checksum) {
                activeSaveChecksum_ = checksum;
                nextItemIndex_ = 0;
                deliveryState_ = DeliveryContextState::awaitingCursor;
                LogInfo("SaveRevision", "Loaded checksum=" + Hex(checksum) + "; awaiting AP cursor");
                SendGameContext();
            }

            void OnSaveWritten(std::uint32_t checksum) {
                if (!IsDeliveryReady(deliveryState_)) {
                    LogWarning("SaveRevision", "Generated save checksum before AP cursor was established");
                    return;
                }
                activeSaveChecksum_ = checksum;
                deliveryState_ = DeliveryContextState::activeRevision;
                if (!sessionConfiguration_ || !revisionJournalAvailable_) {
                    LogWarning("SaveRevision", "Could not associate generated checksum with an AP session");
                    return;
                }
                revisionJournal_.Record(CurrentRevisionSession(), checksum, nextItemIndex_);
                if (!PersistRevisionJournal()) {
                    return;
                }
                LogInfo("SaveRevision",
                        "Generated checksum=" + Hex(checksum) + " next_index=" + std::to_string(nextItemIndex_));
                if (communicationsActive_) {
                    SendPendingRevisions();
                }
            }

            bool SameSession(const SessionReadyMessage& session) const {
                return sessionConfiguration_ && session.seedName == sessionConfiguration_->seedName &&
                       session.team == sessionConfiguration_->team && session.slot == sessionConfiguration_->slot;
            }

            bool SupportsManagedItems(const SessionReadyMessage& session) const {
                return std::all_of(session.managedUnlockables.begin(), session.managedUnlockables.end(),
                                   [](const auto& name) { return UnlockableController::SupportsItem(name); }) &&
                       std::all_of(session.managedCheats.begin(), session.managedCheats.end(),
                                   [](const auto& name) { return CheatController::SupportsItem(name); });
            }

            bool InstallPolicies(const SessionReadyMessage& session) {
                const bool cheatsRequested = !session.managedCheats.empty();
                const bool notorietyRequested = session.notorietyTraps;
                const bool respectRequested = session.exclusiveRespect;
                const bool unlockablesRequested = !session.managedUnlockables.empty();

                cheatsInstalled_ = cheatsRequested && cheats_.Install(session.managedCheats);
                notorietyInstalled_ = notorietyRequested && notoriety_.Install();
                respectInstalled_ = respectRequested && saveMonitoringInstalled_ && respect_.Install();
                unlockablesInstalled_ = unlockablesRequested && unlockables_.Install(session.blockVanillaUnlockables &&
                                                                                         blockVanillaUnlockables_,
                                                                                     session.managedUnlockables);

                const bool failed =
                    (cheatsRequested && !cheatsInstalled_) || (notorietyRequested && !notorietyInstalled_) ||
                    (respectRequested && !respectInstalled_) || (unlockablesRequested && !unlockablesInstalled_);
                if (!failed) {
                    return true;
                }

                unlockables_.Remove();
                respect_.Remove();
                notoriety_.Remove();
                cheats_.Remove();
                cheatsInstalled_ = notorietyInstalled_ = respectInstalled_ = unlockablesInstalled_ = false;
                return false;
            }

            void HandleSession(const SessionReadyMessage& session) {
                if (session.protocol != 3 || !gameSupported_ || !revisionJournalAvailable_) {
                    LogWarning("Session", "Rejected unsupported session protocol, executable, or revision journal");
                    return;
                }
                if (sessionLatched_) {
                    if (!SameSession(session)) {
                        LogWarning("Session", "Rejected conflicting AP session; restart SR2 to switch");

                        if (client_) {
                            constexpr std::string_view message =
                                "Conflicting AP session detected by game integration plugin, please restart Saints Row "
                                "2.";

                            client_->SendLine(SerializeSessionReject("session_reject", message));
                        }

                        return;
                    }
                    communicationsActive_ = true;
                    LogInfo("Session", "Authenticated AP session resumed");
                    BeginRevisionSync();
                    return;
                }
                if (!SupportsManagedItems(session)) {
                    LogWarning("Session", "Rejected session containing unsupported managed items");
                    return;
                }
                if (!InstallPolicies(session)) {
                    LogError("Session", "AP gameplay policy activation failed; session rejected");
                    return;
                }

                sessionConfiguration_ = session;
                sessionLatched_ = true;
                communicationsActive_ = true;
                LogInfo("Session", "AP gameplay policy activated seed=" + session.seedName + " team=" +
                                       std::to_string(session.team) + " slot=" + std::to_string(session.slot));
                BeginRevisionSync();
            }

            void HandleSaveContext(const SaveContextMessage& context) {
                if (!communicationsActive_ || !saveMonitoringInstalled_ ||
                    deliveryState_ != DeliveryContextState::awaitingCursor || !activeSaveChecksum_ ||
                    context.checksum != *activeSaveChecksum_) {
                    LogWarning("Items", "Rejected save context for inactive checksum=" + Hex(context.checksum));
                    return;
                }
                nextItemIndex_ = context.nextIndex;
                deliveryState_ = DeliveryContextState::activeRevision;
                LogInfo("Items", "Save context ready checksum=" + Hex(context.checksum) +
                                     " next_index=" + std::to_string(nextItemIndex_));
            }

            void Acknowledge(std::uint64_t index, bool accepted) {
                if (client_) {
                    client_->SendLine(SerializeItemAcknowledgement(index, accepted));
                }
            }

            void HandleItem(const ReceivedItemMessage& item) {
                if (!communicationsActive_ || !IsDeliveryReady(deliveryState_) ||
                    GetGameReadiness() != GameReadiness::GameplayReady) {
                    LogWarning("Items",
                               "Rejected item while gameplay context unavailable index=" + std::to_string(item.index));
                    Acknowledge(item.index, false);
                    return;
                }
                if (item.index < nextItemIndex_) {
                    LogInfo("Items", "Acknowledged duplicate index=" + std::to_string(item.index));
                    Acknowledge(item.index, true);
                    return;
                }
                if (item.index != nextItemIndex_) {
                    LogWarning("Items", "Rejected out-of-order index=" + std::to_string(item.index) +
                                            " expected=" + std::to_string(nextItemIndex_));
                    Acknowledge(item.index, false);
                    return;
                }

                const bool accepted =
                    cheats_.ActivateReceivedItem(item.name) || notoriety_.ActivateReceivedItem(item.name) ||
                    respect_.ActivateReceivedItem(item.name) || unlockables_.QueueReceivedItem(item.name);
                if (accepted) {
                    ++nextItemIndex_;
                } else {
                    LogWarning("Items", "Rejected received item: " + item.name);
                }
                Acknowledge(item.index, accepted);
            }

            void HandleMessage(std::string_view message) {
                if (const auto session = ParseSessionReadyMessage(message)) {
                    HandleSession(*session);
                } else if (IsSessionEndMessage(message)) {
                    communicationsActive_ = false;
                    LogInfo("Session", "AP communications ended; gameplay policy remains latched");
                } else if (const auto context = ParseSaveContextMessage(message)) {
                    HandleSaveContext(*context);
                } else if (const auto acknowledgement = ParseSaveRevisionAcknowledgementMessage(message)) {
                    HandleSaveRevisionAcknowledgement(*acknowledgement);
                } else if (const auto item = ParseReceivedItemMessage(message)) {
                    HandleItem(*item);
                }
            }

            bool ProgressionEnabled(ProgressionKind kind) const {
                switch (kind) {
                    case ProgressionKind::Hitman:
                        return sessionConfiguration_->hitman;
                    case ProgressionKind::ChopShop:
                        return sessionConfiguration_->chopShop;
                    case ProgressionKind::Mission:
                        return sessionConfiguration_->missions;
                    case ProgressionKind::Activity:
                        return sessionConfiguration_->activities;
                    case ProgressionKind::Cd:
                        return sessionConfiguration_->cds;
                }
                return false;
            }

            CheatController cheats_;
            NotorietyController notoriety_;
            RespectController respect_;
            UnlockableController unlockables_;
            std::optional<ArchipelagoTcpClient> client_;
            std::optional<SessionReadyMessage> sessionConfiguration_;
            RevisionJournal revisionJournal_;
            std::filesystem::path revisionJournalPath_;
            std::optional<std::uint32_t> activeSaveChecksum_;
            std::uint64_t nextItemIndex_{};
            DeliveryContextState deliveryState_{DeliveryContextState::waitingForGameplay};
            bool blockVanillaUnlockables_{};
            bool gameSupported_{};
            bool saveMonitoringInstalled_{};
            bool cheatsInstalled_{};
            bool notorietyInstalled_{};
            bool respectInstalled_{};
            bool unlockablesInstalled_{};
            bool sessionLatched_{};
            bool communicationsActive_{};
            bool networkWasConnected_{};
            bool revisionJournalAvailable_{};
            bool revisionSyncPending_{};
        };
    }  // namespace

    void RequestShutdown() {
        shutdownRequested.store(true, std::memory_order_release);
    }

    DWORD WINAPI PluginThread(void* parameter) {
        try {
            const auto plugin = static_cast<HMODULE>(parameter);
            const auto pluginInfo = InspectModule(plugin);
            const auto pluginDirectory = pluginInfo && !pluginInfo->path.empty() ? pluginInfo->path.parent_path()
                                                                                 : std::filesystem::current_path();
            const auto configPath = pluginDirectory / L"SR2Archipelago.ini";
            const auto [config, fileFound, warnings] = LoadConfig(configPath);
            log::Initialize(pluginDirectory, config.debugLogging);
            LogInfo("Plugin", std::string("SR2Archipelago loaded version=") + SR2AP_VERSION);
            LogInfo("Plugin", "compiler=MSVC " + std::to_string(_MSC_VER));
#ifdef NDEBUG
            LogInfo("Plugin", "build_configuration=Release architecture=x86");
#else
            LogInfo("Plugin", "build_configuration=Debug architecture=x86");
#endif
            LogInfo("Config", "path=" + Narrow(configPath) + (fileFound ? " loaded=1" : " loaded=0 defaults_used=1"));
            if (warnings) {
                LogWarning("Config", "Malformed values ignored/clamped: " + std::to_string(warnings));
            }

            ReportAllModules(plugin);
            const auto executable = InspectModule(GetModuleHandleW(nullptr));
            const bool gameSupported = executable && IsSupportedExecutable(*executable);
            if (!gameSupported) {
                LogWarning("Plugin", "Unsupported executable; game-specific hooks are disabled.");
            } else {
                LogInfo("Plugin", "Supported executable found.");
            }

            SessionRuntime session{config, gameSupported, pluginDirectory / L"SR2ArchipelagoRevisions.json"};
            SaveRevisionMonitor saveRevisions;
            const bool saveRevisionInstalled = session.InstallSaveMonitoring(saveRevisions, config.enabled);
            if (saveRevisionInstalled) {
                LogInfo("SaveRevision", "Checksum load/write monitoring installed");
            } else if (config.enabled && gameSupported) {
                LogWarning("SaveRevision", "Checksum monitoring unavailable; AP item delivery disabled");
            }

            if (config.enabled) {
                if (config.debugLogging) {
                    LogDebug("Hitman", "Polling enabled interval_ms=" + std::to_string(config.pollingIntervalMs));
                    LogDebug("ChopShop", "Polling enabled interval_ms=" + std::to_string(config.pollingIntervalMs));
                    LogDebug("Missions", "Polling enabled interval_ms=" + std::to_string(config.pollingIntervalMs) +
                                             " base_game_missions=56 pc_dlc_missions=excluded");
                    LogDebug("Activities", "Polling enabled interval_ms=" + std::to_string(config.pollingIntervalMs) +
                                               " expected_instances=24");
                    LogDebug("CDs",
                             "Polling enabled interval_ms=" + std::to_string(config.pollingIntervalMs) + " target=50");
                }
                if (config.networkEnabled) {
                    session.Connect(config.networkPort);
                    LogInfo("Network", "Connecting to AP client on 127.0.0.1:" + std::to_string(config.networkPort));
                }
            } else {
                LogInfo("Plugin", "Plugin disabled by configuration; diagnostic hotkeys remain available");
            }

            bool moduleDown = false, snapshotDown = false, dumpDown = false;
            ULONGLONG nextPoll = GetTickCount64();
            const auto statusPath = pluginDirectory / L"SR2ArchipelagoStatus.txt";
            ProgressionMonitor progression(
                statusPath, [&session](const ProgressionEvent& event) { session.SendProgression(event); });
            auto previousReadiness = GetGameReadiness();

            while (!shutdownRequested.load(std::memory_order_acquire)) {
                if (saveRevisionInstalled) {
                    saveRevisions.Poll();
                }

                const auto readiness = GetGameReadiness();
                session.UpdateReadiness(previousReadiness, readiness);
                previousReadiness = readiness;

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
                if (config.enabled && session.CommunicationsActive() && now >= nextPoll) {
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
            LogCritical("Plugin", std::string("Unhandled initialization exception: ") + error.what());
            log::Flush();
        } catch (...) {
            LogCritical("Plugin", "Unhandled non-standard initialization exception");
            log::Flush();
        }
        return 0;
    }
}  // namespace sr2ap
