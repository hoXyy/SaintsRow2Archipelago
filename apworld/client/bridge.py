from __future__ import annotations

import asyncio
import json
from collections.abc import Awaitable, Callable
from typing import TYPE_CHECKING, Any

from CommonClient import logger

from ..activities import ACTIVITY_LEVEL_IDS, CHOP_SHOP_LISTS, HITMAN_LISTS
from .progression import locations_for_progress
from .save_sync import Inventory, accept_save_revision, resolve_save_context

if TYPE_CHECKING:
    from .context import SR2Context

PROTOCOL_VERSION = 8
SLOT_DATA_PROTOCOL_VERSION = 5
ACK_TIMEOUT_SECONDS = 5.0
RETRY_SECONDS = 1.0

REQUIRED_SLOT_DATA = {
    "protocol",
    "managed_unlockables",
    "managed_cheats",
    "persistent_items",
    "features",
    "enabled_progression",
}


def _list_location_ids(activity: str, lists: dict) -> dict[int, list[int]]:
    """Location ids per one-based list number, from the `_<list>_<n>` key suffix."""
    result: dict[int, list[int]] = {}
    for district, entries in lists.items():
        for entry in entries:
            for key, name in entry.items():
                list_number = int(key.rsplit("_", 2)[1])
                result.setdefault(list_number, []).append(
                    ACTIVITY_LEVEL_IDS[f"{activity} ({district}) - {name}"]
                )
    return result


LIST_LOCATION_IDS = {
    "chop_shop": _list_location_ids("Chop Shop", CHOP_SHOP_LISTS),
    "hitman": _list_location_ids("Hitman", HITMAN_LISTS),
}


class PluginBridge:
    def __init__(self, ctx: SR2Context, port: int) -> None:
        self.ctx = ctx
        self.port = port
        self.server: asyncio.Server | None = None
        self.writer: asyncio.StreamWriter | None = None

        # Per plugin connection:
        self.hello = False
        self.pending_ack: asyncio.Future[tuple[int, bool]] | None = None
        # Per AP session (cleared when either side disconnects):
        self.session_sent = False
        # Index of the next received item to deliver; None until a save is loaded.
        self.live_cursor: int | None = None

        # Bumped on every game_context, so an item delivery that was in flight
        # when the game loaded another save can tell it is stale.
        self.context_generation = 0

        self._handlers: dict[str, Callable[[dict[str, Any]], Awaitable[None]]] = {
            "hello": self._on_hello,
            "game_context": self._on_game_context,
            "item_ack": self._on_item_ack,
            "save_revision": self._on_save_revision,
            "progression": self._on_progression,
            "session_reject": self._on_session_reject,
            "death": self._on_death,
            "list_opened": self._on_list_opened,
        }

    @property
    def connected(self) -> bool:
        return self.writer is not None

    # Lifecycle

    async def start(self) -> None:
        self.server = await asyncio.start_server(
            self._handle_connection, "127.0.0.1", self.port
        )
        logger.info(f"Waiting for SR2 plugin on 127.0.0.1:{self.port}")

    async def stop(self) -> None:
        await self.send({"type": "session_end"})
        if self.writer is not None:
            self.writer.close()
            await self.writer.wait_closed()
        if self.server is not None:
            self.server.close()
            await self.server.wait_closed()

    async def end_session(self) -> None:
        """Tell the plugin the AP session is gone. The plugin stays connected."""
        await self.send({"type": "session_end"})
        self._reset_session()

    def _reset_session(self) -> None:
        self.session_sent = False
        self.live_cursor = None

    def _reset_connection(self) -> None:
        self.hello = False
        self.pending_ack = None
        self._reset_session()

    async def send(self, message: dict[str, Any]) -> bool:
        if self.writer is None:
            return False
        try:
            encoded = json.dumps(message, separators=(",", ":")) + "\n"
            self.writer.write(encoded.encode("utf-8"))
            await self.writer.drain()
            return True
        except (ConnectionError, OSError):
            return False

    async def _handle_connection(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        # Only one plugin at a time.
        if self.writer is not None:
            writer.close()
            await writer.wait_closed()
            return
        self.writer = writer
        self._reset_connection()
        logger.info("Saints Row 2 game integration plugin connected")
        try:
            while line := await reader.readline():
                try:
                    message = json.loads(line)
                    if isinstance(message, dict):
                        await self.handle_message(message)
                except (json.JSONDecodeError, KeyError, TypeError, ValueError):
                    logger.warning("Ignored malformed SR2 plugin message")
        finally:
            if self.writer is writer:
                self.writer = None
                self._reset_connection()
            writer.close()
            await writer.wait_closed()
            logger.info("Saints Row 2 game integration plugin disconnected")

    # Client -> plugin

    async def send_session_ready(self) -> None:
        """Start the plugin session once both the plugin and AP are ready."""
        ctx = self.ctx
        if (
            self.session_sent
            or not self.hello
            or not ctx.inventory_synced
            or ctx.slot_data is None
            or not ctx.revisions.remote_loaded
            or ctx.seed_name is None
            or ctx.team is None
            or ctx.slot is None
        ):
            return

        slot_data = ctx.slot_data
        if not REQUIRED_SLOT_DATA.issubset(slot_data):
            logger.error(
                "This seed lacks compatible SR2 protocol slot data; generate a new seed"
            )
            return
        if slot_data["protocol"] != SLOT_DATA_PROTOCOL_VERSION:
            logger.error(
                "This seed uses an incompatible SR2 protocol version; generate a new seed"
            )
            return

        message = {
            "type": "session_ready",
            "protocol": PROTOCOL_VERSION,
            "seed_name": ctx.seed_name,
            "team": ctx.team,
            "slot": ctx.slot,
            "managed_unlockables": sorted(set(slot_data["managed_unlockables"])),
            "managed_cheats": sorted(set(slot_data["managed_cheats"])),
            "persistent_items": sorted(set(slot_data["persistent_items"])),
            "features": slot_data["features"],
            "enabled_progression": slot_data["enabled_progression"],
        }
        if await self.send(message):
            self.session_sent = True
            logger.info("The game plugin accepted the current AP session data")

    async def deliver_items(self) -> None:
        """Send received items from the live cursor onward, one ack at a time."""
        ctx = self.ctx
        while (
            self.session_sent
            and self.live_cursor is not None
            and self.live_cursor < len(ctx.items_received)
        ):
            index = self.live_cursor
            generation = self.context_generation
            item = ctx.items_received[index]
            message = {
                "type": "item",
                "index": index,
                "name": ctx.item_names.lookup_in_game(item.item, ctx.game),
                "flags": item.flags,
            }
            if item.player != ctx.slot:
                message["sender"] = ctx.player_names[item.player]

            self.pending_ack = asyncio.get_running_loop().create_future()
            if not await self.send(message):
                break
            try:
                ack_index, accepted = await asyncio.wait_for(
                    self.pending_ack, timeout=ACK_TIMEOUT_SECONDS
                )
            except TimeoutError:
                await asyncio.sleep(RETRY_SECONDS)
                continue
            if generation != self.context_generation:
                break
            if ack_index != index or not accepted:
                await asyncio.sleep(RETRY_SECONDS)
                continue
            self.live_cursor += 1

    # Plugin -> client

    async def handle_message(self, message: dict[str, Any]) -> None:
        handler = self._handlers.get(message.get("type"))
        if handler is not None:
            await handler(message)

    async def _on_hello(self, message: dict[str, Any]) -> None:
        if message.get("protocol") != PROTOCOL_VERSION:
            logger.error("SR2 plugin protocol mismatch")
            return
        self.hello = True
        self.ctx.request_sync()

    async def _on_game_context(self, message: dict[str, Any]) -> None:
        self.context_generation += 1
        self.pending_ack = None
        next_index = int(message["next_index"])

        if not message.get("needs_cursor"):
            # The plugin has settled on a save; deliver from here.
            self.live_cursor = next_index
            self.ctx.request_sync()
            return

        checksum = int(message["checksum"])
        revisions = self.ctx.revisions
        if not revisions.active:
            logger.warning(
                f"Ignored SR2 save context while disconnected from AP "
                f"checksum={checksum:08X}"
            )
            return
        resolved = resolve_save_context(
            checksum,
            plugin_cursor=next_index if message.get("snapshot_known") is True else None,
            plugin_items=list(message["persistent_items"]),
            local_cursor=revisions.local_cursor(checksum),
            remote_cursor=revisions.remote_cursor(checksum),
            inventory=self._inventory(),
        )
        if resolved is None:
            return
        if resolved.persist:
            try:
                revisions.persist(checksum, resolved.next_index)
            except OSError as error:
                logger.error(
                    f"Could not store client save revision checksum={checksum:08X}: "
                    f"{error}"
                )
                return
        if resolved.from_server:
            logger.info(
                f"Recovered save revision from the server checksum={checksum:08X} "
                f"next_index={resolved.next_index}"
            )

        await self.send(
            {
                "type": "save_context",
                "checksum": checksum,
                "next_index": resolved.next_index,
                "snapshot_known": True,
                "persistent_items": resolved.persistent_items,
            }
        )

    async def _on_item_ack(self, message: dict[str, Any]) -> None:
        if self.pending_ack is not None and not self.pending_ack.done():
            self.pending_ack.set_result(
                (int(message["index"]), bool(message["accepted"]))
            )

    async def _on_save_revision(self, message: dict[str, Any]) -> None:
        checksum = int(message["checksum"])
        next_index = int(message["next_index"])
        await self.send(
            {
                "type": "save_revision_ack",
                "checksum": checksum,
                "next_index": next_index,
                "accepted": self._record_save_revision(checksum, next_index),
            }
        )

    def _record_save_revision(self, checksum: int, next_index: int) -> bool:
        revisions = self.ctx.revisions
        if not revisions.active:
            logger.warning(
                f"Rejected SR2 save revision while disconnected from AP "
                f"checksum={checksum:08X}"
            )
            return False

        local_cursor = revisions.local_cursor(checksum)
        if not accept_save_revision(
            checksum, next_index, local_cursor, len(self.ctx.items_received)
        ):
            return False
        if local_cursor is not None:
            # Already recorded with the same cursor.
            return True
        try:
            revisions.persist(checksum, next_index)
        except OSError as error:
            logger.error(
                f"Could not store client save revision checksum={checksum:08X}: {error}"
            )
            return False
        return True

    async def _on_progression(self, message: dict[str, Any]) -> None:
        locations = locations_for_progress(
            message.get("category"),
            message.get("key"),
            int(message.get("previous", 0)),
            int(message.get("current", 0)),
        )
        self.ctx.observed_locations.update(locations)
        await self.ctx.submit_observed_locations()

    async def _on_session_reject(self, message: dict[str, Any]) -> None:
        logger.warning(message.get("message"))

    def _inventory(self) -> Inventory:
        ctx = self.ctx
        return Inventory(
            names=[
                ctx.item_names.lookup_in_game(item.item, ctx.game)
                for item in ctx.items_received
            ],
            persistent_names=frozenset(ctx.slot_data["persistent_items"]),
        )

    async def _on_death(self, _message: dict[str, Any]) -> None:
        await self.ctx.send_death(
            f"{self.ctx.player_names[self.ctx.slot]} got smoked on the streets of Stilwater."
        )

    async def _on_list_opened(self, message: dict[str, Any]) -> None:
        lists = LIST_LOCATION_IDS.get(message["category"], {})
        location_ids = [
            location_id
            for location_id in lists.get(message["list"], [])
            if location_id in self.ctx.missing_locations
        ]
        if location_ids:
            await self.ctx.send_hints(location_ids)
