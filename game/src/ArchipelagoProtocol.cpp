#include "sr2ap/ArchipelagoProtocol.hpp"
#include <json.hpp>
#include <limits>
#include <unordered_set>

using json = nlohmann::json;

namespace sr2ap {
    namespace {
        constexpr std::size_t maximumListSize{256};
        constexpr std::size_t maximumNameSize{128};

        std::optional<json> ParseObject(const std::string_view message, const std::string_view type) {
            auto parsed = json::parse(message, nullptr, false);
            if (parsed.is_discarded() || !parsed.is_object() || parsed.value("type", std::string{}) != type) {
                return std::nullopt;
            }
            return parsed;
        }

        template <class T>
        std::optional<T> ReadUnsigned(const json& object, const char* key) {
            if (!object.contains(key) || !object[key].is_number_unsigned()) {
                return std::nullopt;
            }
            const auto value = object[key].get<std::uint64_t>();
            if (value > std::numeric_limits<T>::max()) {
                return std::nullopt;
            }
            return static_cast<T>(value);
        }

        std::optional<bool> ReadBoolean(const json& object, const char* key) {
            if (!object.contains(key) || !object[key].is_boolean()) {
                return std::nullopt;
            }
            return object[key].get<bool>();
        }

        std::optional<std::vector<std::string>> ReadNames(const json& object, const char* key) {
            if (!object.contains(key) || !object[key].is_array() || object[key].size() > maximumListSize) {
                return std::nullopt;
            }

            std::vector<std::string> names;
            std::unordered_set<std::string> seen;
            for (const auto& value : object[key]) {
                if (!value.is_string()) {
                    return std::nullopt;
                }
                auto name = value.get<std::string>();
                if (name.empty() || name.size() > maximumNameSize) {
                    return std::nullopt;
                }
                if (seen.emplace(name).second) {
                    names.push_back(std::move(name));
                }
            }
            return names;
        }

        const char* ToString(const ProgressionKind kind) noexcept {
            switch (kind) {
                case ProgressionKind::Hitman:
                    return "hitman";
                case ProgressionKind::ChopShop:
                    return "chop_shop";
                case ProgressionKind::Mission:
                    return "mission";
                case ProgressionKind::Activity:
                    return "activity";
                case ProgressionKind::Cd:
                    return "cd";
            }

            return "unknown";
        }
    }  // namespace

    std::string SerializeProgressionEvent(const ProgressionEvent& event) {
        const json message = {{"type", "progression"},
                              {"category", ToString(event.kind)},
                              {"key", event.key},
                              {"previous", event.previous},
                              {"current", event.current}};

        return message.dump();
    }

    std::optional<ReceivedItemMessage> ParseReceivedItemMessage(const std::string_view message) {
        const auto parsed = ParseObject(message, "item");
        if (!parsed || !parsed->contains("name") || !(*parsed)["name"].is_string()) {
            return std::nullopt;
        }
        const auto index = ReadUnsigned<std::uint64_t>(*parsed, "index");
        if (!index) {
            return std::nullopt;
        }
        return ReceivedItemMessage{*index, (*parsed)["name"].get<std::string>()};
    }

    std::string SerializeItemAcknowledgement(std::uint64_t index, bool accepted) {
        return json{{"type", "item_ack"}, {"index", index}, {"accepted", accepted}}.dump();
    }

    std::string SerializeSessionReject(const std::string_view reason, const std::string_view message) {
        return json{{"type", "session_rejected"}, {"reason", reason}, {"message", message}}.dump();
    }

    std::optional<SaveContextMessage> ParseSaveContextMessage(const std::string_view message) {
        const auto parsed = ParseObject(message, "save_context");
        if (!parsed) {
            return std::nullopt;
        }
        const auto checksum = ReadUnsigned<std::uint32_t>(*parsed, "checksum");
        const auto nextIndex = ReadUnsigned<std::uint64_t>(*parsed, "next_index");
        if (!checksum || !nextIndex) {
            return std::nullopt;
        }
        return SaveContextMessage{*checksum, *nextIndex};
    }

    std::optional<SessionReadyMessage> ParseSessionReadyMessage(const std::string_view message) {
        const auto parsed = ParseObject(message, "session_ready");
        if (!parsed || !parsed->contains("seed_name") || !(*parsed)["seed_name"].is_string()) {
            return std::nullopt;
        }
        const auto protocol = ReadUnsigned<std::uint32_t>(*parsed, "protocol");
        const auto team = ReadUnsigned<std::uint32_t>(*parsed, "team");
        const auto slot = ReadUnsigned<std::uint32_t>(*parsed, "slot");
        const auto seedName = (*parsed)["seed_name"].get<std::string>();
        if (!protocol || !team || !slot || seedName.empty() || seedName.size() > maximumNameSize) {
            return std::nullopt;
        }
        const auto unlockables = ReadNames(*parsed, "managed_unlockables");
        const auto cheats = ReadNames(*parsed, "managed_cheats");

        if (!unlockables || !cheats || !parsed->contains("features") || !(*parsed)["features"].is_object() ||
            !parsed->contains("enabled_progression") || !(*parsed)["enabled_progression"].is_object()) {
            return std::nullopt;
        }

        const auto& features = (*parsed)["features"];
        const auto& progression = (*parsed)["enabled_progression"];
        const auto exclusiveRespect = ReadBoolean(features, "exclusive_respect");
        const auto blockUnlockables = ReadBoolean(features, "block_vanilla_unlockables");
        const auto notorietyTraps = ReadBoolean(features, "notoriety_traps");
        const auto missions = ReadBoolean(progression, "missions");
        const auto activities = ReadBoolean(progression, "activities");
        const auto hitman = ReadBoolean(progression, "hitman");
        const auto chopShop = ReadBoolean(progression, "chop_shop");
        const auto cds = ReadBoolean(progression, "cds");
        if (!exclusiveRespect || !blockUnlockables || !notorietyTraps || !missions || !activities || !hitman ||
            !chopShop || !cds) {
            return std::nullopt;
        }
        return SessionReadyMessage{
            *protocol,         seedName,        *team,     *slot,       *unlockables, *cheats,   *exclusiveRespect,
            *blockUnlockables, *notorietyTraps, *missions, *activities, *hitman,      *chopShop, *cds};
    }

    bool IsSessionEndMessage(const std::string_view message) {
        return ParseObject(message, "session_end").has_value();
    }

    std::string SerializeGameContext(const std::optional<std::uint32_t> checksum,
                                     std::uint64_t nextIndex,
                                     bool provisional,
                                     bool needsCursor) {
        json message{{"type", "game_context"},
                     {"next_index", nextIndex},
                     {"provisional", provisional},
                     {"needs_cursor", needsCursor}};
        if (checksum) {
            message["checksum"] = *checksum;
        } else {
            message["checksum"] = nullptr;
        }
        return message.dump();
    }

    std::string SerializeSaveRevision(std::uint32_t checksum, std::uint64_t nextIndex) {
        return json{{"type", "save_revision"}, {"checksum", checksum}, {"next_index", nextIndex}}.dump();
    }
}  // namespace sr2ap
