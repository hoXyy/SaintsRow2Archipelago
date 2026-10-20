from __future__ import annotations

import asyncio
import pkgutil
from pathlib import Path
from typing import Any

try:
    from worlds.tracker.TrackerClient import (
        TrackerCommandProcessor as SuperCommandProcessor,
        TrackerGameContext as SuperContext,
    )

    tracker_loaded = True
except ModuleNotFoundError:
    from CommonClient import (
        ClientCommandProcessor as SuperCommandProcessor,
        CommonContext as SuperContext,
    )

    tracker_loaded = False

from NetUtils import ClientStatus
from Utils import user_path

from CommonClient import logger

from ..items import BONUS_RESPECT_ITEM_NAME, ITEM_NAME_TO_ID, RESPECT_ITEM_NAME
from ..missions import ALL_MISSIONS
from .bridge import PluginBridge
from .revisions import RevisionStore


def calculate_ut_respect(
    items_received: list[Any], checked_locations: set[int] | frozenset[int]
) -> tuple[int, int]:
    respect_item_ids = {
        ITEM_NAME_TO_ID[RESPECT_ITEM_NAME],
        ITEM_NAME_TO_ID[BONUS_RESPECT_ITEM_NAME],
    }
    received_respect = sum(item.item in respect_item_ids for item in items_received)
    spent_respect = sum(
        mission.required_respect
        for mission in ALL_MISSIONS
        if mission.id in checked_locations
    )
    return received_respect, spent_respect


class SR2CommandProcessor(SuperCommandProcessor):
    def _cmd_plugin(self) -> bool:
        """Show the SR2 plugin bridge state."""
        bridge = self.ctx.bridge
        cursor = bridge.live_cursor
        self.output(
            f"SR2 plugin: {'connected' if bridge.connected else 'waiting'}; "
            f"session: {'ready' if bridge.session_sent else 'inactive'}; "
            f"hello={int(bridge.hello)} "
            f"ap={int(self.ctx.slot is not None)} "
            f"inventory={int(self.ctx.inventory_synced)} "
            f"slot_data={int(self.ctx.slot_data is not None)}; "
            f"cursor: {cursor if cursor is not None else 'none'}"
        )
        return True


class SR2Context(SuperContext):
    game = "Saints Row 2"
    tags = {"AP"}
    items_handling = 0b111
    command_processor = SR2CommandProcessor

    def __init__(
        self,
        server_address: str | None,
        password: str | None,
        plugin_port: int,
    ) -> None:
        super().__init__(server_address, password)
        self.slot_data: dict[str, Any] | None = None
        self.inventory_synced = False
        self.observed_locations: set[int] = set()
        self.sync_event = asyncio.Event()
        self.revisions = RevisionStore(
            Path(user_path("saints_row_2", "item_ledger.json")), self.send_msgs_soon
        )
        self.bridge = PluginBridge(self, plugin_port)

    def request_sync(self) -> None:
        """Wake `sync_loop`; call after anything it depends on changes."""
        self.sync_event.set()

    async def sync_loop(self) -> None:
        while not self.exit_event.is_set():
            await self.sync_event.wait()
            self.sync_event.clear()
            await self.bridge.send_session_ready()
            await self.submit_observed_locations()
            await self.send_goal_if_complete()
            await self.bridge.deliver_items()

    def send_msgs_soon(self, msgs: list[dict[str, Any]]) -> bool:
        """Queue messages to the AP server without waiting; False when offline."""
        if not (self.server and self.server.socket):
            return False
        asyncio.create_task(self.send_msgs(msgs))
        return True

    # AP server events

    def on_deathlink(self, data: dict) -> None:
        super().on_deathlink(data)

        asyncio.create_task(
            self.bridge.send(
                {
                    "type": "death_link",
                    "source": data.get("source", ""),
                    "cause": data.get("cause", ""),
                }
            )
        )

    def on_package(self, cmd: str, args: dict[str, Any]) -> None:
        super().on_package(cmd, args)

        if cmd == "RoomInfo":
            self.seed_name = args["seed_name"]
        elif cmd == "Connected":
            self.slot_data = args.get("slot_data")
            features = (self.slot_data or {}).get("features", {})
            asyncio.create_task(
                self.update_death_link(features.get("death_link", False))
            )
            self.inventory_synced = True
            self.revisions.start_session(self.seed_name, self.team, self.slot)
            self.request_sync()
        elif cmd == "Retrieved" and self.revisions.remote_key in args["keys"]:
            self.revisions.load_remote(args["keys"][self.revisions.remote_key])
            self.request_sync()
        elif cmd == "ReceivedItems":
            start_index = int(args["index"])
            received_count = len(args["items"])
            self.inventory_synced = (
                start_index == 0
                or start_index + received_count == len(self.items_received)
            )
            self.request_sync()
        elif cmd == "PrintJSON" and args.get("type") == "ItemSend":
            self._notify_item_sent(args)

    def _notify_item_sent(self, args: dict[str, Any]) -> None:
        # Show our items that went to other players in the game's HUD.
        item = args["item"]
        receiving = args["receiving"]
        if item.player != self.slot or receiving == self.slot:
            return
        asyncio.create_task(
            self.bridge.send(
                {
                    "type": "item_sent",
                    "item": self.item_names.lookup_in_slot(item.item, receiving),
                    "receiver": self.player_names[receiving],
                    "flags": item.flags,
                }
            )
        )

    async def server_auth(self, password_requested: bool = False) -> None:
        if password_requested and not self.password:
            await super().server_auth(password_requested)
        await self.get_username()
        await self.send_connect()

    async def connection_closed(self) -> None:
        await self.bridge.end_session()
        self.inventory_synced = False
        self.slot_data = None
        self.revisions.end_session()
        await super().connection_closed()

    # Locations and goal

    async def submit_observed_locations(self) -> None:
        new_locations = sorted(
            (self.observed_locations & self.missing_locations) - self.locations_checked
        )
        if not new_locations:
            return

        self.locations_checked.update(new_locations)
        await self.send_msgs([{"cmd": "LocationChecks", "locations": new_locations}])
        await self.send_goal_if_complete()

    async def send_goal_if_complete(self) -> None:
        if self.finished_game or self.slot_data is None:
            return

        raw_goal_locations = self.slot_data.get("goal_locations")
        if not isinstance(raw_goal_locations, list) or not raw_goal_locations:
            return

        try:
            goal_locations = {int(location) for location in raw_goal_locations}
        except (TypeError, ValueError):
            logger.error("Ignored invalid SR2 goal location data")
            return

        completed_locations = self.checked_locations | self.locations_checked
        if not goal_locations.issubset(completed_locations):
            return

        self.finished_game = True
        await self.send_msgs(
            [{"cmd": "StatusUpdate", "status": ClientStatus.CLIENT_GOAL}]
        )

    # GUI and Universal Tracker

    def make_gui(self):
        base_manager = super().make_gui()

        icon_path = Path(user_path("saints_row_2", "client_icon.png"))
        # icon.png lives in the apworld root, one package up.
        world_package = __package__.rpartition(".")[0]
        icon_data = pkgutil.get_data(world_package, "icon.png")

        if icon_data is not None:
            icon_path.parent.mkdir(parents=True, exist_ok=True)
            if not icon_path.exists() or icon_path.read_bytes() != icon_data:
                icon_path.write_bytes(icon_data)

        class SR2Manager(base_manager):
            base_title = "Saints Row 2 Client"

            def __init__(self, ctx):
                super().__init__(ctx)
                if icon_data is not None:
                    self.icon = str(icon_path)

        return SR2Manager

    def updateTracker(self):
        world = self.tracker_core.get_current_world()

        if world is not None:
            checked_locations = frozenset(self.checked_locations)
            received_respect, spent_respect = calculate_ut_respect(
                self.items_received,
                checked_locations,
            )

            world.ut_checked_locations = checked_locations
            world.ut_received_respect = received_respect
            world.ut_spent_respect = spent_respect

        return super().updateTracker()
