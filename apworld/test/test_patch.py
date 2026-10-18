import unittest
import xml.etree.ElementTree as XmlTree
from types import SimpleNamespace

from ..options import RONIN_ARC_NAME, ULTOR_EPILOGUE_ARC_NAME
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
        xml = generate_patched_mission_chains_file({RONIN_ARC_NAME}, False, False)
        missions = get_missions(xml)

        self.assertIsNotNone(missions["rn01"].find("StartNav"))

    def test_disabled_arc_removes_start_nav(self) -> None:
        xml = generate_patched_mission_chains_file(set(), False, False)
        missions = get_missions(xml)

        self.assertIsNone(missions["rn01"].find("StartNav"))

    def test_enabled_ultor_arc_keeps_start_nav_and_prerequisite_flag(self) -> None:
        xml = generate_patched_mission_chains_file(
            {ULTOR_EPILOGUE_ARC_NAME}, False, False
        )
        missions = get_missions(xml)
        ep00 = missions["ep00"]
        ep01 = missions["ep01"]

        self.assertIsNotNone(ep00.find("StartNav"))
        self.assertIn(
            "No Prerequisites Locked",
            [flag.text for flag in ep00.findall("./Flags/Flag")],
        )
        self.assertIsNotNone(ep01.find("StartNav"))
        self.assertEqual(
            ["ep00"],
            [
                prereq.text
                for prereq in ep01.findall(
                    "./MissionStronghold/Prerequisites/Prereq"
                )
            ],
        )

    def test_disabled_ultor_arc_converts_ep00_to_inert_mission(self) -> None:
        xml = generate_patched_mission_chains_file(set(), False, False)
        missions = get_missions(xml)
        ep00 = missions["ep00"]
        ep01 = missions["ep01"]
        flags = [flag.text for flag in ep00.findall("./Flags/Flag")]
        ep00_prerequisites = [
            prereq.text
            for prereq in ep00.findall(
                "./MissionStronghold/Prerequisites/Prereq"
            )
        ]
        ep01_prerequisites = [
            prereq.text
            for prereq in ep01.findall("./MissionStronghold/Prerequisites/Prereq")
        ]

        self.assertIsNone(ep00.find("StartNav"))
        self.assertEqual("Mission", ep00.findtext("Type"))
        self.assertIsNone(ep00.find("Silent"))
        self.assertEqual("0", ep00.findtext("./MissionStronghold/Cost"))
        self.assertEqual("none", ep00.findtext("./MissionStronghold/display_group"))
        self.assertNotIn("No Prerequisites Locked", flags)
        self.assertIn("No Percentage Count", flags)
        self.assertEqual(["tss04"], ep00_prerequisites)
        self.assertIsNotNone(ep01.find("StartNav"))
        self.assertEqual(["ep00"], ep01_prerequisites)

    def test_enabled_revelation_has_start_nav(self) -> None:
        xml = generate_patched_mission_chains_file(set(), True, True)
        missions = get_missions(xml)

        self.assertIsNotNone(missions["em01"].find("StartNav"))

    def test_disabled_revelation_has_no_start_nav(self) -> None:
        xml = generate_patched_mission_chains_file(set(), False, True)
        missions = get_missions(xml)

        self.assertIsNone(missions["em01"].find("StartNav"))

    def test_enabled_stilwater_caverns_has_start_nav(self) -> None:
        xml = generate_patched_mission_chains_file(set(), True, True)
        missions = get_missions(xml)

        self.assertIsNotNone(missions["sh_tss_caverns"].find("StartNav"))

    def test_disabled_stilwater_caverns_has_no_start_nav(self) -> None:
        xml = generate_patched_mission_chains_file(set(), True, False)
        missions = get_missions(xml)

        self.assertIsNone(missions["sh_tss_caverns"].find("StartNav"))

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
