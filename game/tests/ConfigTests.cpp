#include "sr2ap/Config.hpp"

#include <gtest/gtest.h>

namespace sr2ap {
    class BooleanConfigTest : public testing::TestWithParam<const char*> {};

    TEST_P(BooleanConfigTest, AcceptsTrueFormsCaseInsensitively) {
        const auto result = ParseConfig(std::string{"[General]\nEnabled="} + GetParam());
        EXPECT_TRUE(result.config.enabled);
        EXPECT_EQ(result.warnings, 0U);
    }

    INSTANTIATE_TEST_SUITE_P(TrueForms, BooleanConfigTest, testing::Values("1", "true", "TRUE", "yes", "ON"));

    TEST(ConfigTest, DefaultsAreStable) {
        const auto result = ParseConfig("");
        EXPECT_TRUE(result.config.enabled);
        EXPECT_EQ(result.config.pollingIntervalMs, 1000U);
        EXPECT_EQ(result.config.networkPort, 38282U);
        EXPECT_EQ(result.warnings, 0U);
    }

    TEST(ConfigTest, LoadsEverySectionAndHexadecimalHotkeys) {
        const auto result = ParseConfig(R"ini(
[General]
Enabled=0
[Progression]
PollingIntervalMs=1500
LogFullSnapshots=off
LogStateChanges=0
[Network]
Enabled=off
Port=65535
[Unlockables]
BlockVanillaRewards=yes
[Debug]
DebugLogging=false
EnableHotkeys=false
ModuleReportHotkey=0x70
SnapshotHotkey=0X71
AddressDumpHotkey=114
)ini");
        EXPECT_EQ(result.warnings, 0U);
        EXPECT_FALSE(result.config.enabled);
        EXPECT_FALSE(result.config.debugLogging);
        EXPECT_EQ(result.config.pollingIntervalMs, 1500U);
        EXPECT_FALSE(result.config.logFullSnapshots);
        EXPECT_FALSE(result.config.logStateChanges);
        EXPECT_FALSE(result.config.networkEnabled);
        EXPECT_EQ(result.config.networkPort, 65535U);
        EXPECT_TRUE(result.config.blockVanillaUnlockables);
        EXPECT_FALSE(result.config.enableHotkeys);
        EXPECT_EQ(result.config.moduleReportHotkey, 0x70U);
        EXPECT_EQ(result.config.snapshotHotkey, 0x71U);
        EXPECT_EQ(result.config.addressDumpHotkey, 114U);
    }

    TEST(ConfigTest, RejectsMalformedAndOutOfRangeValuesWithoutReplacingDefaults) {
        const auto result = ParseConfig(
            "[General]\nEnabled=perhaps\n[Network]\nPort=65536\n"
            "[Debug]\nSnapshotHotkey=4294967296\n[Unknown]\nKey=value\n");
        EXPECT_TRUE(result.config.enabled);
        EXPECT_EQ(result.config.networkPort, 38282U);
        EXPECT_EQ(result.config.snapshotHotkey, 0x77U);
        EXPECT_EQ(result.warnings, 4U);
    }

    TEST(ConfigTest, ClampsPollingIntervalAtMinimum) {
        const auto below = ParseConfig("[Progression]\nPollingIntervalMs=99\n");
        EXPECT_EQ(below.config.pollingIntervalMs, 100U);
        EXPECT_EQ(below.warnings, 1U);
        const auto boundary = ParseConfig("[Progression]\nPollingIntervalMs=100\n");
        EXPECT_EQ(boundary.config.pollingIntervalMs, 100U);
        EXPECT_EQ(boundary.warnings, 0U);
    }

    TEST(ConfigTest, ParseConfigWithComments) {
        const auto result = ParseConfig("[Network]\nPort=12345 ; test comment\n");
        EXPECT_EQ(result.config.networkPort, 12345U);
    }
}  // namespace sr2ap
