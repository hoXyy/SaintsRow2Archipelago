import random
import re
from collections import Counter
from typing import Any

from BaseClasses import LocationProgressType
from Fill import distribute_items_restrictive
from Options import OptionError

from ..items import (
    ACTIVITY_UNLOCK_ITEMS_SETTINGS_MAPPING,
    BONUS_RESPECT_ITEM_NAME,
    EM01_UNLOCK_ITEM,
    RESPECT_ITEM_NAME,
    TSS_SH_CAVERNS_UNLOCK_ITEM,
)
from ..items_list import (
    CHEAT_ITEMS,
    FILLER_UNLOCKABLES,
    MISC_CHEAT_ITEMS,
    MONEY_ITEM_NAMES,
    TRAP_CHEATS,
    TRAP_ITEMS,
    TRAP_NOTORIETY_ITEMS,
    USEFUL_UNLOCKABLES,
    VEHICLE_CHEAT_ITEMS,
    WEAPON_ITEM_NAMES,
)
from ..missions import (
    STILWATER_CAVERNS_STRONGHOLD,
    ULTOR_SECRET_MISSION,
    get_mission_by_key,
    get_required_respect_count,
)
from ..options import (
    BROTHERHOOD_ARC_NAME,
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)
from .bases import STARTING_ACTIVITY_INCLUDE_OPTIONS, SR2TestBase
from .fuzz import fuzz_cases, sample_range

ARCS = (RONIN_ARC_NAME, SAMEDI_ARC_NAME, BROTHERHOOD_ARC_NAME, ULTOR_EPILOGUE_ARC_NAME)

FILLER_WEIGHT_CATEGORIES = {
    "money_filler_item_weight": MONEY_ITEM_NAMES,
    "weapon_filler_item_weight": WEAPON_ITEM_NAMES,
    "vehicle_filler_item_weight": VEHICLE_CHEAT_ITEMS,
    "misc_filler_item_weight": MISC_CHEAT_ITEMS,
}

TOGGLES = (
    "include_secret_mission",
    "include_secret_mission_as_goal",
    "include_stilwater_caverns",
    "include_stilwater_caverns_as_goal",
    "allow_progression_items_on_high_style_level",
    "include_weather_trap_items",
    "include_notoriety_trap_items",
    "death_link",
)

FILLER_WEIGHT_ERROR = "filler category weight"
NO_ARCS_ERROR = "enable any gang arcs"
NO_ACTIVITY_ERROR = "At least one activity needs to be enabled"
STARTING_ACTIVITY_ERROR = "starting activity must also be enabled"

RESPECT_SAFE_ERROR = re.compile(
    r"requires (\d+) progression Respect items, but the selected options "
    r"provide only (\d+) Respect-safe locations"
)
REQUIRED_ITEMS_ERROR = re.compile(
    r"requires (\d+) Respect and unlock items, but only has (\d+) available locations"
)


def sample_options(rng: random.Random) -> dict[str, Any]:
    # A shared density makes "almost everything off" and "almost everything
    # on" configs common instead of vanishingly rare.
    density = rng.choice((0.0, 0.1, 0.3, 0.5, 0.7, 0.9, 1.0, 1.0))
    options: dict[str, Any] = {
        name: int(rng.random() < density)
        for name in STARTING_ACTIVITY_INCLUDE_OPTIONS.values()
    }

    arcs = sorted(arc for arc in ARCS if rng.random() < 0.5)
    if not arcs and rng.random() < 0.9:
        arcs = [rng.choice(ARCS)]

    enabled = enabled_starting_activities(options)
    roll = rng.random()
    if roll < 0.4:
        starting_activity: int | str = "random"
    elif roll < 0.8 and enabled:
        starting_activity = rng.choice(enabled)
    else:
        starting_activity = rng.choice(list(STARTING_ACTIVITY_INCLUDE_OPTIONS))

    options |= {name: int(rng.random() < 0.5) for name in TOGGLES}

    if rng.random() < 0.1:
        options |= dict.fromkeys(FILLER_WEIGHT_CATEGORIES, 0)
    else:
        options |= {
            name: 0 if rng.random() < 0.3 else sample_range(rng, 0, 100)
            for name in FILLER_WEIGHT_CATEGORIES
        }

    return options | {
        "required_gang_arcs": arcs,
        "starting_activity": starting_activity,
        "style_level_location_count": sample_range(rng, 0, 10),
        "trap_chance": sample_range(rng, 0, 100),
        "bonus_respect_percentage": sample_range(rng, 0, 100),
    }


def enabled_starting_activities(options: dict[str, Any]) -> list[int]:
    return [
        value
        for value, include_option in STARTING_ACTIVITY_INCLUDE_OPTIONS.items()
        if options[include_option]
    ]


def traps_enabled(options: dict[str, Any]) -> bool:
    return bool(
        options["include_weather_trap_items"] or options["include_notoriety_trap_items"]
    )


def expected_option_error(options: dict[str, Any]) -> str | None:
    """The error these options must be rejected with, in generation order.

    The Respect errors are left out: predicting them means recounting the
    locations, so their claims are checked against the world instead.
    """
    weight_total = sum(options[name] for name in FILLER_WEIGHT_CATEGORIES)
    if weight_total == 0 and (
        options["trap_chance"] < 100 or not traps_enabled(options)
    ):
        return FILLER_WEIGHT_ERROR
    if not options["required_gang_arcs"]:
        return NO_ARCS_ERROR

    enabled = enabled_starting_activities(options)
    if not enabled:
        return NO_ACTIVITY_ERROR
    if options["starting_activity"] != "random" and (
        options["starting_activity"] not in enabled
    ):
        return STARTING_ACTIVITY_ERROR
    return None


class TestGenerationFuzz(SR2TestBase):
    """Random options must either be rejected for a known reason or generate a
    world that fills and can be beaten."""

    auto_construct = False
    run_default_tests = False

    def test_random_options(self) -> None:
        for case, rng in fuzz_cases("generation"):
            options = sample_options(rng)
            seed = rng.getrandbits(32)
            with self.subTest(case=case, seed=seed, options=options):
                self.run_case(options, seed)

    def run_case(self, options: dict[str, Any], seed: int) -> None:
        self.options = options
        expected_error = expected_option_error(options)

        try:
            self.world_setup(seed)
        except OptionError as error:
            if expected_error is not None:
                self.assertIn(expected_error, str(error))
            else:
                self.assert_respect_error_is_true(str(error))
            return

        self.assertIsNone(
            expected_error, f"Generation should have failed with {expected_error!r}"
        )
        self.assert_item_pool(options)
        self.assert_style_levels(options)
        self.assert_slot_data()
        self.assert_fills_and_is_beatable()

    def assert_respect_error_is_true(self, message: str) -> None:
        unfilled = self.multiworld.get_unfilled_locations(self.player)

        if match := RESPECT_SAFE_ERROR.search(message):
            required, available = map(int, match.groups())
            respect_safe = [
                location
                for location in unfilled
                if getattr(location, "respect_safe", False)
                and location.progress_type != LocationProgressType.EXCLUDED
            ]
            self.assertEqual(get_required_respect_count(self.world), required)
            self.assertEqual(len(respect_safe), available)
            self.assertGreater(required, available)
        elif match := REQUIRED_ITEMS_ERROR.search(message):
            required, available = map(int, match.groups())
            self.assertEqual(len(unfilled), available)
            self.assertGreater(required, available)
        else:
            self.fail(f"Unexpected option error: {message}")

    def pool_counts(self) -> Counter[str]:
        return Counter(
            item.name for item in self.multiworld.itempool if item.player == self.player
        )

    def assert_item_pool(self, options: dict[str, Any]) -> None:
        pool = self.pool_counts()
        unfilled = self.multiworld.get_unfilled_locations(self.player)
        self.assertEqual(len(unfilled), pool.total())

        required_respect = get_required_respect_count(self.world)
        bonus_percentage = options["bonus_respect_percentage"]
        expected_bonus = round(required_respect * bonus_percentage / 100)
        if bonus_percentage > 0:
            expected_bonus = max(1, expected_bonus)
        self.assertEqual(required_respect, pool[RESPECT_ITEM_NAME])
        self.assertEqual(expected_bonus, pool[BONUS_RESPECT_ITEM_NAME])

        # The starting pass sits on Appointed Defender, every other enabled
        # pass is in the pool exactly once.
        enabled = enabled_starting_activities(options)
        starting_item = self.world.get_location(get_mission_by_key("tss02").name).item
        enabled_passes = {ACTIVITY_UNLOCK_ITEMS_SETTINGS_MAPPING[v] for v in enabled}
        self.assertIn(starting_item.name, enabled_passes)
        if options["starting_activity"] != "random":
            self.assertEqual(
                ACTIVITY_UNLOCK_ITEMS_SETTINGS_MAPPING[options["starting_activity"]],
                starting_item.name,
            )
        for item_name in ACTIVITY_UNLOCK_ITEMS_SETTINGS_MAPPING.values():
            expected = int(
                item_name in enabled_passes and item_name != starting_item.name
            )
            self.assertEqual(expected, pool[item_name], item_name)

        # Goal-only optional missions still get their location and unlock item.
        location_names = {location.name for location in self.world.get_locations()}
        for location, unlock_item, include_option in (
            (ULTOR_SECRET_MISSION, EM01_UNLOCK_ITEM, "include_secret_mission"),
            (
                STILWATER_CAVERNS_STRONGHOLD,
                TSS_SH_CAVERNS_UNLOCK_ITEM,
                "include_stilwater_caverns",
            ),
        ):
            included = bool(
                options[include_option] or options[f"{include_option}_as_goal"]
            )
            self.assertEqual(included, location.name in location_names)
            self.assertEqual(int(included), pool[unlock_item])

        for name in USEFUL_UNLOCKABLES + FILLER_UNLOCKABLES:
            self.assertLessEqual(pool[name], 1, name)

        forbidden: set[str] = set()
        if not options["include_weather_trap_items"]:
            forbidden.update(TRAP_CHEATS)
        if not options["include_notoriety_trap_items"]:
            forbidden.update(TRAP_NOTORIETY_ITEMS)
        if options["trap_chance"] == 0:
            forbidden.update(TRAP_ITEMS)
        if options["trap_chance"] == 100 and traps_enabled(options):
            forbidden.update(MONEY_ITEM_NAMES, WEAPON_ITEM_NAMES, CHEAT_ITEMS)
        for weight_option, names in FILLER_WEIGHT_CATEGORIES.items():
            if options[weight_option] == 0:
                forbidden.update(names)
        self.assertEqual(set(), forbidden & set(pool))

    def assert_style_levels(self, options: dict[str, Any]) -> None:
        style_locations = [
            location
            for location in self.world.get_locations()
            if location.name.startswith("Style Level - ")
        ]
        self.assertEqual(options["style_level_location_count"], len(style_locations))

        priority_levels = []
        for location in style_locations:
            level = int(location.name.rsplit(" ", 1)[1])
            excluded = location.progress_type == LocationProgressType.EXCLUDED
            should_exclude = (
                level >= 8
                and not options["allow_progression_items_on_high_style_level"]
            )
            self.assertEqual(should_exclude, excluded, location.name)
            if location.progress_type == LocationProgressType.PRIORITY:
                priority_levels.append(level)

        self.assertLessEqual(len(priority_levels), 1)
        self.assertTrue(all(4 <= level <= 6 for level in priority_levels))

    def assert_slot_data(self) -> None:
        slot_data = self.world.fill_slot_data()
        locations = list(self.world.get_locations())
        names = [location.name for location in locations]
        addresses = {location.address for location in locations}

        self.assertLessEqual(set(slot_data["goal_locations"]), addresses)
        self.assertTrue(slot_data["goal_locations"])

        def has_location(predicate) -> bool:
            return any(predicate(name) for name in names)

        progression = slot_data["enabled_progression"]
        self.assertEqual(
            progression["activities"], has_location(lambda n: ") - Level " in n)
        )
        self.assertEqual(
            progression["chop_shop"],
            has_location(lambda n: n.startswith("Chop Shop (")),
        )
        self.assertEqual(
            progression["hitman"], has_location(lambda n: n.startswith("Hitman ("))
        )
        self.assertEqual(
            progression["cds"], has_location(lambda n: n.startswith("CD #"))
        )
        self.assertEqual(
            progression["tags"], has_location(lambda n: n.startswith("Tag #"))
        )
        self.assertEqual(
            progression["races"], has_location(lambda n: n.endswith(" Medal"))
        )
        self.assertEqual(
            progression["stunt_jumps"],
            has_location(lambda n: n.startswith("Stunt Jump #")),
        )
        for category, prefix in (
            ("taxi", "Taxi Diversion - "),
            ("paramedic", "Paramedic Diversion - "),
            ("firefighter", "Firefighter Diversion - "),
            ("towing", "Towing Diversion - "),
            ("hoing", "Ho-ing Diversion - "),
        ):
            self.assertEqual(
                progression[category],
                has_location(lambda n, prefix=prefix: n.startswith(prefix)),
            )
        self.assertEqual(
            progression["style_level"],
            has_location(lambda n: n.startswith("Style Level - ")),
        )

        player_items = {
            item.name
            for item in self.multiworld.get_items()
            if item.player == self.player
        }
        self.assertLessEqual(set(slot_data["persistent_items"]), player_items)
        self.assertLessEqual(set(slot_data["managed_unlockables"]), player_items)
        self.assertLessEqual(set(slot_data["managed_cheats"]), player_items)

    def assert_fills_and_is_beatable(self) -> None:
        all_state = self.multiworld.get_all_state()
        unreachable = [
            location.name
            for location in self.multiworld.get_locations(self.player)
            if not location.can_reach(all_state)
        ]
        self.assertEqual([], unreachable)
        self.assertTrue(self.multiworld.can_beat_game(all_state))

        distribute_items_restrictive(self.multiworld)

        self.assertEqual([], self.multiworld.get_unfilled_locations(self.player))
        self.assertTrue(self.multiworld.fulfills_accessibility())
        for location in self.multiworld.get_filled_locations(self.player):
            item = location.item
            if item.name == RESPECT_ITEM_NAME and item.player == self.player:
                self.assertTrue(location.respect_safe, location.name)
