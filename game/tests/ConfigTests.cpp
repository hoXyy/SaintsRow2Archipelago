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

    TEST(ConfigTest, ParseFullDefaultConfig) {
        const auto result = ParseConfig(R"ini(
[General]
Enabled=1

[Progression]
PollingIntervalMs=1000 ; how often to poll the game for progression state
LogFullSnapshots=0
LogStateChanges=0 ; whether to log progression state changes (for ex. a mission pass)

[Network]
Enabled=1 ; shouldn't ever need to disable this
Port=38282 ; port to use to connect to the AP client, currently only changable on the client's end using the CLI

[Debug]
DebugLogging=0
WriteStatusFile=0
EnableHotkeys=0
ModuleReportHotkey=0x76
SnapshotHotkey=0x77
AddressDumpHotkey=0x78
)ini");
        EXPECT_EQ(result.warnings, 0U);
        EXPECT_TRUE(result.config.enabled);
        EXPECT_FALSE(result.config.debugLogging);
        EXPECT_EQ(result.config.pollingIntervalMs, 1000U);
        EXPECT_FALSE(result.config.logFullSnapshots);
        EXPECT_FALSE(result.config.logStateChanges);
        EXPECT_TRUE(result.config.networkEnabled);
        EXPECT_EQ(result.config.networkPort, 38282U);
        EXPECT_FALSE(result.config.enableHotkeys);
        EXPECT_FALSE(result.config.writeStatusFile);
        EXPECT_EQ(result.config.moduleReportHotkey, 0x76U);
        EXPECT_EQ(result.config.snapshotHotkey, 0x77U);
        EXPECT_EQ(result.config.addressDumpHotkey, 0x78U);
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

    TEST(ConfigTest, ParseConfigWithSameLineComments) {
        const auto result = ParseConfig("[Network]\nPort=12345 ; test comment\n");
        EXPECT_EQ(result.config.networkPort, 12345U);
    }

    TEST(ConfigTest, ParseConfigWithMultiLineComments) {
        const auto result = ParseConfig("; test comment \n [Network]\nPort=12345\n");
        EXPECT_EQ(result.config.networkPort, 12345U);
    }

    TEST(ConfigTest, ParseConfigWithBothTypeOfComments) {
        const auto result = ParseConfig("; test comment 1 \n [Network]\nPort=12345 ; test comment 2\n");
        EXPECT_EQ(result.config.networkPort, 12345U);
    }
}  // namespace sr2ap
