#include "sr2ap/ReaderResult.hpp"

#include <gtest/gtest.h>

namespace sr2ap {
    TEST(ReaderResultTest, ConvertsEveryValueToItsProtocolName) {
        EXPECT_STREQ(ToString(ReaderResult::Success), "success");
        EXPECT_STREQ(ToString(ReaderResult::UnsupportedVersion), "unsupported_version");
        EXPECT_STREQ(ToString(ReaderResult::GameNotReady), "game_not_ready");
        EXPECT_STREQ(ToString(ReaderResult::ManagerUnavailable), "manager_unavailable");
        EXPECT_STREQ(ToString(ReaderResult::InvalidPointer), "invalid_pointer");
        EXPECT_STREQ(ToString(ReaderResult::InvalidData), "invalid_data");
        EXPECT_STREQ(ToString(ReaderResult::InvalidFunction), "invalid_function");
        EXPECT_STREQ(ToString(ReaderResult::ReaderUnavailable), "reader_unavailable");
    }
}  // namespace sr2ap
