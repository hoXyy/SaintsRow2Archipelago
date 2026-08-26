#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
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
    RevisionJournal();
    ~RevisionJournal();

    RevisionJournal(const RevisionJournal&) = delete;
    RevisionJournal& operator=(const RevisionJournal&) = delete;

    [[nodiscard]] bool Load(const std::filesystem::path& path);
    [[nodiscard]] std::string Serialize() const;
    [[nodiscard]] std::string_view LastError() const noexcept;

    void Record(const RevisionSession& session, std::uint32_t checksum,
                std::uint64_t nextIndex);
    [[nodiscard]] bool Acknowledge(const RevisionSession& session,
                                   std::uint32_t checksum,
                                   std::uint64_t nextIndex);
    [[nodiscard]] std::vector<SaveRevision> Pending(
        const RevisionSession& session) const;

   private:
    class Implementation;
    std::unique_ptr<Implementation> implementation_;
};
}  // namespace sr2ap
