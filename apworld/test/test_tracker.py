import json
import unittest
from importlib.resources import files

from ..collectibles import (
    PROGRESSIVE_CD_LOCATIONS,
    PROGRESSIVE_STUNT_JUMP_LOCATIONS,
    PROGRESSIVE_TAGS_LOCATIONS,
    STYLE_LEVEL_LOCATIONS,
)
from ..locations import LOCATION_NAME_TO_ID
from ..missions import get_mission_by_key

# The tracker folder lives in the apworld root, one package up.
TRACKER_DIR = files(__package__.rpartition(".")[0]).joinpath("tracker")

# Locations without a place on the map.
NOT_ON_MAP = {
    get_mission_by_key("tss01").name,
    *STYLE_LEVEL_LOCATIONS,
    *PROGRESSIVE_CD_LOCATIONS,
    *PROGRESSIVE_STUNT_JUMP_LOCATIONS,
    *PROGRESSIVE_TAGS_LOCATIONS,
}


def load(name: str):
    return json.loads(TRACKER_DIR.joinpath(name).read_text(encoding="utf-8"))


def walk(nodes: list[dict]):
    for node in nodes:
        yield node
        yield from walk(node.get("children", []))


class TestTrackerData(unittest.TestCase):
    def test_every_section_is_a_location(self) -> None:
        for node in walk(load("locations.json")):
            for section in node.get("sections", []):
                with self.subTest(section=section["name"]):
                    self.assertIn(section["name"], LOCATION_NAME_TO_ID)

    def test_every_location_is_on_the_map(self) -> None:
        sections = {
            section["name"]
            for node in walk(load("locations.json"))
            for section in node.get("sections", [])
        }

        self.assertEqual(set(), set(LOCATION_NAME_TO_ID) - sections - NOT_ON_MAP)

    def test_map_locations_reference_existing_maps(self) -> None:
        maps = {tracker_map["name"] for tracker_map in load("maps.json")}

        for node in walk(load("locations.json")):
            for map_location in node.get("map_locations", []):
                with self.subTest(location=node["name"]):
                    self.assertIn(map_location["map"], maps)

    def test_maps_reference_existing_images(self) -> None:
        for tracker_map in load("maps.json"):
            with self.subTest(map=tracker_map["name"]):
                self.assertTrue(TRACKER_DIR.joinpath(tracker_map["img"]).is_file())

    def test_layouts_reference_existing_maps(self) -> None:
        maps = {tracker_map["name"] for tracker_map in load("maps.json")}

        for name, layout in load("layouts.json").items():
            with self.subTest(layout=name):
                self.assertLessEqual(set(layout.get("maps", [])), maps)
