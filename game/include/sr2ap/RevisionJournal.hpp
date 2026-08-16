#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace sr2ap {
    struct RevisionSession {
        std::string seedName;
        std::uint32_t team{};
        std::uint32_t slot{};
    };

    struct SaveRevision {
        std::uint32_t checksum{};
        std::uint64_t nextIndex{};
    };

    class RevisionJournal {
       public:
        [[nodiscard]] bool Load(const std::filesystem::path& path);
        [[nodiscard]] std::string Serialize() const;

        void Record(const RevisionSession& session, std::uint32_t checksum, std::uint64_t nextIndex);
        [[nodiscard]] bool Acknowledge(const RevisionSession& session, std::uint32_t checksum, std::uint64_t nextIndex);
        [[nodiscard]] std::vector<SaveRevision> Pending(const RevisionSession& session) const;

       private:
        using Revisions = std::map<std::uint32_t, std::uint64_t>;
        std::map<std::string, Revisions> sessions_;

        [[nodiscard]] static std::string SessionKey(const RevisionSession& session);
    };
}  // namespace sr2ap
