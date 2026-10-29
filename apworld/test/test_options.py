from Options import OptionError

from ..items_list import CHEAT_ITEMS, MONEY_ITEM_NAMES, WEAPON_ITEM_NAMES
from ..missions import get_required_respect_count
from ..options import (
    BROTHERHOOD_ARC_NAME,
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
    StartingActivity,
)
from .bases import STARTING_ACTIVITY_INCLUDE_OPTIONS, SR2TestBase

NO_FILLER_WEIGHTS = {
    "money_filler_item_weight": 0,
    "weapon_filler_item_weight": 0,
    "vehicle_filler_item_weight": 0,
    "misc_filler_item_weight": 0,
}

# Ronin needs 17 Respect. Demolition Derby gives 6 Respect-safe locations and
# Jailbreak 1 (Appointed Defender holds the starting pass), so style levels
# make up the rest.
ONLY_DEMO_DERBY = dict.fromkeys(STARTING_ACTIVITY_INCLUDE_OPTIONS.values(), 0) | {
    "include_demo_derby_levels": 6,
    "starting_activity": StartingActivity.option_demo_derby,
    "required_gang_arcs": {RONIN_ARC_NAME},
}


class TestRoninOnly(SR2TestBase):
    options = {"required_gang_arcs": {RONIN_ARC_NAME}}


class TestSamediOnly(SR2TestBase):
    options = {"required_gang_arcs": {SAMEDI_ARC_NAME}}


class TestBrotherhoodOnly(SR2TestBase):
    options = {"required_gang_arcs": {BROTHERHOOD_ARC_NAME}}


class TestUltorOnly(SR2TestBase):
    options = {"required_gang_arcs": {ULTOR_EPILOGUE_ARC_NAME}}


class TestFullGame(SR2TestBase):
    options = {
        "required_gang_arcs": {
            BROTHERHOOD_ARC_NAME,
            SAMEDI_ARC_NAME,
            RONIN_ARC_NAME,
            ULTOR_EPILOGUE_ARC_NAME,
        }
    }


class TestInvalidOptions(SR2TestBase):
    auto_construct = False
    run_default_tests = False

    def test_requires_at_least_one_arc(self) -> None:
        self.options = {"required_gang_arcs": set()}

        with self.assertRaisesRegex(OptionError, "enable any gang arcs"):
            self.world_setup()

    def test_filler_weights_need_full_trap_chance(self) -> None:
        self.options = NO_FILLER_WEIGHTS | {"trap_chance": 99}

        with self.assertRaisesRegex(OptionError, "filler category weight"):
            self.world_setup()

    def test_filler_weights_need_an_enabled_trap_type(self) -> None:
        self.options = NO_FILLER_WEIGHTS | {
            "trap_chance": 100,
            "include_weather_trap_items": 0,
            "include_notoriety_trap_items": 0,
        }

        with self.assertRaisesRegex(OptionError, "filler category weight"):
            self.world_setup()

    def test_too_few_respect_safe_locations_are_rejected(self) -> None:
        self.options = ONLY_DEMO_DERBY | {"style_level_location_count": 0}

        with self.assertRaisesRegex(OptionError, "Respect-safe locations"):
            self.world_setup()

    def test_excluded_style_levels_do_not_count_as_respect_safe(self) -> None:
        # Style levels 8-10 can't hold progression, so this is 3 short.
        self.options = ONLY_DEMO_DERBY | {"style_level_location_count": 10}

        with self.assertRaisesRegex(OptionError, "Respect-safe locations"):
            self.world_setup()

    def test_item_pool_larger_than_locations_is_rejected(self) -> None:
        # Exactly enough Respect-safe locations, but the doubled Respect plus
        # the Plane Races pass is one item more than there are locations.
        self.options = ONLY_DEMO_DERBY | {
            "include_plane_races": 1,
            "style_level_location_count": 4,
            "bonus_respect_percentage": 100,
        }

        with self.assertRaisesRegex(OptionError, "available locations"):
            self.world_setup()


class TestOnlyTrapFiller(SR2TestBase):
    options = NO_FILLER_WEIGHTS | {"trap_chance": 100}

    def test_no_regular_filler_is_generated(self) -> None:
        names = {
            item.name for item in self.multiworld.itempool if item.player == self.player
        }

        self.assertEqual(set(), names & set(MONEY_ITEM_NAMES + WEAPON_ITEM_NAMES))
        self.assertEqual(set(), names & set(CHEAT_ITEMS))


class TestExactlyEnoughRespectSafeLocations(SR2TestBase):
    options = ONLY_DEMO_DERBY | {
        "style_level_location_count": 10,
        "allow_progression_items_on_high_style_level": 1,
    }

    def test_every_respect_safe_location_holds_respect(self) -> None:
        respect_safe = [
            location
            for location in self.multiworld.get_unfilled_locations(self.player)
            if location.respect_safe
        ]

        self.assertEqual(get_required_respect_count(self.world), len(respect_safe))
