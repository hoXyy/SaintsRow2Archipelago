import asyncio
import contextlib
import json
import unittest
from types import SimpleNamespace
from unittest.mock import AsyncMock, Mock

from ..activities import ACTIVITY_LEVEL_IDS
from ..client.bridge import (
    LIST_LOCATION_IDS,
    PROTOCOL_VERSION,
    SLOT_DATA_PROTOCOL_VERSION,
    PluginBridge,
)
from ..client.revisions import RevisionStore
from ..collectibles import (
    CD_IDS,
    CD_MAPPING,
    PROGRESSIVE_CD_IDS,
    PROGRESSIVE_CD_LOCATIONS,
)
from ..missions import get_mission_by_key


def make_bridge(item_names: list[str] | None = None, **ctx_fields) -> PluginBridge:
    """A bridge whose plugin is connected and has said hello, with a fake context."""
    names = item_names or []
    revisions = Mock(spec=RevisionStore)
    revisions.active = True
    revisions.remote_loaded = True
    revisions.local_cursor.return_value = None
    revisions.remote_cursor.return_value = None
    ctx = SimpleNamespace(
        **{
            "seed_name": "Test Seed",
            "team": 0,
            "slot": 1,
            "game": "Saints Row 2",
            "inventory_synced": True,
            "slot_data": {
                "protocol": SLOT_DATA_PROTOCOL_VERSION,
                "managed_unlockables": ["Vehicle: Taxi", "Vehicle: Taxi"],
                "managed_cheats": ["$1,000", "$1,000"],
                "persistent_items": ["persistent"],
                "features": {"exclusive_respect": True},
                "enabled_progression": {"missions": True},
            },
            "items_received": [
                SimpleNamespace(item=index) for index in range(len(names))
            ],
            "item_names": SimpleNamespace(
                lookup_in_game=lambda item, _game: names[item]
            ),
            "revisions": revisions,
            "observed_locations": set(),
            "submit_observed_locations": AsyncMock(),
            "request_sync": Mock(),
        }
        | ctx_fields
    )
    bridge = PluginBridge(ctx, port=0)
    bridge.hello = True
    bridge.send = AsyncMock(return_value=True)
    return bridge


def game_context(**fields) -> dict:
    return {
        "type": "game_context",
        "checksum": 42,
        "next_index": 0,
        "needs_cursor": True,
        "snapshot_known": False,
        "persistent_items": [],
    } | fields


class TestHello(unittest.IsolatedAsyncioTestCase):
    async def test_matching_protocol_is_accepted(self) -> None:
        bridge = make_bridge()
        bridge.hello = False

        await bridge.handle_message({"type": "hello", "protocol": PROTOCOL_VERSION})

        self.assertTrue(bridge.hello)
        bridge.ctx.request_sync.assert_called_once()

    async def test_protocol_mismatch_is_rejected(self) -> None:
        bridge = make_bridge()
        bridge.hello = False

        await bridge.handle_message({"type": "hello", "protocol": PROTOCOL_VERSION + 1})

        self.assertFalse(bridge.hello)


class TestSessionReady(unittest.IsolatedAsyncioTestCase):
    async def test_ready_context_sends_session(self) -> None:
        bridge = make_bridge()

        await bridge.send_session_ready()

        self.assertTrue(bridge.session_sent)
        message = bridge.send.await_args.args[0]
        self.assertEqual("session_ready", message["type"])
        self.assertEqual(PROTOCOL_VERSION, message["protocol"])
        self.assertEqual("Test Seed", message["seed_name"])
        self.assertEqual(0, message["team"])
        self.assertEqual(1, message["slot"])

    async def test_session_deduplicates_managed_names(self) -> None:
        bridge = make_bridge()
        bridge.ctx.slot_data["managed_unlockables"] = ["B", "A", "B"]
        bridge.ctx.slot_data["managed_cheats"] = ["D", "C", "D"]
        bridge.ctx.slot_data["persistent_items"] = ["F", "E", "F"]

        await bridge.send_session_ready()

        message = bridge.send.await_args.args[0]
        self.assertEqual(["A", "B"], message["managed_unlockables"])
        self.assertEqual(["C", "D"], message["managed_cheats"])
        self.assertEqual(["E", "F"], message["persistent_items"])

    async def assert_not_sent(self, bridge: PluginBridge) -> None:
        await bridge.send_session_ready()

        self.assertFalse(bridge.session_sent)
        bridge.send.assert_not_awaited()

    async def test_missing_persistent_items_is_not_sent(self) -> None:
        bridge = make_bridge()
        del bridge.ctx.slot_data["persistent_items"]
        await self.assert_not_sent(bridge)

    async def test_wrong_protocol_is_not_sent(self) -> None:
        bridge = make_bridge()
        bridge.ctx.slot_data["protocol"] = SLOT_DATA_PROTOCOL_VERSION + 1
        await self.assert_not_sent(bridge)

    async def test_plugin_hello_is_required(self) -> None:
        bridge = make_bridge()
        bridge.hello = False
        await self.assert_not_sent(bridge)

    async def test_inventory_sync_is_required(self) -> None:
        await self.assert_not_sent(make_bridge(inventory_synced=False))

    async def test_server_revisions_are_required(self) -> None:
        bridge = make_bridge()
        bridge.ctx.revisions.remote_loaded = False
        await self.assert_not_sent(bridge)

    async def test_slot_data_is_required(self) -> None:
        await self.assert_not_sent(make_bridge(slot_data=None))


class TestGameContext(unittest.IsolatedAsyncioTestCase):
    async def test_resolved_cursor_is_persisted_and_sent(self) -> None:
        bridge = make_bridge(["persistent", "ordinary"])
        bridge.ctx.revisions.remote_cursor.return_value = 2

        await bridge.handle_message(game_context())

        bridge.ctx.revisions.persist.assert_called_once_with(42, 2)
        bridge.send.assert_awaited_once_with(
            {
                "type": "save_context",
                "checksum": 42,
                "next_index": 2,
                "snapshot_known": True,
                "persistent_items": ["persistent"],
            }
        )
        self.assertIsNone(bridge.live_cursor)

    async def test_known_cursor_is_not_rewritten(self) -> None:
        bridge = make_bridge(["persistent", "ordinary"])
        bridge.ctx.revisions.local_cursor.return_value = 2

        await bridge.handle_message(
            game_context(
                next_index=2, snapshot_known=True, persistent_items=["persistent"]
            )
        )

        bridge.ctx.revisions.persist.assert_not_called()
        bridge.send.assert_awaited_once()

    async def test_rejected_save_sends_nothing(self) -> None:
        bridge = make_bridge(["persistent", "ordinary"])
        bridge.ctx.revisions.local_cursor.return_value = 1

        await bridge.handle_message(
            game_context(
                next_index=2, snapshot_known=True, persistent_items=["persistent"]
            )
        )

        bridge.send.assert_not_awaited()
        self.assertIsNone(bridge.live_cursor)

    async def test_ledger_write_failure_sends_nothing(self) -> None:
        bridge = make_bridge()
        bridge.ctx.revisions.persist.side_effect = OSError("failed")

        await bridge.handle_message(game_context())

        bridge.send.assert_not_awaited()

    async def test_context_without_ap_session_is_ignored(self) -> None:
        bridge = make_bridge()
        bridge.ctx.revisions.active = False

        await bridge.handle_message(game_context())

        bridge.ctx.revisions.local_cursor.assert_not_called()
        bridge.send.assert_not_awaited()

    async def test_settled_context_starts_delivery(self) -> None:
        bridge = make_bridge()

        await bridge.handle_message(game_context(next_index=3, needs_cursor=False))

        self.assertEqual(3, bridge.live_cursor)
        bridge.ctx.request_sync.assert_called_once()

    async def test_every_context_invalidates_pending_delivery(self) -> None:
        bridge = make_bridge()
        bridge.pending_ack = Mock()

        await bridge.handle_message(game_context(needs_cursor=False))

        self.assertEqual(1, bridge.context_generation)
        self.assertIsNone(bridge.pending_ack)


class TestSaveRevision(unittest.IsolatedAsyncioTestCase):
    async def test_valid_revision_is_persisted(self) -> None:
        bridge = make_bridge(["a", "b", "c"])

        await bridge.handle_message(
            {"type": "save_revision", "checksum": 0x12345678, "next_index": 3}
        )

        bridge.ctx.revisions.persist.assert_called_once_with(0x12345678, 3)
        bridge.send.assert_awaited_once_with(
            {
                "type": "save_revision_ack",
                "checksum": 0x12345678,
                "next_index": 3,
                "accepted": True,
            }
        )

    async def test_rejected_revision_is_not_persisted(self) -> None:
        bridge = make_bridge(["a", "b", "c"])

        await bridge.handle_message(
            {"type": "save_revision", "checksum": 1, "next_index": 4}
        )

        bridge.ctx.revisions.persist.assert_not_called()
        self.assertFalse(bridge.send.await_args.args[0]["accepted"])

    async def test_revision_without_ap_session_is_rejected(self) -> None:
        bridge = make_bridge(["a", "b", "c"])
        bridge.ctx.revisions.active = False

        await bridge.handle_message(
            {"type": "save_revision", "checksum": 1, "next_index": 3}
        )

        bridge.ctx.revisions.persist.assert_not_called()
        self.assertFalse(bridge.send.await_args.args[0]["accepted"])

    async def test_ledger_write_failure_rejects_revision(self) -> None:
        bridge = make_bridge(["a", "b", "c"])
        bridge.ctx.revisions.persist.side_effect = OSError("failed")

        await bridge.handle_message(
            {"type": "save_revision", "checksum": 1, "next_index": 3}
        )

        self.assertFalse(bridge.send.await_args.args[0]["accepted"])

    async def test_already_stored_revision_is_acked_without_persisting(self) -> None:
        bridge = make_bridge(["a", "b", "c"])
        bridge.ctx.revisions.local_cursor.return_value = 3

        await bridge.handle_message(
            {"type": "save_revision", "checksum": 1, "next_index": 3}
        )

        bridge.ctx.revisions.persist.assert_not_called()
        self.assertTrue(bridge.send.await_args.args[0]["accepted"])


class TestProgression(unittest.IsolatedAsyncioTestCase):
    async def test_progression_is_observed_and_submitted(self) -> None:
        bridge = make_bridge()
        mission = get_mission_by_key("rn01")

        await bridge.handle_message(
            {
                "type": "progression",
                "category": "mission",
                "key": mission.key,
                "previous": 0,
                "current": 1,
            }
        )

        self.assertEqual({mission.id}, bridge.ctx.observed_locations)
        bridge.ctx.submit_observed_locations.assert_awaited_once()

    async def test_collectible_observes_individual_and_progressive_locations(
        self,
    ) -> None:
        bridge = make_bridge()
        key, name = next(iter(CD_MAPPING.items()))

        await bridge.handle_message(
            {"type": "progression", "category": "cd", "key": key, "current": 1}
        )

        self.assertEqual(
            {CD_IDS[name], PROGRESSIVE_CD_IDS[PROGRESSIVE_CD_LOCATIONS[0]]},
            bridge.ctx.observed_locations,
        )

    async def test_session_end_clears_collectible_counts(self) -> None:
        bridge = make_bridge()
        first, second = list(CD_MAPPING)[:2]

        await bridge.handle_message(
            {"type": "progression", "category": "cd", "key": first, "current": 1}
        )
        await bridge.end_session()
        bridge.ctx.observed_locations.clear()
        await bridge.handle_message(
            {"type": "progression", "category": "cd", "key": second, "current": 1}
        )

        self.assertNotIn(
            PROGRESSIVE_CD_IDS[PROGRESSIVE_CD_LOCATIONS[1]],
            bridge.ctx.observed_locations,
        )


class TestDeath(unittest.IsolatedAsyncioTestCase):
    async def test_plugin_death_is_sent_as_death_link(self) -> None:
        bridge = make_bridge(player_names={1: "Player"}, send_death=AsyncMock())

        await bridge.handle_message({"type": "death"})

        bridge.ctx.send_death.assert_awaited_once()
        cause = bridge.ctx.send_death.await_args.args[0]
        self.assertTrue(cause.startswith("Player "))


class TestListOpened(unittest.IsolatedAsyncioTestCase):
    def test_lists_map_to_their_locations(self) -> None:
        self.assertEqual([1, 2, 3, 4, 5], sorted(LIST_LOCATION_IDS["chop_shop"]))
        self.assertEqual([1, 2, 3, 4, 5], sorted(LIST_LOCATION_IDS["hitman"]))
        self.assertTrue(
            all(len(ids) == 8 for ids in LIST_LOCATION_IDS["chop_shop"].values())
        )
        self.assertTrue(
            all(len(ids) == 6 for ids in LIST_LOCATION_IDS["hitman"].values())
        )
        self.assertIn(
            ACTIVITY_LEVEL_IDS["Chop Shop (Suburbs) - Mockingbird"],
            LIST_LOCATION_IDS["chop_shop"][1],
        )
        self.assertIn(
            ACTIVITY_LEVEL_IDS["Hitman (Saint's Row) - Russell"],
            LIST_LOCATION_IDS["hitman"][5],
        )

    async def test_missing_locations_of_opened_list_are_hinted(self) -> None:
        checked, *missing = LIST_LOCATION_IDS["hitman"][2]
        bridge = make_bridge(missing_locations=set(missing), send_hints=AsyncMock())

        await bridge.handle_message(
            {"type": "list_opened", "category": "hitman", "list": 2}
        )

        bridge.ctx.send_hints.assert_awaited_once_with(missing)
        self.assertNotIn(checked, bridge.ctx.send_hints.await_args.args[0])

    async def test_unknown_or_fully_checked_lists_are_not_hinted(self) -> None:
        bridge = make_bridge(missing_locations=set(), send_hints=AsyncMock())

        for message in (
            {"category": "chop_shop", "list": 1},
            {"category": "chop_shop", "list": 6},
            {"category": "mission", "list": 1},
        ):
            await bridge.handle_message({"type": "list_opened"} | message)

        bridge.ctx.send_hints.assert_not_awaited()


class TestPluginConnection(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self) -> None:
        self.bridge = make_bridge()
        self.bridge.hello = False
        await self.bridge.start()
        self.addAsyncCleanup(self.bridge.stop)
        self.port = self.bridge.server.sockets[0].getsockname()[1]

    async def connect(self) -> tuple[asyncio.StreamReader, asyncio.StreamWriter]:
        reader, writer = await asyncio.open_connection("127.0.0.1", self.port)
        self.addAsyncCleanup(self.close, writer)
        return reader, writer

    async def close(self, writer: asyncio.StreamWriter) -> None:
        writer.close()
        with contextlib.suppress(ConnectionError):
            await writer.wait_closed()

    async def wait_until(self, condition) -> None:
        async with asyncio.timeout(2):
            while not condition():
                await asyncio.sleep(0.01)

    async def send_line(self, writer: asyncio.StreamWriter, line: bytes) -> None:
        writer.write(line + b"\n")
        await writer.drain()

    async def say_hello(self, writer: asyncio.StreamWriter) -> None:
        hello = {"type": "hello", "protocol": PROTOCOL_VERSION}
        await self.send_line(writer, json.dumps(hello).encode())
        await self.wait_until(lambda: self.bridge.hello)

    async def test_second_plugin_is_refused(self) -> None:
        _, first = await self.connect()
        await self.wait_until(lambda: self.bridge.connected)
        await self.say_hello(first)

        second_reader, _ = await self.connect()

        self.assertEqual(b"", await asyncio.wait_for(second_reader.read(), 2))
        self.assertTrue(self.bridge.connected)
        self.assertTrue(self.bridge.hello)

    async def test_malformed_lines_are_skipped(self) -> None:
        _, writer = await self.connect()

        for line in (b"not json", b"[1, 2]", b'{"type": "game_context"}'):
            await self.send_line(writer, line)
        await self.say_hello(writer)

        self.assertTrue(self.bridge.connected)

    async def test_disconnect_resets_the_connection_state(self) -> None:
        _, writer = await self.connect()
        await self.say_hello(writer)
        self.bridge.session_sent = True
        self.bridge.live_cursor = 3

        await self.close(writer)
        await self.wait_until(lambda: not self.bridge.connected)

        self.assertFalse(self.bridge.hello)
        self.assertFalse(self.bridge.session_sent)
        self.assertIsNone(self.bridge.live_cursor)
        self.assertIsNone(self.bridge.pending_ack)
