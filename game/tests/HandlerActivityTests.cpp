#include "sr2ap/HandlerActivity.hpp"

#include <gtest/gtest.h>

namespace sr2ap {
    TEST(HandlerActivityTest, CountsConcurrentLeases) {
        HandlerActivity activity;
        EXPECT_TRUE(activity.IsIdle());
        {
            auto first = activity.Acquire();
            auto second = activity.Acquire();
            EXPECT_TRUE(first);
            EXPECT_TRUE(second);
            EXPECT_FALSE(activity.IsIdle());
        }
        EXPECT_TRUE(activity.IsIdle());
    }

    TEST(HandlerActivityTest, RejectsNewLeasesWhileStoppedAndCanRestart) {
        HandlerActivity activity;
        {
            auto active = activity.Acquire();
            activity.Stop();
            EXPECT_FALSE(activity.Acquire());
            EXPECT_FALSE(activity.IsIdle());
        }
        EXPECT_TRUE(activity.IsIdle());
        activity.Start();
        EXPECT_TRUE(activity.Acquire());
    }
}  // namespace sr2ap
