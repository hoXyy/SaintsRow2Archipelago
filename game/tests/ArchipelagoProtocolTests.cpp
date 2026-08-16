#include "sr2ap/ArchipelagoProtocol.hpp"

#include <gtest/gtest.h>

namespace sr2ap {
    TEST(ArchipelagoProtocolTest, ParsesReceivedItemAndRejectsInvalidFields) {
        const auto item = ParseReceivedItemMessage(R"({"type":"item","index":17,"name":"Vehicle: Super Taxi"})");
        ASSERT_TRUE(item);
        EXPECT_EQ(item->index, 17U);
        EXPECT_EQ(item->name, "Vehicle: Super Taxi");
        EXPECT_FALSE(ParseReceivedItemMessage(R"({"type":"item","index":-1,"name":"x"})"));
        EXPECT_FALSE(ParseReceivedItemMessage(R"({"type":"item","index":1.5,"name":"x"})"));
        EXPECT_FALSE(ParseReceivedItemMessage(R"({"type":"item","index":17})"));
        EXPECT_FALSE(ParseReceivedItemMessage("not json"));
    }

    TEST(ArchipelagoProtocolTest, ParsesSaveContextWithIntegerBoundaries) {
        const auto context =
            ParseSaveContextMessage(R"({"type":"save_context","checksum":4294967295,"next_index":18})");
        ASSERT_TRUE(context);
        EXPECT_EQ(context->checksum, 4294967295U);
        EXPECT_FALSE(ParseSaveContextMessage(R"({"type":"save_context","checksum":4294967296,"next_index":18})"));
        EXPECT_FALSE(ParseSaveContextMessage(R"({"type":"save_context","checksum":1,"next_index":-1})"));
    }

    TEST(ArchipelagoProtocolTest, ParsesSaveRevisionAcknowledgement) {
        const auto acknowledgement = ParseSaveRevisionAcknowledgementMessage(
            R"({"type":"save_revision_ack","checksum":4294967295,"next_index":18,"accepted":true})");
        ASSERT_TRUE(acknowledgement);
        EXPECT_EQ(acknowledgement->checksum, 4294967295U);
        EXPECT_EQ(acknowledgement->nextIndex, 18U);
        EXPECT_TRUE(acknowledgement->accepted);
        EXPECT_FALSE(
            ParseSaveRevisionAcknowledgementMessage(R"({"type":"save_revision_ack","checksum":1,"next_index":18})"));
    }

    TEST(ArchipelagoProtocolTest, ParsesCompleteSessionAndDeduplicatesManagedNames) {
        const auto session = ParseSessionReadyMessage(R"({
          "type":"session_ready","protocol":3,"seed_name":"seed","team":1,"slot":2,
          "managed_unlockables":["Taxi","Taxi"],"managed_cheats":["Evil Cars","Evil Cars"],
          "features":{"exclusive_respect":true,"block_vanilla_unlockables":false,"notoriety_traps":true},
          "enabled_progression":{"missions":true,"activities":false,"hitman":true,"chop_shop":false,"cds":true}})");
        ASSERT_TRUE(session);
        EXPECT_EQ(session->managedUnlockables, std::vector<std::string>{"Taxi"});
        EXPECT_EQ(session->managedCheats, std::vector<std::string>{"Evil Cars"});
        EXPECT_TRUE(session->exclusiveRespect);
        EXPECT_FALSE(session->activities);
    }

    TEST(ArchipelagoProtocolTest, RejectsIncompleteOrOversizedSessions) {
        EXPECT_FALSE(
            ParseSessionReadyMessage(R"({"type":"session_ready","protocol":3,"seed_name":"x","team":0,"slot":1})"));
        std::string longName(129, 'x');
        EXPECT_FALSE(ParseSessionReadyMessage("{\"type\":\"session_ready\",\"protocol\":2,\"seed_name\":\"" + longName +
                                              "\",\"team\":0,\"slot\":1}"));
    }

    TEST(ArchipelagoProtocolTest, SerializesEveryProgressionCategory) {
        const std::pair<ProgressionKind, const char*> cases[]{{ProgressionKind::Hitman, "hitman"},
                                                              {ProgressionKind::ChopShop, "chop_shop"},
                                                              {ProgressionKind::Mission, "mission"},
                                                              {ProgressionKind::Activity, "activity"},
                                                              {ProgressionKind::Cd, "cd"}};
        for (const auto& [kind, category] : cases) {
            const auto json = SerializeProgressionEvent({kind, "key", 1, 3});
            EXPECT_NE(json.find(std::string{"\"category\":\""} + category + '"'), std::string::npos);
            EXPECT_NE(json.find("\"previous\":1"), std::string::npos);
            EXPECT_NE(json.find("\"current\":3"), std::string::npos);
        }
    }

    TEST(ArchipelagoProtocolTest, SerializesContextAndAcknowledgements) {
        EXPECT_EQ(SerializeItemAcknowledgement(17, true), R"({"accepted":true,"index":17,"type":"item_ack"})");
        EXPECT_EQ(SerializeGameContext(std::nullopt, 4, true, false),
                  R"({"checksum":null,"needs_cursor":false,"next_index":4,"provisional":true,"type":"game_context"})");
        EXPECT_EQ(SerializeSaveRevision(5, 18), R"({"checksum":5,"next_index":18,"type":"save_revision"})");
        EXPECT_TRUE(IsSessionEndMessage(R"({"type":"session_end"})"));
        EXPECT_FALSE(IsSessionEndMessage(R"({"type":"session_ready"})"));
    }
}  // namespace sr2ap
