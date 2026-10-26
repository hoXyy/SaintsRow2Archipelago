#include <gtest/gtest.h>

#include <memory>

#include "util/GameEvents.hpp"

namespace sr2ap {
TEST(GameEventsTest, PreservesGameThreadOrder) {
    GameEventQueue queue;
    queue.PushRequestCompleted(1, true);
    queue.PushSaveWritten(0xAABBCCDD);
    queue.PushRequestCompleted(2, true);

    const auto events = queue.Drain();
    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(events[0].kind, GameEventKind::RequestCompleted);
    EXPECT_EQ(events[0].requestId, 1U);
    EXPECT_EQ(events[1].kind, GameEventKind::SaveWritten);
    EXPECT_EQ(events[1].checksum, 0xAABBCCDDU);
    EXPECT_EQ(events[2].requestId, 2U);
    EXPECT_TRUE(queue.Drain().empty());
}

TEST(GameEventsTest, PlayerDeathKeepsGameThreadOrder) {
    GameEventQueue queue;
    queue.PushSaveWritten(0x11);
    queue.PushPlayerDied();
    queue.PushRequestCompleted(4, true);

    const auto events = queue.Drain();
    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(events[1].kind, GameEventKind::PlayerDied);
    EXPECT_EQ(events[2].kind, GameEventKind::RequestCompleted);
}

TEST(GameEventsTest, ListOpenedCarriesListAndKeepsOrder) {
    GameEventQueue queue;
    queue.PushListOpened(ActivityList::ChopShop, 3);
    queue.PushPlayerDied();
    queue.PushListOpened(ActivityList::Hitman, 5);

    const auto events = queue.Drain();
    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(events[0].kind, GameEventKind::ListOpened);
    EXPECT_EQ(events[0].activityList, ActivityList::ChopShop);
    EXPECT_EQ(events[0].list, 3U);
    EXPECT_EQ(events[1].kind, GameEventKind::PlayerDied);
    EXPECT_EQ(events[2].activityList, ActivityList::Hitman);
    EXPECT_EQ(events[2].list, 5U);
}

TEST(GameEventsTest, JobLevelCompletedCarriesJobAndLevel) {
    GameEventQueue queue;
    queue.PushJobLevelCompleted(Job::Taxi, 4);
    queue.PushJobLevelCompleted(Job::Hoing, 1);

    const auto events = queue.Drain();
    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].kind, GameEventKind::JobLevelCompleted);
    EXPECT_EQ(events[0].job, Job::Taxi);
    EXPECT_EQ(events[0].level, 4U);
    EXPECT_EQ(events[1].job, Job::Hoing);
    EXPECT_EQ(events[1].level, 1U);
}

TEST(GameEventsTest, DroppedCompletionReportsNotApplied) {
    auto queue = std::make_shared<GameEventQueue>();
    {
        ItemCompletion completion{queue, 7};
        auto copy = completion;
    }

    const auto events = queue->Drain();
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].requestId, 7U);
    EXPECT_FALSE(events[0].applied);
}

TEST(GameEventsTest, FirstCompletionWins) {
    auto queue = std::make_shared<GameEventQueue>();
    {
        ItemCompletion completion{queue, 3};
        completion.Complete(true);
        completion.Complete(false);
    }

    const auto events = queue->Drain();
    ASSERT_EQ(events.size(), 1U);
    EXPECT_TRUE(events[0].applied);
}

TEST(GameEventsTest, LoadBeforeDrainInvalidatesIssuedWork) {
    auto queue = std::make_shared<GameEventQueue>();
    ItemCompletion issued{queue, 1};
    EXPECT_TRUE(issued.CanApply());

    queue->PushSaveLoaded(0x1234);
    EXPECT_FALSE(issued.CanApply());

    // Work issued before the plugin thread drained the load still belongs to
    // the old world.
    ItemCompletion beforeDrain{queue, 2};
    EXPECT_FALSE(beforeDrain.IsCurrent());

    (void)queue->Drain();
    ItemCompletion afterDrain{queue, 3};
    EXPECT_TRUE(afterDrain.IsCurrent());
}

TEST(GameEventsTest, InvalidateWorldRejectsEarlierWork) {
    auto queue = std::make_shared<GameEventQueue>();
    ItemCompletion issued{queue, 1};
    queue->InvalidateWorld();
    EXPECT_FALSE(issued.IsCurrent());
    EXPECT_TRUE((ItemCompletion{queue, 2}.IsCurrent()));
}

TEST(GameEventsTest, CanApplyRequiresInteractiveGameplay) {
    bool interactive = false;
    auto queue = std::make_shared<GameEventQueue>(
        [&interactive] { return interactive; });
    ItemCompletion completion{queue, 1};
    EXPECT_TRUE(completion.IsCurrent());
    EXPECT_FALSE(completion.CanApply());
    interactive = true;
    EXPECT_TRUE(completion.CanApply());
}
}  // namespace sr2ap
