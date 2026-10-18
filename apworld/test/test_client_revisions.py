import os
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import Mock, patch

from ..client.revisions import RevisionStore

REMOTE_KEY = "saints_row_2_revisions_v1_0_1"


class RevisionStoreTestCase(unittest.TestCase):
    def setUp(self) -> None:
        directory = TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.path = Path(directory.name) / "ledger.json"
        self.send = Mock(return_value=True)

    def make_store(self, retrieved: dict | None = None) -> RevisionStore:
        """A store in an active session; `retrieved` simulates the server's reply."""
        store = RevisionStore(self.path, self.send)
        store.start_session("seed", 0, 1)
        if retrieved is not None:
            store.load_remote(retrieved)
        self.send.reset_mock()
        return store

    def sent_values(self) -> list[dict]:
        return [
            call.args[0][0]["operations"][0]["value"]
            for call in self.send.call_args_list
        ]


class TestLocalLedger(RevisionStoreTestCase):
    def test_start_session_requests_server_copy(self) -> None:
        store = RevisionStore(self.path, self.send)

        store.start_session("seed", 0, 1)

        self.send.assert_called_once_with([{"cmd": "Get", "keys": [REMOTE_KEY]}])
        self.assertFalse(store.remote_loaded)

    def test_persisted_cursor_survives_a_restart(self) -> None:
        self.make_store().persist(42, 3)

        self.assertEqual(3, self.make_store().local_cursor(42))

    def test_cursors_are_per_session(self) -> None:
        self.make_store().persist(42, 3)

        other = RevisionStore(self.path, self.send)
        other.start_session("other seed", 0, 1)

        self.assertIsNone(other.local_cursor(42))

    def test_failed_replace_does_not_mutate_in_memory_ledger(self) -> None:
        store = self.make_store()

        with patch.object(os, "replace", side_effect=OSError("failed")):
            with self.assertRaises(OSError):
                store.persist(42, 3)

        self.assertIsNone(store.local_cursor(42))

    def test_cursor_lookup_needs_a_session(self) -> None:
        store = RevisionStore(self.path, self.send)

        self.assertFalse(store.active)

        with self.assertRaises(RuntimeError):
            store.local_cursor(42)


class TestRemoteMirror(RevisionStoreTestCase):
    def test_persisted_revision_is_mirrored_to_data_storage(self) -> None:
        store = self.make_store(retrieved={})

        store.persist(42, 3)

        self.send.assert_called_once_with(
            [
                {
                    "cmd": "Set",
                    "key": REMOTE_KEY,
                    "default": {},
                    "want_reply": False,
                    "operations": [{"operation": "update", "value": {"0000002A": 3}}],
                }
            ]
        )

    def test_no_data_storage_write_while_disconnected(self) -> None:
        store = self.make_store(retrieved={})
        self.send.return_value = False

        store.persist(42, 3)

        self.assertIsNone(store.remote_cursor(42))
        self.assertEqual(3, store.local_cursor(42))

    def test_no_data_storage_write_before_retrieval(self) -> None:
        store = self.make_store()

        store.persist(42, 3)

        self.send.assert_not_called()
        self.assertEqual(3, store.local_cursor(42))

    def test_retrieval_pushes_only_missing_local_revisions(self) -> None:
        store = self.make_store()
        for checksum in (1, 2, 3):
            store.persist(checksum, checksum)

        store.load_remote({"00000001": 1, "00000002": 5})

        self.assertEqual([{"00000003": 3}], self.sent_values())
        self.assertEqual(5, store.remote_cursor(2))
        self.assertEqual(2, store.local_cursor(2))

    def test_malformed_server_revisions_are_dropped(self) -> None:
        store = self.make_store(
            retrieved={
                "0000002A": 3,
                "2a": 1,
                "0000002B": -1,
                "0000002C": True,
                "0000002D": "4",
            }
        )

        self.assertEqual(3, store.remote_cursor(0x2A))
        for checksum in (0x2B, 0x2C, 0x2D):
            self.assertIsNone(store.remote_cursor(checksum))

    def test_non_dict_server_value_is_treated_as_empty(self) -> None:
        store = self.make_store()

        store.load_remote(None)

        self.assertTrue(store.remote_loaded)
        self.assertIsNone(store.remote_cursor(42))

    def test_end_session_forgets_server_copy(self) -> None:
        store = self.make_store(retrieved={"0000002A": 3})

        store.end_session()

        self.assertFalse(store.remote_loaded)
        self.assertIsNone(store.remote_key)
