#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "sr2ap/RevisionJournal.hpp"

namespace sr2ap {
TEST(RevisionJournalTest,
     PersistsMultipleSessionScopedRevisionsAndAcknowledgesExactValue) {
    RevisionJournal journal;
    const RevisionSession first{"seed", 0, 1};
    const RevisionSession second{"seed", 0, 2};
    journal.Record(first, 0x12345678, 17);
    journal.Record(first, 0xABCDEF01, 22);
    journal.Record(second, 0x12345678, 99);

    const auto path = std::filesystem::temp_directory_path() /
                      "sr2ap_revision_journal_test.json";
    {
        std::ofstream output(path, std::ios::trunc);
        output << journal.Serialize();
    }

    RevisionJournal restored;
    ASSERT_TRUE(restored.Load(path));
    std::filesystem::remove(path);
    const auto pending = restored.Pending(first);
    ASSERT_EQ(pending.size(), 2U);
    EXPECT_EQ(pending[0].checksum, 0x12345678U);
    EXPECT_EQ(pending[0].nextIndex, 17U);
    EXPECT_FALSE(restored.Acknowledge(first, 0x12345678, 18));
    EXPECT_TRUE(restored.Acknowledge(first, 0x12345678, 17));
    EXPECT_EQ(restored.Pending(first).size(), 1U);
    ASSERT_EQ(restored.Pending(second).size(), 1U);
    EXPECT_EQ(restored.Pending(second)[0].nextIndex, 99U);
}

TEST(RevisionJournalTest, RejectsMalformedJournal) {
    const auto path = std::filesystem::temp_directory_path() /
                      "sr2ap_bad_revision_journal_test.json";
    {
        std::ofstream output(path, std::ios::trunc);
        output << R"({"version":1,"sessions":{"seed|0|1":{"not-hex":2}}})";
    }
    RevisionJournal journal;
    EXPECT_FALSE(journal.Load(path));
    EXPECT_FALSE(journal.LastError().empty());
    std::filesystem::remove(path);
}

TEST(RevisionJournalTest, FailedLoadPreservesExistingState) {
    RevisionJournal journal;
    const RevisionSession session{"seed", 0, 1};
    journal.Record(session, 0x12345678, 17);

    const auto path = std::filesystem::temp_directory_path() /
                      "sr2ap_transactional_revision_journal_test.json";
    {
        std::ofstream output(path, std::ios::trunc);
        output << R"({"version":1,"sessions":{"seed|0|1":{"not-hex":2}}})";
    }

    EXPECT_FALSE(journal.Load(path));
    std::filesystem::remove(path);
    const auto pending = journal.Pending(session);
    ASSERT_EQ(pending.size(), 1U);
    EXPECT_EQ(pending[0].checksum, 0x12345678U);
    EXPECT_EQ(pending[0].nextIndex, 17U);
}

TEST(RevisionJournalTest, RejectsDuplicateNormalizedChecksums) {
    const auto path = std::filesystem::temp_directory_path() /
                      "sr2ap_duplicate_revision_journal_test.json";
    {
        std::ofstream output(path, std::ios::trunc);
        output
            << R"({"version":1,"sessions":{"seed|0|1":{"A":1,"0000000a":2}}})";
    }

    RevisionJournal journal;
    EXPECT_FALSE(journal.Load(path));
    EXPECT_NE(journal.LastError().find("duplicate"), std::string_view::npos);
    std::filesystem::remove(path);
}
}  // namespace sr2ap
