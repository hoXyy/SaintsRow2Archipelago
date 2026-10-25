import asyncio
import random
import unittest
from types import SimpleNamespace
from typing import Any
from unittest.mock import patch

from ..activities import RACES
from ..client import bridge as bridge_module
from ..client.bridge import PluginBridge
from ..client.progression import (
    ACTIVITY_LOCATIONS,
    SINGLE_LOCATIONS,
    locations_for_progress,
)
from ..client.save_sync import Inventory, accept_save_revision, resolve_save_context
from ..collectibles import STYLE_LEVEL_IDS, STYLE_LEVEL_LOCATIONS
from ..locations import LOCATION_NAME_TO_ID
from .fuzz import fuzz_cases
from .test_client_bridge import game_context, make_bridge

SLOT = 1
PLAYER_NAMES = {1: "Self", 2: "Other", 3: "Third"}


class FakePlugin:
    """Answers item messages like the game plugin, with random faults.

    The plugin applies only the item at its own cursor, acks duplicates as
    accepted and defers anything ahead of its cursor.
    """

    def __init__(
        self, bridge: PluginBridge, rng: random.Random, names: list[str], cursor: int
    ) -> None:
        self.bridge = bridge
        self.rng = rng
        self.names = names
        self.cursor = cursor
        self.fault_rate = rng.choice((0.0, 0.2, 0.5))
        self.reloads_left = rng.randint(0, 3)
        self.items_left_to_receive = rng.randint(0, 3)
        self.applied: list[tuple[int, str]] = []
        self.messages: list[dict[str, Any]] = []
        self.sent_ahead: list[int] = []

    def ack(self, index: int, accepted: bool) -> None:
        pending = self.bridge.pending_ack
        if pending is not None and not pending.done():
            pending.set_result((index, accepted))

    def receive_new_item(self) -> None:
        ctx = self.bridge.ctx
        index = len(ctx.items_received)
        self.names.append(f"Item {index}")
        ctx.items_received.append(random_item(self.rng, index))

    async def send(self, message: dict[str, Any]) -> bool:
        if message["type"] != "item":
            return True
        self.messages.append(message)
        index = message["index"]
        if index > self.cursor:
            self.sent_ahead.append(index)

        if self.items_left_to_receive and self.rng.random() < 0.2:
            self.items_left_to_receive -= 1
            self.receive_new_item()

        if self.rng.random() < self.fault_rate:
            fault = self.rng.choice(
                ("send_fails", "defer", "drop", "lost_ack", "wrong_index", "reload")
            )
            if fault == "send_fails":
                return False
            if fault == "defer":
                self.ack(index, False)
            elif fault == "lost_ack" and index == self.cursor:
                self.apply(message)
            elif fault == "wrong_index":
                self.ack(index + self.rng.choice((-1, 1, 5)), True)
            elif fault == "reload" and self.reloads_left:
                # Load an older save; the plugin reports its cursor again.
                self.reloads_left -= 1
                self.cursor = self.rng.randint(0, self.cursor)
                await self.bridge.handle_message(
                    game_context(next_index=self.cursor, needs_cursor=False)
                )
            return True

        if index < self.cursor:
            self.ack(index, True)
        elif index == self.cursor:
            self.apply(message)
            self.ack(index, True)
        else:
            self.ack(index, False)
        return True

    def apply(self, message: dict[str, Any]) -> None:
        self.applied.append((message["index"], message["name"]))
        self.cursor += 1


def random_item(rng: random.Random, index: int) -> SimpleNamespace:
    return SimpleNamespace(
        item=index, player=rng.choice(list(PLAYER_NAMES)), flags=rng.choice((0, 1, 2))
    )


class TestItemDeliveryFuzz(unittest.IsolatedAsyncioTestCase):
    """However the plugin misbehaves, every item ends up applied exactly once
    per save, in order, and the bridge never sends past the plugin's cursor."""

    async def test_random_plugin_faults(self) -> None:
        with (
            patch.object(bridge_module, "ACK_TIMEOUT_SECONDS", 0.001),
            patch.object(bridge_module, "RETRY_SECONDS", 0),
        ):
            for case, rng in fuzz_cases("delivery"):
                with self.subTest(case=case):
                    await self.run_case(rng)

    async def run_case(self, rng: random.Random) -> None:
        item_count = rng.randint(0, 12)
        names = [f"Item {index}" for index in range(item_count)]
        bridge = make_bridge(
            names,
            slot=SLOT,
            player_names=PLAYER_NAMES,
            items_received=[random_item(rng, index) for index in range(item_count)],
        )
        start = rng.randint(0, item_count)
        plugin = FakePlugin(bridge, rng, names, start)
        bridge.send = plugin.send
        bridge.session_sent = True
        bridge.live_cursor = start

        # Like the sync loop, retry until delivery catches up.
        for _ in range(200):
            await asyncio.wait_for(bridge.deliver_items(), timeout=10)
            if bridge.live_cursor == len(bridge.ctx.items_received):
                break

        items = bridge.ctx.items_received
        self.assertEqual(len(items), bridge.live_cursor)
        self.assertEqual(len(items), plugin.cursor)
        self.assertEqual([], plugin.sent_ahead)
        for index, name in plugin.applied:
            self.assertEqual(names[index], name)
        for message in plugin.messages:
            item = items[message["index"]]
            self.assertEqual(names[message["index"]], message["name"])
            self.assertEqual(item.flags, message["flags"])
            if item.player == SLOT:
                self.assertNotIn("sender", message)
            else:
                self.assertEqual(PLAYER_NAMES[item.player], message["sender"])


PERSISTENT = ("Persistent A", "Persistent B")
ITEM_NAMES = (*PERSISTENT, "Ordinary A", "Ordinary B")


def random_cursor(rng: random.Random, item_count: int) -> int | None:
    if rng.random() < 0.3:
        return None
    return rng.randint(-1, item_count + 2)


class TestSaveSyncFuzz(unittest.TestCase):
    def test_resolve_save_context(self) -> None:
        for case, rng in fuzz_cases("resolve_save_context"):
            inventory = Inventory(
                [rng.choice(ITEM_NAMES) for _ in range(rng.randint(0, 8))],
                frozenset(PERSISTENT),
            )
            plugin_cursor = random_cursor(rng, len(inventory.names))
            if (
                plugin_cursor is not None
                and inventory.contains_cursor(plugin_cursor)
                and rng.random() < 0.6
            ):
                plugin_items = inventory.persistent_before(plugin_cursor)
                rng.shuffle(plugin_items)
            else:
                plugin_items = rng.sample(PERSISTENT, rng.randint(0, 2))
            local_cursor = random_cursor(rng, len(inventory.names))
            remote_cursor = random_cursor(rng, len(inventory.names))

            with self.subTest(
                case=case,
                inventory=inventory.names,
                plugin_cursor=plugin_cursor,
                plugin_items=plugin_items,
                local_cursor=local_cursor,
                remote_cursor=remote_cursor,
            ):
                self.check_resolve(
                    inventory, plugin_cursor, plugin_items, local_cursor, remote_cursor
                )

    def check_resolve(
        self,
        inventory: Inventory,
        plugin_cursor: int | None,
        plugin_items: list[str],
        local_cursor: int | None,
        remote_cursor: int | None,
    ) -> None:
        result = resolve_save_context(
            42, plugin_cursor, plugin_items, local_cursor, remote_cursor, inventory
        )

        known_cursors = [c for c in (plugin_cursor, local_cursor) if c is not None]
        plugin_items_match = (
            not plugin_items
            if plugin_cursor is None
            else inventory.contains_cursor(plugin_cursor)
            and sorted(plugin_items) == inventory.persistent_before(plugin_cursor)
        )
        consistent = (
            plugin_items_match
            and all(inventory.contains_cursor(c) for c in known_cursors)
            and len(set(known_cursors)) <= 1
        )
        self.assertEqual(consistent, result is not None)
        if result is None:
            return

        self.assertTrue(inventory.contains_cursor(result.next_index))
        self.assertEqual(
            inventory.persistent_before(result.next_index), result.persistent_items
        )
        self.assertEqual(local_cursor is None, result.persist)
        if known_cursors:
            # A cursor either local copy knows always beats the server's.
            self.assertEqual(known_cursors[0], result.next_index)
            self.assertFalse(result.from_server)
        elif result.from_server:
            self.assertEqual(remote_cursor, result.next_index)
        else:
            self.assertEqual(0, result.next_index)
            self.assertTrue(
                remote_cursor is None or not inventory.contains_cursor(remote_cursor)
            )

    def test_accept_save_revision(self) -> None:
        for case, rng in fuzz_cases("accept_save_revision"):
            item_count = rng.randint(0, 8)
            checksum = rng.choice((-1, 0, rng.getrandbits(32), 0xFFFFFFFF, 1 << 32))
            next_index = rng.randint(-2, item_count + 2)
            local_cursor = random_cursor(rng, item_count)

            with self.subTest(
                case=case,
                checksum=checksum,
                next_index=next_index,
                local_cursor=local_cursor,
                item_count=item_count,
            ):
                expected = (
                    0 <= checksum <= 0xFFFFFFFF
                    and 0 <= next_index <= item_count
                    and local_cursor in (None, next_index)
                )
                self.assertEqual(
                    expected,
                    accept_save_revision(
                        checksum, next_index, local_cursor, item_count
                    ),
                )


ALL_LOCATION_IDS = set(LOCATION_NAME_TO_ID.values())
CATEGORY_KEYS: dict[str | None, list[str | None]] = {
    **{category: list(keys) for category, keys in SINGLE_LOCATIONS.items()},
    "activity": list(ACTIVITY_LOCATIONS),
    "racing": list(RACES),
    "style_level": [None],
    "unknown": ["rn01"],
    None: [None],
}
VALUE_RANGES = {"activity": 0xFF, "racing": 4, "style_level": 12}


class TestProgressionFuzz(unittest.TestCase):
    def test_random_progression_events(self) -> None:
        for case, rng in fuzz_cases("progression"):
            category = rng.choice(list(CATEGORY_KEYS))
            key = rng.choice([*CATEGORY_KEYS[category], "unknown"])
            high = VALUE_RANGES.get(category, 2)
            previous, current = rng.randint(0, high), rng.randint(0, high)

            with self.subTest(
                case=case,
                category=category,
                key=key,
                previous=previous,
                current=current,
            ):
                self.check_progress(category, key, previous, current)

    def check_progress(
        self, category: str | None, key: str | None, previous: int, current: int
    ) -> None:
        locations = locations_for_progress(category, key, previous, current)

        self.assertLessEqual(set(locations), ALL_LOCATION_IDS)
        self.assertEqual(len(locations), len(set(locations)))
        unknown_key = key == "unknown" and category != "style_level"
        if current <= previous or unknown_key or category in ("unknown", None):
            self.assertEqual([], locations)
            return

        if category in SINGLE_LOCATIONS:
            self.assertEqual([SINGLE_LOCATIONS[category][key]], locations)
        elif category == "activity":
            activity, district = ACTIVITY_LOCATIONS[key]
            newly_completed = current & ~previous & 0b111111
            self.assertEqual(newly_completed.bit_count(), len(locations))
            instance_ids = {
                LOCATION_NAME_TO_ID[f"{activity} ({district}) - Level {level}"]
                for level in range(1, 7)
            }
            self.assertLessEqual(set(locations), instance_ids)
        elif category == "racing":
            self.assertEqual(min(current, 3), len(locations))
            # A better medal still includes every lower one.
            lower = locations_for_progress(category, key, 0, current - 1)
            self.assertLessEqual(set(lower), set(locations))
        elif category == "style_level":
            self.assertEqual(
                [STYLE_LEVEL_IDS[name] for name in STYLE_LEVEL_LOCATIONS[:current]],
                locations,
            )

    def test_activity_progress_splits_into_steps(self) -> None:
        # Reporting a jump at once checks the same levels as reporting it in
        # two steps.
        for case, rng in fuzz_cases("activity_steps"):
            key = rng.choice(list(ACTIVITY_LOCATIONS))
            first = rng.getrandbits(6)
            middle = first | rng.getrandbits(6)
            last = middle | rng.getrandbits(6)

            with self.subTest(case=case, key=key, values=(first, middle, last)):
                at_once = locations_for_progress("activity", key, first, last)
                in_steps = locations_for_progress(
                    "activity", key, first, middle
                ) + locations_for_progress("activity", key, middle, last)

                self.assertEqual(sorted(at_once), sorted(in_steps))
