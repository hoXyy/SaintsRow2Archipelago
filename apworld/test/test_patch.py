import tempfile
import unittest
import xml.etree.ElementTree as XmlTree
import zipfile
from itertools import combinations, product
from pathlib import Path
from types import SimpleNamespace

from ..options import (
    BROTHERHOOD_ARC_NAME,
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)
from ..patch import (
    ARC_INTRO_CHAIN_MISSIONS,
    MISSION_GLOBALS_LUA_FILE_NAME,
    MISSIONS_TABLE_FILE,
    MISSIONS_TABLE_FILE_NAME,
    TEXT_LANGUAGES,
    VANILLA_MISSION_GLOBALS_LUA_FILE,
    generate_main_menu_info_strings,
    generate_patched_game_files,
    generate_patched_mission_chains_file,
    read_game_file,
)
from .bases import SR2TestBase

ARCS = (RONIN_ARC_NAME, SAMEDI_ARC_NAME, BROTHERHOOD_ARC_NAME, ULTOR_EPILOGUE_ARC_NAME)
BH01_CALL = b"cellphone_receive_call(CELLPHONE_CALL_BH01)"


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
                for prereq in ep01.findall("./MissionStronghold/Prerequisites/Prereq")
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
            for prereq in ep00.findall("./MissionStronghold/Prerequisites/Prereq")
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


class TestEveryMissionPatchCombination(unittest.TestCase):
    def test_only_the_selected_missions_change(self) -> None:
        vanilla = get_missions(read_game_file(MISSIONS_TABLE_FILE).decode("utf-8"))
        arc_subsets = [
            set(subset)
            for size in range(len(ARCS) + 1)
            for subset in combinations(ARCS, size)
        ]

        for arcs, revelation, caverns in product(
            arc_subsets, (False, True), (False, True)
        ):
            with self.subTest(
                arcs=sorted(arcs), revelation=revelation, caverns=caverns
            ):
                missions = get_missions(
                    generate_patched_mission_chains_file(arcs, revelation, caverns)
                )
                self.assertEqual(vanilla.keys(), missions.keys())

                start_navs = {
                    mission_name: arc in arcs
                    for arc, mission_name in ARC_INTRO_CHAIN_MISSIONS.items()
                } | {"em01": revelation, "sh_tss_caverns": caverns}
                for mission_name, has_start_nav in start_navs.items():
                    self.assertEqual(
                        has_start_nav,
                        missions[mission_name].find("StartNav") is not None,
                        mission_name,
                    )

                ep00 = missions["ep00"]
                if ULTOR_EPILOGUE_ARC_NAME in arcs:
                    self.assertEqual(
                        XmlTree.tostring(vanilla["ep00"]), XmlTree.tostring(ep00)
                    )
                else:
                    self.assertEqual("Mission", ep00.findtext("Type"))
                    self.assertIsNone(ep00.find("Silent"))
                    self.assertIsNotNone(ep00.find("MissionStronghold"))

                for mission_name, mission in missions.items():
                    if mission_name not in start_navs:
                        self.assertEqual(
                            XmlTree.tostring(vanilla[mission_name]),
                            XmlTree.tostring(mission),
                            mission_name,
                        )


class TestPatchContainer(SR2TestBase):
    auto_construct = False
    run_default_tests = False

    def write_container(self) -> zipfile.ZipFile:
        self.world_setup()
        output_directory = Path(self.enterContext(tempfile.TemporaryDirectory()))
        generate_patched_game_files(self.world, str(output_directory))

        (container_path,) = output_directory.glob("*.zip")
        return self.enterContext(zipfile.ZipFile(container_path))

    def read_seed_file(self, container: zipfile.ZipFile, name: str) -> bytes:
        return container.read(f"mods/sr2ap_seed_files/{name}")

    def test_container_has_every_seed_file(self) -> None:
        container = self.write_container()
        names = set(container.namelist())

        for name in (
            MISSIONS_TABLE_FILE_NAME,
            MISSION_GLOBALS_LUA_FILE_NAME,
            *(f"sr2ap_seed_{language}.cxt" for language in TEXT_LANGUAGES),
        ):
            with self.subTest(name=name):
                self.assertIn(f"mods/sr2ap_seed_files/{name}", names)

    def test_seed_files_match_the_world(self) -> None:
        self.options = {"required_gang_arcs": {RONIN_ARC_NAME}}
        container = self.write_container()
        missions = get_missions(
            self.read_seed_file(container, MISSIONS_TABLE_FILE_NAME).decode("utf-8")
        )
        menu_info = self.read_seed_file(container, "sr2ap_seed_US.cxt").decode("utf-8")

        self.assertIsNotNone(missions["rn01"].find("StartNav"))
        self.assertIsNone(missions["ss01"].find("StartNav"))
        self.assertIn(self.multiworld.seed_name, menu_info)

    def test_brotherhood_keeps_vanilla_mission_globals(self) -> None:
        self.options = {"required_gang_arcs": {BROTHERHOOD_ARC_NAME}}
        container = self.write_container()

        self.assertEqual(
            read_game_file(VANILLA_MISSION_GLOBALS_LUA_FILE),
            self.read_seed_file(container, MISSION_GLOBALS_LUA_FILE_NAME),
        )

    def test_no_brotherhood_removes_the_bh01_call(self) -> None:
        self.options = {"required_gang_arcs": {RONIN_ARC_NAME}}
        container = self.write_container()
        lua = self.read_seed_file(container, MISSION_GLOBALS_LUA_FILE_NAME)

        self.assertIn(BH01_CALL, read_game_file(VANILLA_MISSION_GLOBALS_LUA_FILE))
        self.assertNotIn(BH01_CALL, lua)
        self.assertIn(b"function bh01_phonecall()", lua)
