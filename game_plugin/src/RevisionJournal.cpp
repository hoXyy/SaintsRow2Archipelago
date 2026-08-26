#include "sr2ap/RevisionJournal.hpp"

#include <cstdint>

#include "rust/cxx.h"
#include "sr2ap/src/ffi/revision_journal_ffi.rs.h"

namespace sr2ap {
namespace {
::rust::Slice<const std::uint8_t> Bytes(const std::string_view value) {
    return {reinterpret_cast<const std::uint8_t*>(value.data()), value.size()};
}

std::string ToString(const ::rust::String& value) {
    return {value.data(), value.size()};
}
}  // namespace

class RevisionJournal::Implementation {
   public:
    Implementation() : journal{rust::revision_journal_new()} {
    }

    ::rust::Box<rust::RevisionJournal> journal;
    std::string lastError;
};

RevisionJournal::RevisionJournal()
    : implementation_{std::make_unique<Implementation>()} {
}

RevisionJournal::~RevisionJournal() = default;

bool RevisionJournal::Load(const std::filesystem::path& path) {
    const auto& native = path.native();
    try {
        rust::revision_journal_load(
            *implementation_->journal,
            {reinterpret_cast<const std::uint16_t*>(native.data()),
             native.size()});
        implementation_->lastError.clear();
        return true;
    } catch (const ::rust::Error& error) {
        implementation_->lastError = error.what();
        return false;
    }
}

std::string RevisionJournal::Serialize() const {
    try {
        auto serialized =
            rust::revision_journal_serialize(*implementation_->journal);
        implementation_->lastError.clear();
        return ToString(serialized);
    } catch (const ::rust::Error& error) {
        implementation_->lastError = error.what();
        return {};
    }
}

std::string_view RevisionJournal::LastError() const noexcept {
    return implementation_->lastError;
}

void RevisionJournal::Record(const RevisionSession& session,
                             const std::uint32_t checksum,
                             const std::uint64_t nextIndex) {
    try {
        rust::revision_journal_record(*implementation_->journal,
                                      Bytes(session.seedName), session.team,
                                      session.slot, checksum, nextIndex);
        implementation_->lastError.clear();
    } catch (const ::rust::Error& error) {
        implementation_->lastError = error.what();
    }
}

bool RevisionJournal::Acknowledge(const RevisionSession& session,
                                  const std::uint32_t checksum,
                                  const std::uint64_t nextIndex) {
    try {
        const auto acknowledged = rust::revision_journal_acknowledge(
            *implementation_->journal, Bytes(session.seedName), session.team,
            session.slot, checksum, nextIndex);
        implementation_->lastError.clear();
        return acknowledged;
    } catch (const ::rust::Error& error) {
        implementation_->lastError = error.what();
        return false;
    }
}

std::vector<SaveRevision> RevisionJournal::Pending(
    const RevisionSession& session) const {
    try {
        const auto pending = rust::revision_journal_pending(
            *implementation_->journal, Bytes(session.seedName), session.team,
            session.slot);
        std::vector<SaveRevision> result;
        result.reserve(pending.size());
        for (const auto& revision : pending) {
            result.push_back({revision.checksum, revision.next_index});
        }
        implementation_->lastError.clear();
        return result;
    } catch (const ::rust::Error& error) {
        implementation_->lastError = error.what();
        return {};
    }
}
}  // namespace sr2ap
