#include "sr2ap/Status.hpp"

#include <gtest/gtest.h>

namespace sr2ap {
    TEST(StatusSerializationTest, SerializesCountsFlagsAndSuccessfulEntries) {
        const HitmanSnapshot hitman{ReaderResult::Success, {{"hit_a", 1, 1, true}, {"hit_b", 1, 2, false}}};
        const ChopShopSnapshot chopShop{ReaderResult::Success, {{"car_a", 1, 1, true, 500, 10}}};
        const MissionSnapshot missions{ReaderResult::Success, {{"m01", true}, {"m02", false}}};
        const ActivitySnapshot activities{ReaderResult::Success, {{"act", 2, 3, 0x05}}};
        const RacingSnapshot racing{ReaderResult::Success, {{"car_pj", 13, 0x0EBCD323, RacingMedal::Gold, 81.994F, 0}}};
        const CdSnapshot cds{ReaderResult::Success, 50, {GetCdDefinitions().front().id}};

        const auto text = SerializeProgressionStatus(hitman, chopShop, missions, activities, racing, cds);
        EXPECT_NE(text.find("target_count=2\ncomplete_count=1"), std::string::npos);
        EXPECT_NE(text.find("hit_a=1\nhit_b=0"), std::string::npos);
        EXPECT_NE(text.find("vehicle_count=1\nretrieved_count=1"), std::string::npos);
        EXPECT_NE(text.find("mission_count=2\ncomplete_count=1"), std::string::npos);
        EXPECT_NE(text.find("act=2\nact_flags=0x05"), std::string::npos);
        EXPECT_NE(text.find("race_count=1\nmedal_count=1"), std::string::npos);
        EXPECT_NE(text.find("car_pj=3\ncar_pj_medal=gold\ncar_pj_best_time=81.994"), std::string::npos);
        EXPECT_NE(text.find(std::string{GetCdDefinitions().front().districtKey} + "=1"), std::string::npos);
    }

    TEST(StatusSerializationTest, HidesEntriesWhenReaderIsUnavailable) {
        const HitmanSnapshot hitman{ReaderResult::GameNotReady, {{"must_not_appear", 1, 1, true}}};
        const auto text = SerializeProgressionStatus(hitman, {}, {}, {}, {}, {});
        EXPECT_NE(text.find("game_ready=0"), std::string::npos);
        EXPECT_NE(text.find("result=game_not_ready"), std::string::npos);
        EXPECT_EQ(text.find("must_not_appear="), std::string::npos);
    }

    TEST(StatusSerializationTest, EmitsCdDefinitionsInStableAlphabeticalOrder) {
        CdSnapshot cds{ReaderResult::Success, 50, {}};
        const auto text = SerializeProgressionStatus({}, {}, {}, {}, {}, cds);
        EXPECT_LT(text.find("adept_way_1="), text.find("amberbrook_1="));
        EXPECT_LT(text.find("amberbrook_1="), text.find("athos_bay_1="));
    }
}  // namespace sr2ap
