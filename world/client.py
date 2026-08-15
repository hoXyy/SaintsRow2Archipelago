from __future__ import annotations

import asyncio
import json
import os
from pathlib import Path
from typing import Any

from CommonClient import (
    ClientCommandProcessor,
    CommonContext,
    get_base_parser,
    gui_enabled,
    logger,
    server_loop,
)
from NetUtils import ClientStatus
from Utils import init_logging, user_path

from .activities import (
    ACTIVITIES_LEVEL_BASED,
    ACTIVITY_LEVEL_IDS,
    CHOP_SHOP_LISTS,
    HITMAN_LISTS,
)
from .collectibles import CD_IDS, CD_MAPPING
from .missions import MISSION_CHAINS, ULTOR_SECRET_MISSION

PROTOCOL_VERSION = 2
DEFAULT_PLUGIN_PORT = 38282
RETRY_SECONDS = 1.0


def _mission_locations() -> dict[str, int]:
    result: dict[str, int] = {}
    for chain in MISSION_CHAINS:
        for mission in (*chain["missions"], *chain["strongholds"]):
            result[mission.key] = mission.id
    result[ULTOR_SECRET_MISSION.key] = ULTOR_SECRET_MISSION.id
    return result


def _activity_locations() -> dict[str, tuple[str, str]]:
    result: dict[str, tuple[str, str]] = {}
    for activity, instances in ACTIVITIES_LEVEL_BASED.items():
        for instance in instances:
            for key, district in instance.items():
                result[key] = (activity, district)
    return result


def _diversion_locations(
    prefix: str, lists: dict[str, list[dict[str, str]]]
) -> dict[str, int]:
    result: dict[str, int] = {}
    for district, entries in lists.items():
        for entry in entries:
            for key, target in entry.items():
                name = f"{prefix} ({district}) - {target}"
                result[key] = ACTIVITY_LEVEL_IDS[name]
    return result


MISSION_LOCATIONS = _mission_locations()
ACTIVITY_LOCATIONS = _activity_locations()
CHOP_SHOP_LOCATIONS = _diversion_locations("Chop Shop", CHOP_SHOP_LISTS)
HITMAN_LOCATIONS = _diversion_locations("Hitman", HITMAN_LISTS)


class SR2CommandProcessor(ClientCommandProcessor):
    def _cmd_plugin(self) -> bool:
        """Show the SR2 plugin bridge state."""
        connected = self.ctx.plugin_writer is not None
        self.output(
            f"SR2 plugin: {'connected' if connected else 'waiting'}; "
            f"session: {'ready' if self.ctx.session_sent else 'inactive'}; "
            f"hello={int(self.ctx.plugin_hello)} "
            f"ap={int(self.ctx.slot is not None)} "
            f"inventory={int(self.ctx.inventory_synced)} "
            f"slot_data={int(self.ctx.slot_data is not None)}; "
            f"cursor: {self.ctx.live_cursor if self.ctx.live_cursor is not None else 'none'}"
        )
        return True


class SR2Context(CommonContext):
    game = "Saints Row 2"
    items_handling = 0b111
    command_processor = SR2CommandProcessor

    def __init__(
        self,
        server_address: str | None,
        password: str | None,
        plugin_port: int,
    ) -> None:
        super().__init__(server_address, password)
        self.plugin_port = plugin_port
        self.plugin_server: asyncio.Server | None = None
        self.plugin_writer: asyncio.StreamWriter | None = None
        self.plugin_hello = False
        self.session_sent = False
        self.inventory_synced = False
        self.slot_data: dict[str, Any] | None = None
        self.observed_locations: set[int] = set()
        self.live_cursor: int | None = None
        self.context_generation = 0
        self.pending_ack: asyncio.Future[tuple[int, bool]] | None = None
        self.bridge_event = asyncio.Event()
        self.ledger_path = Path(user_path("saints_row_2", "item_ledger.json"))
        self.ledger = self._load_ledger()

    def run_gui(self):
        from kvui import GameManager

        class SR2Manager(GameManager):
            base_title = "Saints Row 2 Client"

        self.ui = SR2Manager(self)
        self.ui_task = asyncio.create_task(self.ui.async_run(), name="UI")

    def on_package(self, cmd: str, args: dict[str, Any]) -> None:
        if cmd == "RoomInfo":
            self.seed_name = args["seed_name"]
        elif cmd == "Connected":
            self.slot_data = args.get("slot_data")
            # The AP server omits ReceivedItems when the initial inventory is
            # empty. Connected therefore establishes a valid empty inventory;
            # an immediately following ReceivedItems packet extends/replaces it.
            self.inventory_synced = True
            self.bridge_event.set()
        elif cmd == "ReceivedItems":
            start_index = int(args["index"])
            received_count = len(args["items"])
            self.inventory_synced = (
                start_index == 0
                or start_index + received_count == len(self.items_received)
            )
            self.bridge_event.set()

    async def server_auth(self, password_requested: bool = False) -> None:
        if password_requested and not self.password:
            await super().server_auth(password_requested)
        await self.get_username()
        await self.send_connect()

    async def connection_closed(self) -> None:
        await self.send_plugin({"type": "session_end"})
        self.session_sent = False
        self.inventory_synced = False
        self.slot_data = None
        self.live_cursor = None
        await super().connection_closed()

    async def send_plugin(self, message: dict[str, Any]) -> bool:
        if self.plugin_writer is None:
            return False
        try:
            encoded = json.dumps(message, separators=(",", ":")) + "\n"
            self.plugin_writer.write(encoded.encode("utf-8"))
            await self.plugin_writer.drain()
            return True
        except (ConnectionError, OSError):
            return False

    def session_key(self) -> str:
        if self.seed_name is None or self.team is None or self.slot is None:
            raise RuntimeError("AP session identity is unavailable")
        return f"{self.seed_name}|{self.team}|{self.slot}"

    def revision_cursor(self, checksum: int) -> int:
        session = self.ledger.get("sessions", {}).get(self.session_key(), {})
        revisions = session.get("revisions", {})
        return int(revisions.get(f"{checksum:08X}", 0))

    def persist_revision(self, checksum: int, next_index: int) -> None:
        sessions = self.ledger.setdefault("sessions", {})
        session = sessions.setdefault(self.session_key(), {})
        revisions = session.setdefault("revisions", {})
        revisions[f"{checksum:08X}"] = next_index
        self.ledger_path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self.ledger_path.with_suffix(".tmp")
        with temporary.open("w", encoding="utf-8") as file:
            json.dump(self.ledger, file, indent=2, sort_keys=True)
            file.flush()
            os.fsync(file.fileno())
        os.replace(temporary, self.ledger_path)

    def _load_ledger(self) -> dict[str, Any]:
        try:
            with self.ledger_path.open("r", encoding="utf-8") as file:
                value = json.load(file)
            if value.get("version") == 1 and isinstance(value.get("sessions"), dict):
                return value
        except (FileNotFoundError, json.JSONDecodeError, OSError, AttributeError):
            pass
        return {"version": 1, "sessions": {}}


async def send_session_ready(ctx: SR2Context) -> None:
    if (
        ctx.session_sent
        or not ctx.plugin_hello
        or not ctx.inventory_synced
        or ctx.slot_data is None
        or ctx.seed_name is None
        or ctx.team is None
        or ctx.slot is None
    ):
        return

    required = {
        "protocol",
        "managed_unlockables",
        "managed_cheats",
        "features",
        "enabled_progression",
    }

    if not required.issubset(ctx.slot_data):
        logger.error("This seed lacks SR2 protocol-v2 slot data; generate a new seed")
        return

    message = {
        "type": "session_ready",
        "protocol": ctx.slot_data["protocol"],
        "seed_name": ctx.seed_name,
        "team": ctx.team,
        "slot": ctx.slot,
        "managed_unlockables": sorted(set(ctx.slot_data["managed_unlockables"])),
        "managed_cheats": sorted(set(ctx.slot_data["managed_cheats"])),
        "features": ctx.slot_data["features"],
        "enabled_progression": ctx.slot_data["enabled_progression"],
    }
    if await ctx.send_plugin(message):
        ctx.session_sent = True
        logger.info("The game plugin accepted the current AP session data")


async def send_goal_if_complete(ctx: SR2Context) -> None:
    if ctx.finished_game or ctx.slot_data is None:
        return

    raw_goal_locations = ctx.slot_data.get("goal_locations")
    if not isinstance(raw_goal_locations, list) or not raw_goal_locations:
        return

    try:
        goal_locations = {int(location) for location in raw_goal_locations}
    except (TypeError, ValueError):
        logger.error("Ignored invalid SR2 goal location data")
        return

    if not all(
        location in ctx.locations_checked or location not in ctx.missing_locations
        for location in goal_locations
    ):
        return

    ctx.finished_game = True
    await ctx.send_msgs([{"cmd": "StatusUpdate", "status": ClientStatus.CLIENT_GOAL}])


async def submit_observed_locations(ctx: SR2Context) -> None:
    new_locations = sorted(
        (ctx.observed_locations & ctx.missing_locations) - ctx.locations_checked
    )

    if not new_locations:
        return

    ctx.locations_checked.update(new_locations)
    await ctx.send_msgs([{"cmd": "LocationChecks", "locations": new_locations}])
    await send_goal_if_complete(ctx)


async def process_game_context(ctx: SR2Context, message: dict[str, Any]) -> None:
    ctx.context_generation += 1
    ctx.pending_ack = None
    next_index = int(message["next_index"])
    if message.get("needs_cursor"):
        checksum = int(message["checksum"])
        next_index = ctx.revision_cursor(checksum)
        await ctx.send_plugin(
            {"type": "save_context", "checksum": checksum, "next_index": next_index}
        )

    ctx.live_cursor = next_index
    ctx.bridge_event.set()


async def process_progression(ctx: SR2Context, message: dict[str, Any]) -> None:
    previous = int(message.get("previous", 0))
    current = int(message.get("current", 0))
    if current <= previous:
        return

    category = message.get("category")
    key = message.get("key")
    locations: list[int] = []
    if category == "mission" and key in MISSION_LOCATIONS:
        locations.append(MISSION_LOCATIONS[key])
    elif category == "activity" and key in ACTIVITY_LOCATIONS:
        activity, district = ACTIVITY_LOCATIONS[key]
        newly_completed = current & ~previous
        for level in range(1, 7):
            if newly_completed & (1 << (level - 1)):
                locations.append(
                    ACTIVITY_LEVEL_IDS[f"{activity} ({district}) - Level {level}"]
                )
    elif category == "hitman" and key in HITMAN_LOCATIONS:
        locations.append(HITMAN_LOCATIONS[key])
    elif category == "chop_shop" and key in CHOP_SHOP_LOCATIONS:
        locations.append(CHOP_SHOP_LOCATIONS[key])
    elif category == "cd" and key in CD_MAPPING:
        locations.append(CD_IDS[CD_MAPPING[key]])

    ctx.observed_locations.update(locations)
    await submit_observed_locations(ctx)


async def process_plugin_message(ctx: SR2Context, message: dict[str, Any]) -> None:
    message_type = message.get("type")
    if message_type == "hello":
        if message.get("protocol") != PROTOCOL_VERSION:
            logger.error("SR2 plugin protocol mismatch")
            return
        ctx.plugin_hello = True
        ctx.bridge_event.set()
    elif message_type == "game_context":
        await process_game_context(ctx, message)
    elif message_type == "item_ack":
        if ctx.pending_ack is not None and not ctx.pending_ack.done():
            ctx.pending_ack.set_result(
                (int(message["index"]), bool(message["accepted"]))
            )
    elif message_type == "save_revision":
        ctx.persist_revision(int(message["checksum"]), int(message["next_index"]))
    elif message_type == "progression":
        await process_progression(ctx, message)
    elif message_type == "session_reject":
        logger.warning(message.get("message"))


async def plugin_connection(
    ctx: SR2Context, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
) -> None:
    if ctx.plugin_writer is not None:
        writer.close()
        await writer.wait_closed()
        return
    ctx.plugin_writer = writer
    ctx.plugin_hello = False
    ctx.session_sent = False
    ctx.live_cursor = None
    logger.info("Saints Row 2 game integration plugin connected")
    try:
        while line := await reader.readline():
            try:
                message = json.loads(line)
                if isinstance(message, dict):
                    await process_plugin_message(ctx, message)
            except (json.JSONDecodeError, KeyError, TypeError, ValueError):
                logger.warning("Ignored malformed SR2 plugin message")
    finally:
        if ctx.plugin_writer is writer:
            ctx.plugin_writer = None
            ctx.plugin_hello = False
            ctx.session_sent = False
            ctx.live_cursor = None
            ctx.pending_ack = None
        writer.close()
        await writer.wait_closed()
        logger.info("Saints Row 2 game integration plugin disconnected")


async def bridge_watcher(ctx: SR2Context) -> None:
    while not ctx.exit_event.is_set():
        await ctx.bridge_event.wait()
        ctx.bridge_event.clear()
        await send_session_ready(ctx)
        await submit_observed_locations(ctx)
        await send_goal_if_complete(ctx)

        while (
            ctx.session_sent
            and ctx.live_cursor is not None
            and ctx.live_cursor < len(ctx.items_received)
        ):
            index = ctx.live_cursor
            generation = ctx.context_generation
            item = ctx.items_received[index]
            name = ctx.item_names.lookup_in_game(item.item, ctx.game)
            ctx.pending_ack = asyncio.get_running_loop().create_future()
            if not await ctx.send_plugin(
                {"type": "item", "index": index, "name": name}
            ):
                break
            try:
                ack_index, accepted = await asyncio.wait_for(
                    ctx.pending_ack, timeout=5.0
                )
            except asyncio.TimeoutError:
                await asyncio.sleep(RETRY_SECONDS)
                continue
            if generation != ctx.context_generation:
                break
            if ack_index != index or not accepted:
                await asyncio.sleep(RETRY_SECONDS)
                continue
            ctx.live_cursor += 1


async def run_client(ctx: SR2Context) -> None:
    ctx.plugin_server = await asyncio.start_server(
        lambda reader, writer: plugin_connection(ctx, reader, writer),
        "127.0.0.1",
        ctx.plugin_port,
    )
    logger.info(f"Waiting for SR2 plugin on 127.0.0.1:{ctx.plugin_port}")
    ctx.server_task = asyncio.create_task(server_loop(ctx), name="ServerLoop")
    watcher = asyncio.create_task(bridge_watcher(ctx), name="SR2Bridge")
    if gui_enabled:
        ctx.run_gui()
    ctx.run_cli()

    await ctx.exit_event.wait()
    await ctx.send_plugin({"type": "session_end"})
    if ctx.plugin_writer is not None:
        ctx.plugin_writer.close()
        await ctx.plugin_writer.wait_closed()
    ctx.plugin_server.close()
    await ctx.plugin_server.wait_closed()
    watcher.cancel()
    await asyncio.gather(watcher, return_exceptions=True)
    await ctx.shutdown()


def launch(*launch_args: str) -> None:
    parser = get_base_parser(description="Saints Row 2 Archipelago client")
    parser.add_argument(
        "--plugin-port",
        type=int,
        default=DEFAULT_PLUGIN_PORT,
        help="localhost TCP port configured in SR2Archipelago.ini",
    )
    args = parser.parse_args(launch_args)
    init_logging("SR2Client", exception_logger="Client")

    async def main() -> None:
        ctx = SR2Context(args.connect, args.password, args.plugin_port)
        await run_client(ctx)

    asyncio.run(main())
