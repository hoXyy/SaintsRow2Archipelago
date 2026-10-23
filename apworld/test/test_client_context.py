import asyncio
import unittest
from types import SimpleNamespace
from unittest.mock import AsyncMock, Mock

from NetUtils import ClientStatus

from ..client.context import SR2Context


def make_context(**fields) -> SR2Context:
    # Skip CommonContext.__init__, which starts real client machinery.
    ctx = object.__new__(SR2Context)
    ctx.__dict__.update(
        {
            "observed_locations": set(),
            "session_identity": None,
            "missing_locations": set(),
            "checked_locations": set(),
            "locations_checked": set(),
            "slot_data": None,
            "finished_game": False,
            "send_msgs": AsyncMock(),
        }
        | fields
    )
    return ctx


class TestSubmitObservedLocations(unittest.IsolatedAsyncioTestCase):
    async def test_observed_missing_location_is_submitted(self) -> None:
        ctx = make_context(missing_locations={10}, observed_locations={10})

        await ctx.submit_observed_locations()

        self.assertEqual({10}, ctx.locations_checked)
        ctx.send_msgs.assert_awaited_once_with(
            [{"cmd": "LocationChecks", "locations": [10]}]
        )

    async def test_checked_location_is_not_resubmitted(self) -> None:
        ctx = make_context(
            missing_locations={10}, observed_locations={10}, locations_checked={10}
        )

        await ctx.submit_observed_locations()

        ctx.send_msgs.assert_not_awaited()

    async def test_location_not_in_missing_locations_is_not_submitted(self) -> None:
        # An empty missing_locations means this check is not part of this seed,
        # or the server no longer considers it missing.
        ctx = make_context(observed_locations={10})

        await ctx.submit_observed_locations()

        self.assertNotIn(10, ctx.locations_checked)
        ctx.send_msgs.assert_not_awaited()

    async def test_new_locations_are_submitted_in_sorted_order(self) -> None:
        ctx = make_context(
            missing_locations={10, 20, 30}, observed_locations={30, 10, 20}
        )

        await ctx.submit_observed_locations()

        ctx.send_msgs.assert_awaited_once_with(
            [{"cmd": "LocationChecks", "locations": [10, 20, 30]}]
        )


class TestGoal(unittest.IsolatedAsyncioTestCase):
    GOAL_MESSAGE = [{"cmd": "StatusUpdate", "status": ClientStatus.CLIENT_GOAL}]

    async def test_goal_sent_after_every_goal_location(self) -> None:
        ctx = make_context(
            slot_data={"goal_locations": [16, 31]}, observed_locations={16, 31}
        )

        await ctx.send_goal_if_complete()

        self.assertTrue(ctx.finished_game)
        ctx.send_msgs.assert_awaited_once_with(self.GOAL_MESSAGE)

    async def test_partial_goal_does_not_finish(self) -> None:
        ctx = make_context(
            slot_data={"goal_locations": [16, 31]}, observed_locations={16}
        )

        await ctx.send_goal_if_complete()

        self.assertFalse(ctx.finished_game)
        ctx.send_msgs.assert_not_awaited()

    async def test_goal_is_not_sent_twice(self) -> None:
        ctx = make_context(
            finished_game=True,
            slot_data={"goal_locations": [16]},
            observed_locations={16},
        )

        await ctx.send_goal_if_complete()

        ctx.send_msgs.assert_not_awaited()

    async def test_server_checked_goal_locations_do_not_finish(self) -> None:
        # Another player's collect, or a release of this slot, marks goal
        # locations checked on the server without the player finishing them.
        ctx = make_context(
            slot_data={"goal_locations": [16, 31, 46, 54]},
            checked_locations={16, 31, 46, 54},
            locations_checked={16, 31, 46, 54},
        )

        await ctx.send_goal_if_complete()

        self.assertFalse(ctx.finished_game)
        ctx.send_msgs.assert_not_awaited()

    async def test_goal_observed_after_server_check_finishes(self) -> None:
        # The location is no longer missing, so it is never submitted, but the
        # game reporting it still counts towards the goal.
        ctx = make_context(
            slot_data={"goal_locations": [16]},
            checked_locations={16},
            observed_locations={16},
        )

        await ctx.submit_observed_locations()

        self.assertTrue(ctx.finished_game)
        ctx.send_msgs.assert_awaited_once_with(self.GOAL_MESSAGE)

    async def test_submitting_the_last_goal_location_finishes(self) -> None:
        ctx = make_context(
            slot_data={"goal_locations": [16]},
            missing_locations={16},
            observed_locations={16},
        )

        await ctx.submit_observed_locations()

        self.assertTrue(ctx.finished_game)


class TestSessionSwitch(unittest.IsolatedAsyncioTestCase):
    async def connect(self, ctx: SR2Context, seed_name: str, slot: int) -> None:
        ctx.seed_name = seed_name
        ctx.slot = slot
        ctx.on_package("Connected", {"slot_data": None})
        await asyncio.sleep(0)

    async def make_connected_context(self) -> SR2Context:
        ctx = make_context(
            team=0,
            revisions=Mock(),
            request_sync=Mock(),
            update_death_link=AsyncMock(),
        )
        await self.connect(ctx, "Seed A", 1)
        ctx.observed_locations.update({16, 31})
        return ctx

    async def test_same_slot_reconnect_keeps_observed_locations(self) -> None:
        ctx = await self.make_connected_context()

        await self.connect(ctx, "Seed A", 1)

        self.assertEqual({16, 31}, ctx.observed_locations)

    async def test_switching_slot_clears_observed_locations(self) -> None:
        # Location ids are shared between seeds, so another slot's progress
        # must not be submitted to the new one.
        ctx = await self.make_connected_context()

        await self.connect(ctx, "Seed A", 2)

        self.assertEqual(set(), ctx.observed_locations)

    async def test_switching_seed_clears_observed_locations(self) -> None:
        ctx = await self.make_connected_context()

        await self.connect(ctx, "Seed B", 1)

        self.assertEqual(set(), ctx.observed_locations)

    async def close_connection(self, ctx: SR2Context) -> None:
        ctx.bridge = SimpleNamespace(end_session=AsyncMock())
        ctx.server = None
        await ctx.connection_closed()

    async def test_disconnect_clears_state_replayed_on_connect(self) -> None:
        # AP before 0.6.8 replays these on the next Connected, even to
        # another slot.
        ctx = await self.make_connected_context()
        ctx.locations_checked = {16, 31}
        ctx.missing_locations = {46}
        ctx.finished_game = True

        await self.close_connection(ctx)

        self.assertEqual(set(), ctx.locations_checked)
        self.assertEqual(set(), ctx.missing_locations)
        self.assertFalse(ctx.finished_game)
        self.assertEqual({16, 31}, ctx.observed_locations)

    async def test_offline_submit_does_not_refill_locations_checked(self) -> None:
        ctx = await self.make_connected_context()
        ctx.missing_locations = {16, 31}

        await self.close_connection(ctx)
        await ctx.submit_observed_locations()

        self.assertEqual(set(), ctx.locations_checked)


class TestDeathLink(unittest.IsolatedAsyncioTestCase):
    async def test_received_death_is_forwarded_to_the_plugin(self) -> None:
        ctx = make_context(
            last_death_link=0.0, bridge=SimpleNamespace(send=AsyncMock())
        )

        ctx.on_deathlink(
            {"time": 5.0, "source": "Other", "cause": "Other fell into lava"}
        )
        await asyncio.sleep(0)

        self.assertEqual(5.0, ctx.last_death_link)
        ctx.bridge.send.assert_awaited_once_with(
            {
                "type": "death_link",
                "source": "Other",
                "cause": "Other fell into lava",
            }
        )

    async def test_received_death_without_cause_forwards_empty_cause(self) -> None:
        ctx = make_context(
            last_death_link=0.0, bridge=SimpleNamespace(send=AsyncMock())
        )

        ctx.on_deathlink({"time": 5.0, "source": "Other"})
        await asyncio.sleep(0)

        message = ctx.bridge.send.await_args.args[0]
        self.assertEqual("Other", message["source"])
        self.assertEqual("", message["cause"])

    async def connect(self, slot_data: dict | None) -> SR2Context:
        ctx = make_context(
            seed_name="Test Seed",
            team=0,
            slot=1,
            revisions=Mock(),
            request_sync=Mock(),
            update_death_link=AsyncMock(),
        )

        ctx.on_package("Connected", {"slot_data": slot_data})
        await asyncio.sleep(0)
        return ctx

    async def test_connecting_with_death_link_enables_the_tag(self) -> None:
        ctx = await self.connect({"features": {"death_link": True}})

        ctx.update_death_link.assert_awaited_once_with(True)

    async def test_connecting_without_death_link_disables_the_tag(self) -> None:
        ctx = await self.connect({"features": {"death_link": False}})

        ctx.update_death_link.assert_awaited_once_with(False)

    async def test_connecting_without_slot_data_disables_the_tag(self) -> None:
        ctx = await self.connect(None)

        ctx.update_death_link.assert_awaited_once_with(False)
