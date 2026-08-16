#include "sr2ap/RevisionJournal.hpp"

#include <cstdio>
#include <fstream>
#include <json.hpp>
#include <limits>

using json = nlohmann::json;

namespace sr2ap {
    namespace {
        constexpr std::uint32_t journalVersion{1};
    }

    std::string RevisionJournal::SessionKey(const RevisionSession& session) {
        return session.seedName + "|" + std::to_string(session.team) + "|" + std::to_string(session.slot);
    }

    bool RevisionJournal::Load(const std::filesystem::path& path) {
        sessions_.clear();
        std::ifstream input(path);
        if (!input) {
            return !std::filesystem::exists(path);
        }

        const auto document = json::parse(input, nullptr, false);
        if (document.is_discarded() || !document.is_object() ||
            document.value("version", std::uint32_t{}) != journalVersion || !document.contains("sessions") ||
            !document["sessions"].is_object()) {
            return false;
        }

        for (const auto& [sessionKey, value] : document["sessions"].items()) {
            if (!value.is_object()) {
                return false;
            }
            Revisions revisions;
            for (const auto& [checksumText, nextIndexValue] : value.items()) {
                try {
                    std::size_t consumed{};
                    const auto checksumValue = std::stoull(checksumText, &consumed, 16);
                    if (consumed != checksumText.size() || checksumValue > std::numeric_limits<std::uint32_t>::max() ||
                        !nextIndexValue.is_number_unsigned()) {
                        return false;
                    }
                    revisions.emplace(static_cast<std::uint32_t>(checksumValue), nextIndexValue.get<std::uint64_t>());
                } catch (const std::exception&) {
                    return false;
                }
            }
            sessions_.emplace(sessionKey, std::move(revisions));
        }
        return true;
    }

    std::string RevisionJournal::Serialize() const {
        json sessionValues = json::object();
        for (const auto& [sessionKey, revisions] : sessions_) {
            json revisionValues = json::object();
            for (const auto& [checksum, nextIndex] : revisions) {
                char checksumText[9]{};
                std::snprintf(checksumText, sizeof(checksumText), "%08X", checksum);
                revisionValues[checksumText] = nextIndex;
            }
            sessionValues[sessionKey] = std::move(revisionValues);
        }
        return json{{"version", journalVersion}, {"sessions", std::move(sessionValues)}}.dump(2);
    }

    void RevisionJournal::Record(const RevisionSession& session,
                                 const std::uint32_t checksum,
                                 const std::uint64_t nextIndex) {
        sessions_[SessionKey(session)][checksum] = nextIndex;
    }

    bool RevisionJournal::Acknowledge(const RevisionSession& session,
                                      const std::uint32_t checksum,
                                      const std::uint64_t nextIndex) {
        const auto sessionIt = sessions_.find(SessionKey(session));
        if (sessionIt == sessions_.end()) {
            return false;
        }
        const auto revisionIt = sessionIt->second.find(checksum);
        if (revisionIt == sessionIt->second.end() || revisionIt->second != nextIndex) {
            return false;
        }
        sessionIt->second.erase(revisionIt);
        if (sessionIt->second.empty()) {
            sessions_.erase(sessionIt);
        }
        return true;
    }

    std::vector<SaveRevision> RevisionJournal::Pending(const RevisionSession& session) const {
        std::vector<SaveRevision> result;
        const auto sessionIt = sessions_.find(SessionKey(session));
        if (sessionIt == sessions_.end()) {
            return result;
        }
        result.reserve(sessionIt->second.size());
        for (const auto& [checksum, nextIndex] : sessionIt->second) {
            result.push_back({checksum, nextIndex});
        }
        return result;
    }
}  // namespace sr2ap
