from datetime import datetime, UTC
import os
from typing import Set, TYPE_CHECKING
import xml.etree.ElementTree as XmlTree
import zipfile

from worlds.Files import APPlayerContainer
import Utils

if TYPE_CHECKING:
    from .world import SR2World

from .options import (
    RONIN_ARC_NAME,
    BROTHERHOOD_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)

MISSIONS_TABLE_FILE_NAME = "sr2_city_missions.xtbl"
MISSIONS_TABLE_FILE = os.path.join(
    os.path.dirname(__file__), "game_files", MISSIONS_TABLE_FILE_NAME
)

MISSION_GLOBALS_LUA_FILE_NAME = "mission_globals.lua"
VANILLA_MISSION_GLOBALS_LUA_FILE = os.path.join(
    os.path.dirname(__file__), "game_files", "mission_globals.lua"
)
MODIFIED_MISSION_GLOBALS_LUA_FILE = os.path.join(
    os.path.dirname(__file__), "game_files", "mission_globals_no_bh01_call.lua"
)

PATCHED_FILES_OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "patched_game_files")

ARC_INTRO_CHAIN_MISSIONS = {
    RONIN_ARC_NAME: "rn01",
    BROTHERHOOD_ARC_NAME: "bh01",
    SAMEDI_ARC_NAME: "ss01",
}


class SR2PatchContainer(APPlayerContainer):
    game = "Saints Row 2"
    patch_file_ending = ".zip"

    def __init__(
        self,
        patch_data: dict,
        base_path: str,
        output_dir: str,
        player=None,
        player_name: str = "",
        server: str = "",
    ):
        self.patch_data = patch_data
        self.file_path = base_path

        container_path = os.path.join(output_dir, base_path + ".zip")

        super().__init__(container_path, player, player_name, server)

    def write_contents(self, opened_zipfile: zipfile.ZipFile) -> None:
        opened_zipfile.mkdir("mods/sr2ap_seed_files")
        for filename, contents in self.patch_data.items():
            opened_zipfile.writestr(f"mods/sr2ap_seed_files/{filename}", contents)

        super().write_contents(opened_zipfile)


def generate_patched_game_files(world: "SR2World", output_directory: str) -> None:
    enabled_arcs = world.options.required_gang_arcs.value
    curr_timestamp = datetime.strftime(datetime.now(UTC), "%d%b%Y-%H%M%S")
    missions_globals_file_path = (
        MODIFIED_MISSION_GLOBALS_LUA_FILE
        if not BROTHERHOOD_ARC_NAME in enabled_arcs
        else VANILLA_MISSION_GLOBALS_LUA_FILE
    )

    with open(missions_globals_file_path, "r") as file:
        missions_global_file_content = file.read()

    patch_file_name = (
        f"AP-{world.multiworld.seed_name}-"
        f"P{world.player}-"
        f"{world.multiworld.get_file_safe_player_name(world.player)}-"
        f"{curr_timestamp}"
    )

    patch_dir = os.path.join(
        output_directory, patch_file_name + "-" + Utils.__version__
    )

    patch_files = {
        MISSIONS_TABLE_FILE_NAME: generate_patched_mission_chains_file(enabled_arcs),
        MISSION_GLOBALS_LUA_FILE_NAME: missions_global_file_content,  # this file needs to be included even if it's unmodified in case someone has a modified version installed already
    }

    patch = SR2PatchContainer(
        patch_files,
        patch_dir,
        output_directory,
        world.player,
        world.multiworld.get_file_safe_player_name(world.player),
    )

    patch.write()

    print(f"Wrote patched Saints Row 2 game files for player {world.player}")


def generate_patched_mission_chains_file(enabled_arcs: Set[str]) -> str:
    tree = XmlTree.parse(MISSIONS_TABLE_FILE)
    root = tree.getroot()

    missions = {
        mission.findtext("Name"): mission for mission in root.findall("./Table/Mission")
    }

    for arc, mission_name in ARC_INTRO_CHAIN_MISSIONS.items():
        mission = missions[mission_name]
        start_nav = mission.find("StartNav")

        if arc not in enabled_arcs:
            if start_nav is not None:
                mission.remove(start_nav)

    return XmlTree.tostring(root, encoding="unicode")
