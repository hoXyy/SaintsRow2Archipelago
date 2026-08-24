#include "sr2ap/BaselineTracker.hpp"

#include <gtest/gtest.h>
#include <string>

namespace sr2ap {
    TEST(BaselineTrackerTest, TracksChangesAndIdentity) {
        BaselineTracker<std::string, bool> tracker;
        EXPECT_EQ(tracker.Observe({{"alpha", false}, {"beta", true}}).kind, BaselineUpdateKind::Created);
        EXPECT_TRUE(tracker.IsValid());
        EXPECT_EQ(tracker.Observe({{"alpha", false}, {"beta", true}}).kind, BaselineUpdateKind::Unchanged);

        const auto changed = tracker.Observe({{"alpha", true}, {"beta", true}});
        ASSERT_EQ(changed.kind, BaselineUpdateKind::Changed);
        ASSERT_EQ(changed.changes.size(), 1U);
        EXPECT_EQ(changed.changes.front().key, "alpha");
        EXPECT_FALSE(changed.changes.front().previous);
        EXPECT_TRUE(changed.changes.front().current);

        const auto replaced = tracker.Observe({{"alpha", true}, {"gamma", false}});
        EXPECT_EQ(replaced.kind, BaselineUpdateKind::IdentityChanged);
        EXPECT_TRUE(replaced.changes.empty());
    }

    TEST(BaselineTrackerTest, InvalidatesAndRecoversIdempotently) {
        BaselineTracker<std::string, bool> tracker;
        tracker.Observe({{"alpha", true}});
        EXPECT_EQ(tracker.Invalidate().kind, BaselineUpdateKind::Invalidated);
        EXPECT_FALSE(tracker.IsValid());
        EXPECT_EQ(tracker.Invalidate().kind, BaselineUpdateKind::Unchanged);
        EXPECT_EQ(tracker.Observe({{"alpha", false}}).kind, BaselineUpdateKind::Created);
    }
}  // namespace sr2ap
