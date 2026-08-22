import unittest
import xml.etree.ElementTree as XmlTree
from types import SimpleNamespace

from ..options import RONIN_ARC_NAME
from ..patch import (
    generate_patched_mission_chains_file,
    generate_main_menu_info_strings,
)


def get_missions(xml: str):
    root = XmlTree.fromstring(xml)
    return {
        mission.findtext("Name"): mission for mission in root.findall("./Table/Mission")
    }


class TestMissionPatch(unittest.TestCase):
    def test_enabled_arc_keeps_start_nav(self) -> None:
        xml = generate_patched_mission_chains_file({RONIN_ARC_NAME})
        missions = get_missions(xml)

        self.assertIsNotNone(missions["rn01"].find("StartNav"))

    def test_disabled_arc_removes_start_nav(self) -> None:
        xml = generate_patched_mission_chains_file(set())
        missions = get_missions(xml)

        self.assertIsNone(missions["rn01"].find("StartNav"))

    def test_menu_info_replaces_seed_and_player(self) -> None:
        world = SimpleNamespace(
            player=1,
            multiworld=SimpleNamespace(
                seed_name="Test Seed",
                player_name={1: "PascalHD"},
            ),
        )

        result = generate_main_menu_info_strings(world)

        self.assertIn("Test Seed", result)
        self.assertIn("PascalHD", result)
        self.assertNotIn("{{SEED}}", result)
        self.assertNotIn("{{PLAYER_NAME}}", result)
