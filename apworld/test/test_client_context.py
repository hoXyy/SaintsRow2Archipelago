import unittest
from unittest.mock import AsyncMock

from NetUtils import ClientStatus
from ..client.context import SR2Context


def make_context(**fields) -> SR2Context:
    # Skip CommonContext.__init__, which starts real client machinery.
    ctx = object.__new__(SR2Context)
    ctx.__dict__.update(
        {
            "observed_locations": set(),
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
            slot_data={"goal_locations": [16, 31]}, locations_checked={16, 31}
        )

        await ctx.send_goal_if_complete()

        self.assertTrue(ctx.finished_game)
        ctx.send_msgs.assert_awaited_once_with(self.GOAL_MESSAGE)

    async def test_partial_goal_does_not_finish(self) -> None:
        ctx = make_context(
            slot_data={"goal_locations": [16, 31]}, locations_checked={16}
        )

        await ctx.send_goal_if_complete()

        self.assertFalse(ctx.finished_game)
        ctx.send_msgs.assert_not_awaited()

    async def test_goal_is_not_sent_twice(self) -> None:
        ctx = make_context(
            finished_game=True,
            slot_data={"goal_locations": [16]},
            locations_checked={16},
        )

        await ctx.send_goal_if_complete()

        ctx.send_msgs.assert_not_awaited()

    async def test_goal_uses_server_and_locally_checked_locations(self) -> None:
        ctx = make_context(
            slot_data={"goal_locations": [16, 31, 46, 54]},
            checked_locations={16, 31, 46, 100, 101},
            locations_checked={54, 102},
        )

        await ctx.send_goal_if_complete()

        self.assertTrue(ctx.finished_game)
        ctx.send_msgs.assert_awaited_once_with(self.GOAL_MESSAGE)

    async def test_goal_sent_when_every_goal_was_already_server_checked(self) -> None:
        ctx = make_context(
            slot_data={"goal_locations": [16, 31, 46, 54]},
            checked_locations={16, 31, 46, 54},
        )

        await ctx.send_goal_if_complete()

        self.assertTrue(ctx.finished_game)
        ctx.send_msgs.assert_awaited_once()

    async def test_submitting_the_last_goal_location_finishes(self) -> None:
        ctx = make_context(
            slot_data={"goal_locations": [16]},
            missing_locations={16},
            observed_locations={16},
        )

        await ctx.submit_observed_locations()

        self.assertTrue(ctx.finished_game)
