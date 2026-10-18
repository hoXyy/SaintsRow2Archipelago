import unittest

from ..client.save_sync import (
    Inventory,
    SaveContext,
    accept_save_revision,
    resolve_save_context,
)

INVENTORY = Inventory(["persistent", "ordinary"], frozenset({"persistent"}))


def resolve(
    plugin_cursor: int | None = None,
    plugin_items: list[str] | None = None,
    local_cursor: int | None = None,
    remote_cursor: int | None = None,
    inventory: Inventory = INVENTORY,
) -> SaveContext | None:
    return resolve_save_context(
        42, plugin_cursor, plugin_items or [], local_cursor, remote_cursor, inventory
    )


class TestResolveSaveContext(unittest.TestCase):
    def test_plugin_snapshot_repairs_missing_client_revision(self) -> None:
        self.assertEqual(
            SaveContext(2, ["persistent"], persist=True),
            resolve(plugin_cursor=2, plugin_items=["persistent"]),
        )

    def test_client_revision_repairs_missing_plugin_snapshot(self) -> None:
        self.assertEqual(
            SaveContext(2, ["persistent"], persist=False), resolve(local_cursor=2)
        )

    def test_matching_snapshots_continue_without_rewriting_client_ledger(self) -> None:
        self.assertEqual(
            SaveContext(2, ["persistent"], persist=False),
            resolve(plugin_cursor=2, plugin_items=["persistent"], local_cursor=2),
        )

    def test_missing_snapshots_initialize_an_empty_cursor(self) -> None:
        self.assertEqual(SaveContext(0, [], persist=True), resolve())

    def test_conflicting_cursors_are_rejected(self) -> None:
        self.assertIsNone(
            resolve(plugin_cursor=2, plugin_items=["persistent"], local_cursor=1)
        )

    def test_plugin_items_must_match_the_inventory(self) -> None:
        self.assertIsNone(resolve(plugin_cursor=2, plugin_items=[]))

    def test_plugin_cursor_beyond_inventory_is_rejected(self) -> None:
        self.assertIsNone(resolve(plugin_cursor=3))

    def test_client_cursor_beyond_inventory_is_rejected(self) -> None:
        self.assertIsNone(resolve(local_cursor=3))

    def test_unknown_snapshot_with_items_is_rejected(self) -> None:
        self.assertIsNone(resolve(plugin_items=["persistent"]))

    def test_server_revision_recovers_save_unknown_to_both_local_copies(self) -> None:
        self.assertEqual(
            SaveContext(2, ["persistent"], persist=True, from_server=True),
            resolve(remote_cursor=2),
        )

    def test_out_of_range_server_revision_is_ignored(self) -> None:
        self.assertEqual(SaveContext(0, [], persist=True), resolve(remote_cursor=5))

    def test_local_revision_wins_over_server_revision(self) -> None:
        self.assertEqual(
            SaveContext(2, ["persistent"], persist=False),
            resolve(local_cursor=2, remote_cursor=1),
        )


class TestInventory(unittest.TestCase):
    def test_persistent_before_is_sorted_and_deduplicated(self) -> None:
        inventory = Inventory(["b", "x", "a", "b", "c"], frozenset({"a", "b", "c"}))

        self.assertEqual(["a", "b"], inventory.persistent_before(4))


class TestAcceptSaveRevision(unittest.TestCase):
    def test_valid_revision_is_accepted(self) -> None:
        self.assertTrue(accept_save_revision(0x12345678, 3, None, item_count=5))

    def test_matching_stored_revision_is_accepted(self) -> None:
        self.assertTrue(accept_save_revision(1, 3, 3, item_count=5))

    def test_cursor_beyond_inventory_is_rejected(self) -> None:
        self.assertFalse(accept_save_revision(1, 6, None, item_count=5))

    def test_checksum_out_of_range_is_rejected(self) -> None:
        self.assertFalse(accept_save_revision(0x1_0000_0000, 3, None, item_count=5))

    def test_conflicting_cursor_is_rejected(self) -> None:
        self.assertFalse(accept_save_revision(1, 3, 2, item_count=5))
