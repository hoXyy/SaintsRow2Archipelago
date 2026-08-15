#include "sr2ap/Collectibles.hpp"

#include <gtest/gtest.h>
#include <string>
#include <unordered_set>

namespace sr2ap {
    TEST(CollectibleDefinitionsTest, ContainsFiftyUniqueRoundTrippableDefinitions) {
        const auto& definitions = GetCdDefinitions();
        ASSERT_EQ(definitions.size(), 50U);
        std::unordered_set<std::uint32_t> ids;
        std::unordered_set<std::string> keys;
        for (const auto& definition : definitions) {
            EXPECT_NE(definition.id, 0U);
            EXPECT_TRUE(ids.emplace(definition.id).second) << definition.id;
            ASSERT_NE(definition.districtKey, nullptr);
            EXPECT_TRUE(keys.emplace(definition.districtKey).second) << definition.districtKey;
            EXPECT_STREQ(FindCdDistrictKey(definition.id), definition.districtKey);
        }
        EXPECT_EQ(FindCdDistrictKey(0), nullptr);
    }
}  // namespace sr2ap
