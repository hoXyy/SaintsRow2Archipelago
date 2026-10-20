from __future__ import annotations

import contextlib
import copy
import json
import os
import re
from collections.abc import Callable
from pathlib import Path
from typing import Any

from CommonClient import logger

LEDGER_VERSION = 1


def _checksum_key(checksum: int) -> str:
    return f"{checksum:08X}"


class RevisionStore:
    def __init__(
        self, path: Path, send: Callable[[list[dict[str, Any]]], bool]
    ) -> None:
        """`send` queues messages to the AP server and returns False when offline."""
        self.path = path
        self._send = send
        self._ledger = self._load()
        self._session: str | None = None
        self._team_slot: tuple[int, int] | None = None
        # None until the server copy has been retrieved for this connection.
        self._remote: dict[str, int] | None = None

    def start_session(self, seed_name: str, team: int, slot: int) -> None:
        """Select the AP slot whose cursors are used and fetch the server copy."""
        self._session = f"{seed_name}|{team}|{slot}"
        self._team_slot = (team, slot)
        self._remote = None
        self._send([{"cmd": "Get", "keys": [self.remote_key]}])

    def end_session(self) -> None:
        self._session = None
        self._team_slot = None
        self._remote = None

    @property
    def active(self) -> bool:
        """Whether an AP session is selected; cursors can't be used without one."""
        return self._session is not None

    @property
    def remote_key(self) -> str | None:
        if self._team_slot is None:
            return None
        team, slot = self._team_slot
        return f"saints_row_2_revisions_v1_{team}_{slot}"

    @property
    def remote_loaded(self) -> bool:
        return self._remote is not None

    def local_cursor(self, checksum: int) -> int | None:
        value = self._session_revisions().get(_checksum_key(checksum))
        return None if value is None else int(value)

    def remote_cursor(self, checksum: int) -> int | None:
        if self._remote is None:
            return None
        return self._remote.get(_checksum_key(checksum))

    def persist(self, checksum: int, next_index: int) -> None:
        """Record a cursor in the ledger and mirror it to the server.

        Raises OSError if the ledger can't be written; the in-memory ledger is
        left untouched in that case.
        """
        key = _checksum_key(checksum)
        updated = copy.deepcopy(self._ledger)
        sessions = updated.setdefault("sessions", {})
        session = sessions.setdefault(self._require_session(), {})
        session.setdefault("revisions", {})[key] = next_index
        self._write(updated)
        self._ledger = updated
        self._push_remote({key: next_index})

    def load_remote(self, value: Any) -> None:
        """Accept the server copy, then upload any cursors it is missing."""
        # Data storage is writable by every client in the room, so only
        # well-formed entries are kept.
        remote: dict[str, int] = {}
        if isinstance(value, dict):
            remote = {
                key: cursor
                for key, cursor in value.items()
                if isinstance(key, str)
                and re.fullmatch(r"[0-9A-F]{8}", key)
                and type(cursor) is int
                and cursor >= 0
            }
        self._remote = remote

        local = {key: int(cursor) for key, cursor in self._session_revisions().items()}
        for key, cursor in local.items():
            if key in remote and remote[key] != cursor:
                logger.warning(
                    f"Server save revision for checksum={key} disagrees with the local "
                    f"ledger (server={remote[key]} local={cursor}); keeping local"
                )
        self._push_remote(local)

    def _push_remote(self, revisions: dict[str, int]) -> None:
        # Only entries the server doesn't have yet are sent, so an existing
        # server value is never overwritten.
        if self._remote is None:
            return
        missing = {
            key: cursor for key, cursor in revisions.items() if key not in self._remote
        }
        if not missing:
            return
        sent = self._send(
            [
                {
                    "cmd": "Set",
                    "key": self.remote_key,
                    "default": {},
                    "want_reply": False,
                    "operations": [{"operation": "update", "value": missing}],
                }
            ]
        )
        if sent:
            self._remote.update(missing)

    def _require_session(self) -> str:
        if self._session is None:
            raise RuntimeError("AP session identity is unavailable")
        return self._session

    def _session_revisions(self) -> dict[str, Any]:
        session = self._ledger["sessions"].get(self._require_session(), {})
        return session.get("revisions", {})

    def _load(self) -> dict[str, Any]:
        try:
            with self.path.open("r", encoding="utf-8") as file:
                value = json.load(file)
            if value.get("version") == LEDGER_VERSION and isinstance(
                value.get("sessions"), dict
            ):
                return value
        except (FileNotFoundError, json.JSONDecodeError, OSError, AttributeError):
            pass
        return {"version": LEDGER_VERSION, "sessions": {}}

    def _write(self, ledger: dict[str, Any]) -> None:
        # Write to a temporary file and swap it in, so a crash mid-write never
        # leaves a half-written ledger behind.
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self.path.with_suffix(".tmp")
        try:
            with temporary.open("w", encoding="utf-8") as file:
                json.dump(ledger, file, indent=2, sort_keys=True)
                file.flush()
                os.fsync(file.fileno())
            os.replace(temporary, self.path)
        except OSError:
            with contextlib.suppress(OSError):
                temporary.unlink(missing_ok=True)
            raise
