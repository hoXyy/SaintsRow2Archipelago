import unittest

from ..activities import ACTIVITY_LEVEL_IDS, MEDAL_STRINGS, RACES
from ..client.progression import (
    CHOP_SHOP_LOCATIONS,
    HITMAN_LOCATIONS,
    locations_for_progress,
)
from ..collectibles import (
    CD_IDS,
    CD_MAPPING,
    MISSABLE_STUNT_JUMP_MAPPING,
    STUNT_JUMP_IDS,
    STUNT_JUMP_MAPPING,
    STYLE_LEVEL_IDS,
    STYLE_LEVEL_LOCATIONS,
)
from ..missions import get_mission_by_key


class TestLocationsForProgress(unittest.TestCase):
    def test_mission(self) -> None:
        mission = get_mission_by_key("rn01")

        self.assertEqual(
            [mission.id], locations_for_progress("mission", mission.key, 0, 1)
        )

    def test_activity_reports_only_new_bits(self) -> None:
        level_2 = ACTIVITY_LEVEL_IDS["Crowd Control (Hotels & Marina) - Level 2"]
        level_3 = ACTIVITY_LEVEL_IDS["Crowd Control (Hotels & Marina) - Level 3"]

        # Level 1 was already complete; levels 2 and 3 are newly complete.
        locations = locations_for_progress("activity", "crowd_ht", 0b000001, 0b000111)

        self.assertEqual({level_2, level_3}, set(locations))

    def test_gold_race_reports_all_medals(self) -> None:
        race = RACES["car_pj"]
        expected = {
            ACTIVITY_LEVEL_IDS[f"{race} - {MEDAL_STRINGS['bronze']}"],
            ACTIVITY_LEVEL_IDS[f"{race} - {MEDAL_STRINGS['silver']}"],
            ACTIVITY_LEVEL_IDS[f"{race} - {MEDAL_STRINGS['gold']}"],
        }

        locations = locations_for_progress("racing", "car_pj", 0, 3)

        self.assertEqual(expected, set(locations))

    def test_silver_race_reports_silver_and_bronze(self) -> None:
        race = RACES["car_pj"]
        expected = {
            ACTIVITY_LEVEL_IDS[f"{race} - {MEDAL_STRINGS['bronze']}"],
            ACTIVITY_LEVEL_IDS[f"{race} - {MEDAL_STRINGS['silver']}"],
        }

        locations = locations_for_progress("racing", "car_pj", 1, 2)

        self.assertEqual(expected, set(locations))

    def test_style_level_reports_every_level_up_to_current(self) -> None:
        expected = [STYLE_LEVEL_IDS[name] for name in STYLE_LEVEL_LOCATIONS[:3]]

        self.assertEqual(expected, locations_for_progress("style_level", None, 1, 3))

    def test_cd(self) -> None:
        key, name = next(iter(CD_MAPPING.items()))

        self.assertEqual([CD_IDS[name]], locations_for_progress("cd", key, 0, 1))

    def test_stunt_jump(self) -> None:
        key, name = next(iter(STUNT_JUMP_MAPPING.items()))

        self.assertEqual(
            [STUNT_JUMP_IDS[name]], locations_for_progress("stunt_jumps", key, 0, 1)
        )

    def test_missable_stunt_jump(self) -> None:
        key, name = next(iter(MISSABLE_STUNT_JUMP_MAPPING.items()))

        self.assertEqual(
            [STUNT_JUMP_IDS[name]], locations_for_progress("stunt_jumps", key, 0, 1)
        )

    def test_hitman(self) -> None:
        key, location_id = next(iter(HITMAN_LOCATIONS.items()))

        self.assertEqual([location_id], locations_for_progress("hitman", key, 0, 1))

    def test_chop_shop(self) -> None:
        key, location_id = next(iter(CHOP_SHOP_LOCATIONS.items()))

        self.assertEqual([location_id], locations_for_progress("chop_shop", key, 0, 1))

    def test_unknown_key_is_ignored(self) -> None:
        self.assertEqual([], locations_for_progress("mission", "unknown", 0, 1))

    def test_unknown_category_is_ignored(self) -> None:
        self.assertEqual([], locations_for_progress("unknown", "rn01", 0, 1))

    def test_no_progress_is_ignored(self) -> None:
        mission = get_mission_by_key("rn01")

        self.assertEqual([], locations_for_progress("mission", mission.key, 1, 1))
