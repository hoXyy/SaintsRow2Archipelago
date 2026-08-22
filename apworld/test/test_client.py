import unittest
from types import SimpleNamespace
from unittest.mock import AsyncMock, Mock

from NetUtils import ClientStatus
from ..activities import ACTIVITY_LEVEL_IDS, RACES, MEDAL_STRINGS
from ..client import (
    process_progression,
    send_goal_if_complete,
    HITMAN_LOCATIONS,
    CHOP_SHOP_LOCATIONS,
    submit_observed_locations,
    PROTOCOL_VERSION,
    send_session_ready,
    process_plugin_message,
)
from ..collectibles import CD_IDS, CD_MAPPING
from ..missions import get_mission_by_key


def make_context(missing_locations: set[int]):
    return SimpleNamespace(
        observed_locations=set(),
        missing_locations=set(missing_locations),
        locations_checked=set(),
        slot_data=None,
        finished_game=False,
        send_msgs=AsyncMock(),
    )


def make_ready_context():
    return SimpleNamespace(
        session_sent=False,
        plugin_hello=True,
        inventory_synced=True,
        seed_name="Test Seed",
        team=0,
        slot=1,
        slot_data={
            "protocol": PROTOCOL_VERSION,
            "managed_unlockables": ["Vehicle: Taxi", "Vehicle: Taxi"],
            "managed_cheats": ["$1,000", "$1,000"],
            "features": {
                "exclusive_respect": True,
                "block_vanilla_unlockables": True,
                "notoriety_traps": False,
            },
            "enabled_progression": {
                "missions": True,
                "activities": True,
                "hitman": True,
                "chop_shop": True,
                "cds": True,
                "races": True,
            },
        },
        send_plugin=AsyncMock(return_value=True),
    )


def make_revision_context(item_count: int = 5):
    return SimpleNamespace(
        items_received=[object() for _ in range(item_count)],
        stored_revision_cursor=Mock(return_value=None),
        persist_revision=Mock(),
        send_plugin=AsyncMock(return_value=True),
    )


class TestClientProgression(unittest.IsolatedAsyncioTestCase):
    async def test_mission_progression_submits_check(self) -> None:
        mission = get_mission_by_key("rn01")
        ctx = make_context({mission.id})

        await process_progression(
            ctx,
            {
                "category": "mission",
                "key": mission.key,
                "previous": 0,
                "current": 1,
            },
        )

        self.assertEqual({mission.id}, ctx.observed_locations)
        self.assertEqual({mission.id}, ctx.locations_checked)

        ctx.send_msgs.assert_awaited_once_with(
            [{"cmd": "LocationChecks", "locations": [mission.id]}]
        )

    async def test_activity_reports_only_new_bits(self) -> None:
        level_2 = ACTIVITY_LEVEL_IDS["Crowd Control (Hotels & Marina) - Level 2"]
        level_3 = ACTIVITY_LEVEL_IDS["Crowd Control (Hotels & Marina) - Level 3"]

        ctx = make_context({level_2, level_3})

        await process_progression(
            ctx,
            {
                "category": "activity",
                "key": "crowd_ht",
                # Level 1 was already complete.
                "previous": 0b000001,
                # Levels 2 and 3 are newly complete.
                "current": 0b000111,
            },
        )

        self.assertEqual({level_2, level_3}, ctx.observed_locations)

    async def test_gold_race_reports_all_medals(self) -> None:
        race = RACES["car_pj"]
        expected = {
            ACTIVITY_LEVEL_IDS[f"{race} - {MEDAL_STRINGS['bronze']}"],
            ACTIVITY_LEVEL_IDS[f"{race} - {MEDAL_STRINGS['silver']}"],
            ACTIVITY_LEVEL_IDS[f"{race} - {MEDAL_STRINGS['gold']}"],
        }
        ctx = make_context(expected)

        await process_progression(
            ctx,
            {
                "category": "racing",
                "key": "car_pj",
                "previous": 0,
                "current": 3,
            },
        )

        self.assertEqual(expected, ctx.observed_locations)
        self.assertEqual(expected, ctx.locations_checked)

    async def test_cd_progression_submits_check(self) -> None:
        key, name = next(iter(CD_MAPPING.items()))
        location_id = CD_IDS[name]
        ctx = make_context({location_id})

        await process_progression(
            ctx,
            {
                "category": "cd",
                "key": key,
                "previous": 0,
                "current": 1,
            },
        )

        self.assertEqual({location_id}, ctx.observed_locations)
        self.assertEqual({location_id}, ctx.locations_checked)

        ctx.send_msgs.assert_awaited_once_with(
            [{"cmd": "LocationChecks", "locations": [location_id]}]
        )

    async def test_hitman_progression_submits_check(self) -> None:
        key, location_id = next(iter(HITMAN_LOCATIONS.items()))
        ctx = make_context({location_id})

        await process_progression(
            ctx,
            {
                "category": "hitman",
                "key": key,
                "previous": 0,
                "current": 1,
            },
        )

        self.assertEqual({location_id}, ctx.observed_locations)
        self.assertEqual({location_id}, ctx.locations_checked)

        ctx.send_msgs.assert_awaited_once_with(
            [{"cmd": "LocationChecks", "locations": [location_id]}]
        )

    async def test_chop_shop_progression_submits_check(self) -> None:
        key, location_id = next(iter(CHOP_SHOP_LOCATIONS.items()))
        ctx = make_context({location_id})

        await process_progression(
            ctx,
            {
                "category": "chop_shop",
                "key": key,
                "previous": 0,
                "current": 1,
            },
        )

        self.assertEqual({location_id}, ctx.observed_locations)
        self.assertEqual({location_id}, ctx.locations_checked)

        ctx.send_msgs.assert_awaited_once_with(
            [{"cmd": "LocationChecks", "locations": [location_id]}]
        )

    async def test_unknown_progression_is_ignored(self) -> None:
        ctx = make_context({1, 2, 3})

        await process_progression(
            ctx,
            {
                "category": "mission",
                "key": "unknown",
                "previous": 0,
                "current": 1,
            },
        )

        self.assertEqual(set(), ctx.observed_locations)
        ctx.send_msgs.assert_not_awaited()

    async def test_checked_location_is_not_resubmitted(self) -> None:
        mission = get_mission_by_key("rn01")
        ctx = make_context({mission.id})

        ctx.observed_locations.add(mission.id)
        ctx.locations_checked.add(mission.id)

        await submit_observed_locations(ctx)

        ctx.send_msgs.assert_not_awaited()

    async def test_location_not_in_missing_locations_is_not_submitted(self) -> None:
        mission = get_mission_by_key("rn01")

        # An empty missing_locations means this check is not part of this seed,
        # or the server no longer considers it missing.
        ctx = make_context(set())
        ctx.observed_locations.add(mission.id)

        await submit_observed_locations(ctx)

        self.assertNotIn(mission.id, ctx.locations_checked)
        ctx.send_msgs.assert_not_awaited()

    async def test_new_locations_are_submitted_in_sorted_order(self) -> None:
        ctx = make_context({10, 20, 30})
        ctx.observed_locations.update({30, 10, 20})

        await submit_observed_locations(ctx)

        ctx.send_msgs.assert_awaited_once_with(
            [{"cmd": "LocationChecks", "locations": [10, 20, 30]}]
        )


class TestClientGoal(unittest.IsolatedAsyncioTestCase):
    async def test_goal_sent_after_every_goal_location(self) -> None:
        ctx = SimpleNamespace(
            finished_game=False,
            slot_data={"goal_locations": [16, 31]},
            locations_checked={16, 31},
            send_msgs=AsyncMock(),
        )

        await send_goal_if_complete(ctx)

        self.assertTrue(ctx.finished_game)
        ctx.send_msgs.assert_awaited_once_with(
            [
                {
                    "cmd": "StatusUpdate",
                    "status": ClientStatus.CLIENT_GOAL,
                }
            ]
        )

    async def test_partial_goal_does_not_finish(self) -> None:
        ctx = SimpleNamespace(
            finished_game=False,
            slot_data={"goal_locations": [16, 31]},
            locations_checked={16},
            send_msgs=AsyncMock(),
        )

        await send_goal_if_complete(ctx)

        self.assertFalse(ctx.finished_game)
        ctx.send_msgs.assert_not_awaited()

    async def test_goal_is_not_sent_twice(self) -> None:
        ctx = SimpleNamespace(
            finished_game=True,
            slot_data={"goal_locations": [16]},
            locations_checked={16},
            send_msgs=AsyncMock(),
        )

        await send_goal_if_complete(ctx)

        ctx.send_msgs.assert_not_awaited()


class TestSessionReady(unittest.IsolatedAsyncioTestCase):
    async def test_ready_context_sends_session(self) -> None:
        ctx = make_ready_context()

        await send_session_ready(ctx)

        self.assertTrue(ctx.session_sent)
        ctx.send_plugin.assert_awaited_once()

        message = ctx.send_plugin.await_args.args[0]

        self.assertEqual("session_ready", message["type"])
        self.assertEqual(PROTOCOL_VERSION, message["protocol"])
        self.assertEqual("Test Seed", message["seed_name"])
        self.assertEqual(0, message["team"])
        self.assertEqual(1, message["slot"])

    async def test_session_deduplicates_managed_names(self) -> None:
        ctx = make_ready_context()
        ctx.slot_data["managed_unlockables"] = ["B", "A", "B"]
        ctx.slot_data["managed_cheats"] = ["D", "C", "D"]

        await send_session_ready(ctx)

        message = ctx.send_plugin.await_args.args[0]

        self.assertEqual(["A", "B"], message["managed_unlockables"])
        self.assertEqual(["C", "D"], message["managed_cheats"])

    async def test_wrong_protocol_is_not_sent(self) -> None:
        ctx = make_ready_context()
        ctx.slot_data["protocol"] = PROTOCOL_VERSION + 1

        await send_session_ready(ctx)

        self.assertFalse(ctx.session_sent)
        ctx.send_plugin.assert_not_awaited()

    async def test_plugin_hello_is_required(self) -> None:
        ctx = make_ready_context()
        ctx.plugin_hello = False

        await send_session_ready(ctx)

        self.assertFalse(ctx.session_sent)
        ctx.send_plugin.assert_not_awaited()

    async def test_plugin_inventory_sync_is_required(self) -> None:
        ctx = make_ready_context()
        ctx.inventory_synced = False

        await send_session_ready(ctx)

        self.assertFalse(ctx.session_sent)
        ctx.send_plugin.assert_not_awaited()

    async def test_plugin_slot_data_is_required(self) -> None:
        ctx = make_ready_context()
        ctx.slot_data = None

        await send_session_ready(ctx)

        self.assertFalse(ctx.session_sent)
        ctx.send_plugin.assert_not_awaited()


class TestSaveRevision(unittest.IsolatedAsyncioTestCase):
    async def test_valid_revision_is_persisted(self) -> None:
        ctx = make_revision_context()

        await process_plugin_message(
            ctx,
            {
                "type": "save_revision",
                "checksum": 0x12345678,
                "next_index": 3,
            },
        )

        ctx.persist_revision.assert_called_once_with(
            0x12345678,
            3,
        )
        ctx.send_plugin.assert_awaited_once_with(
            {
                "type": "save_revision_ack",
                "checksum": 0x12345678,
                "next_index": 3,
                "accepted": True,
            }
        )

    async def test_cursor_beyond_inventory_is_rejected(self) -> None:
        ctx = make_revision_context(item_count=5)

        await process_plugin_message(
            ctx,
            {
                "type": "save_revision",
                "checksum": 1,
                "next_index": 6,
            },
        )

        ctx.persist_revision.assert_not_called()

        acknowledgement = ctx.send_plugin.await_args.args[0]
        self.assertFalse(acknowledgement["accepted"])

    async def test_conflicting_cursor_is_rejected(self) -> None:
        ctx = make_revision_context()
        ctx.stored_revision_cursor.return_value = 2

        await process_plugin_message(
            ctx,
            {
                "type": "save_revision",
                "checksum": 1,
                "next_index": 3,
            },
        )

        ctx.persist_revision.assert_not_called()

        acknowledgement = ctx.send_plugin.await_args.args[0]
        self.assertFalse(acknowledgement["accepted"])
