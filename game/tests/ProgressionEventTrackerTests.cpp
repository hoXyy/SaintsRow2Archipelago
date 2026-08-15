#include "sr2ap/ProgressionEventTracker.hpp"

#include <gtest/gtest.h>

namespace sr2ap {
    TEST(ProgressionEventTrackerTest, HitmanBaselineDoesNotEmitAndChangesEmitOnce) {
        ProgressionEventTracker tracker;
        HitmanSnapshot snapshot{ReaderResult::Success, {{"target", 1, 1, false}}};
        EXPECT_TRUE(tracker.Observe(snapshot).events.empty());
        snapshot.targets.front().complete = true;
        const auto changed = tracker.Observe(snapshot);
        ASSERT_EQ(changed.events.size(), 1U);
        EXPECT_EQ(changed.events.front().kind, ProgressionKind::Hitman);
        EXPECT_EQ(changed.events.front().key, "target");
        EXPECT_EQ(changed.events.front().previous, 0U);
        EXPECT_EQ(changed.events.front().current, 1U);
        EXPECT_TRUE(tracker.Observe(snapshot).events.empty());
    }

    TEST(ProgressionEventTrackerTest, UnavailabilityInvalidatesWithoutFalseRecoveryEvent) {
        ProgressionEventTracker tracker;
        HitmanSnapshot snapshot{ReaderResult::Success, {{"target", 1, 1, false}}};
        tracker.Observe(snapshot);
        snapshot.result = ReaderResult::GameNotReady;
        EXPECT_EQ(tracker.Observe(snapshot).kind, BaselineUpdateKind::Invalidated);
        EXPECT_EQ(tracker.Observe(snapshot).kind, BaselineUpdateKind::Unchanged);
        snapshot.result = ReaderResult::Success;
        snapshot.targets.front().complete = true;
        const auto recovered = tracker.Observe(snapshot);
        EXPECT_EQ(recovered.kind, BaselineUpdateKind::Created);
        EXPECT_TRUE(recovered.events.empty());
    }

    TEST(ProgressionEventTrackerTest, MissionsAndActivitiesEmitExistingProgressOnBaseline) {
        ProgressionEventTracker tracker;
        const auto missions =
            tracker.Observe(MissionSnapshot{ReaderResult::Success, {{"complete", true}, {"incomplete", false}}});
        ASSERT_EQ(missions.events.size(), 1U);
        EXPECT_EQ(missions.events.front().key, "complete");
        EXPECT_EQ(missions.events.front().current, 1U);

        const auto activities = tracker.Observe(ActivitySnapshot{ReaderResult::Success, {{"activity", 2, 3, 0x05}}});
        ASSERT_EQ(activities.events.size(), 1U);
        EXPECT_EQ(activities.events.front().key, "activity");
        EXPECT_EQ(activities.events.front().current, 0x05U);
    }

    TEST(ProgressionEventTrackerTest, IdentityChangeRecreatesMissionBaseline) {
        ProgressionEventTracker tracker;
        tracker.Observe(MissionSnapshot{ReaderResult::Success, {{"old", false}}});
        const auto update = tracker.Observe(MissionSnapshot{ReaderResult::Success, {{"new", true}}});
        EXPECT_EQ(update.kind, BaselineUpdateKind::IdentityChanged);
        ASSERT_EQ(update.events.size(), 1U);
        EXPECT_EQ(update.events.front().key, "new");
    }

    TEST(ProgressionEventTrackerTest, CdsEmitOnlyAdditionsAndRepresentUnknownIdsSafely) {
        ProgressionEventTracker tracker;
        const auto known = GetCdDefinitions().front();
        EXPECT_TRUE(tracker.Observe(CdSnapshot{ReaderResult::Success, 50, {}}).events.empty());
        const auto added = tracker.Observe(CdSnapshot{ReaderResult::Success, 50, {known.id, 1}});
        ASSERT_EQ(added.events.size(), 2U);
        bool foundKnown = false;
        bool foundUnknown = false;
        for (const auto& event : added.events) {
            foundKnown |= event.key == known.districtKey;
            foundUnknown |= event.key == "unknown";
        }
        EXPECT_TRUE(foundKnown);
        EXPECT_TRUE(foundUnknown);
        EXPECT_TRUE(tracker.Observe(CdSnapshot{ReaderResult::Success, 50, {}}).events.empty());
    }
}  // namespace sr2ap
