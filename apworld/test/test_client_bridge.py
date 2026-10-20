import unittest
from types import SimpleNamespace
from unittest.mock import AsyncMock, Mock

from ..client.bridge import PROTOCOL_VERSION, SLOT_DATA_PROTOCOL_VERSION, PluginBridge
from ..client.revisions import RevisionStore
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


class TestDeath(unittest.IsolatedAsyncioTestCase):
    async def test_plugin_death_is_sent_as_death_link(self) -> None:
        bridge = make_bridge(player_names={1: "Player"}, send_death=AsyncMock())

        await bridge.handle_message({"type": "death"})

        bridge.ctx.send_death.assert_awaited_once()
        cause = bridge.ctx.send_death.await_args.args[0]
        self.assertTrue(cause.startswith("Player "))
