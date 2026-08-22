from collections import Counter

from .bases import SR2TestBase
from ..items import RESPECT_ITEM_NAME, BONUS_RESPECT_ITEM_NAME
from ..items_list import (
    TRAP_ITEMS,
    FILLER_UNLOCKABLES,
    WEAPON_ITEM_NAMES,
    CHEAT_ITEMS,
    MONEY_ITEM_NAMES,
    USEFUL_UNLOCKABLES,
)
from ..missions import get_required_respect_count
from ..options import (
    RONIN_ARC_NAME,
    BROTHERHOOD_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)


class TestFullGameItemPool(SR2TestBase):
    run_default_tests = False
    options = {
        "required_gang_arcs": {
            RONIN_ARC_NAME,
            BROTHERHOOD_ARC_NAME,
            SAMEDI_ARC_NAME,
            ULTOR_EPILOGUE_ARC_NAME,
        }
    }

    def test_item_pool_matches_unfilled_locations(self) -> None:
        unfilled = self.multiworld.get_unfilled_locations(self.player)
        player_items = [
            item for item in self.multiworld.itempool if item.player == self.player
        ]

        self.assertEqual(len(unfilled), len(player_items))

    def test_required_respect_count(self) -> None:
        counts = Counter(item.name for item in self.multiworld.itempool)

        self.assertEqual(
            get_required_respect_count(self.world),
            counts[RESPECT_ITEM_NAME],
        )


class TestRoninOnlyItemPool(SR2TestBase):
    run_default_tests = False
    options = {"required_gang_arcs": {RONIN_ARC_NAME}}

    def test_required_respect_count(self) -> None:
        counts = Counter(item.name for item in self.multiworld.itempool)

        self.assertEqual(
            get_required_respect_count(self.world),
            counts[RESPECT_ITEM_NAME],
        )


class TestSamediOnlyItemPool(SR2TestBase):
    run_default_tests = False
    options = {"required_gang_arcs": {SAMEDI_ARC_NAME}}

    def test_required_respect_count(self) -> None:
        counts = Counter(item.name for item in self.multiworld.itempool)

        self.assertEqual(
            get_required_respect_count(self.world),
            counts[RESPECT_ITEM_NAME],
        )


class TestBrotherhoodOnlyItemPool(SR2TestBase):
    run_default_tests = False
    options = {"required_gang_arcs": {BROTHERHOOD_ARC_NAME}}

    def test_required_respect_count(self) -> None:
        counts = Counter(item.name for item in self.multiworld.itempool)

        self.assertEqual(
            get_required_respect_count(self.world),
            counts[RESPECT_ITEM_NAME],
        )


class TestNoTrapsInItemPool(SR2TestBase):
    run_default_tests = False
    options = {"trap_chance": 0}

    def test_no_traps_in_item_pool(self) -> None:
        counts = Counter(item.name for item in self.multiworld.itempool)
        for item in TRAP_ITEMS:
            with self.subTest(item=item):
                self.assertEqual(counts[item], 0)


class TestAlwaysTrapAsFillerItem(SR2TestBase):
    run_default_tests = False
    options = {"trap_chance": 100}

    def test_always_trap_as_filler_item(self) -> None:
        for generation in range(200):
            item = self.world.get_filler_item_name()
            with self.subTest(generation=generation, item=item):
                self.assertTrue(item in TRAP_ITEMS)


class TestEmptyBonusRespectOption(SR2TestBase):
    run_default_tests = False
    options = {"bonus_respect_percentage": 0}

    def test_no_bonus_respect_in_pool(self) -> None:
        item_counts = Counter(item.name for item in self.multiworld.itempool)
        self.assertEqual(0, item_counts[BONUS_RESPECT_ITEM_NAME])


class Test1PercentBonusRespectOption(SR2TestBase):
    run_default_tests = False
    options = {"bonus_respect_percentage": 1}

    def test_bonus_respect_amount_in_pool(self) -> None:
        item_counts = Counter(item.name for item in self.multiworld.itempool)
        progression_respect_count = get_required_respect_count(self.world)
        bonus_respect_count = max(
            round(
                progression_respect_count
                * self.world.options.bonus_respect_percentage.value
                / 100
            ),
            1,
        )

        self.assertEqual(1, bonus_respect_count)
        self.assertEqual(bonus_respect_count, item_counts[BONUS_RESPECT_ITEM_NAME])


class Test100PercentBonusRespectOption(SR2TestBase):
    run_default_tests = False
    options = {"bonus_respect_percentage": 100}

    def test_bonus_respect_amount_in_pool(self) -> None:
        item_counts = Counter(item.name for item in self.multiworld.itempool)
        progression_respect_count = get_required_respect_count(self.world)
        bonus_respect_count = round(
            progression_respect_count
            * self.world.options.bonus_respect_percentage.value
            / 100
        )

        self.assertEqual(bonus_respect_count, item_counts[BONUS_RESPECT_ITEM_NAME])
        self.assertEqual(progression_respect_count, item_counts[RESPECT_ITEM_NAME])


class TestItemPlacementRules(SR2TestBase):
    options = {
        "required_gang_arcs": {
            SAMEDI_ARC_NAME,
            RONIN_ARC_NAME,
            BROTHERHOOD_ARC_NAME,
            ULTOR_EPILOGUE_ARC_NAME,
        }
    }
    run_default_tests = False

    def test_respect_item_placement_rules(self) -> None:
        locations = self.multiworld.get_unfilled_locations()

        for location in locations:
            with self.subTest(location=location):
                respect_item = self.world.create_item(RESPECT_ITEM_NAME)

                if getattr(location, "respect_safe", True):
                    self.assertTrue(location.item_rule(respect_item))
                else:
                    self.assertFalse(location.item_rule(respect_item))

    def test_event_locations_are_locked(self) -> None:
        for location in self.multiworld.get_locations(self.player):
            if location.address is None:
                with self.subTest(location=location.name):
                    self.assertIsNotNone(location.item)
                    self.assertIsNone(location.item.code)


class TestItemClassification(SR2TestBase):
    run_default_tests = False

    def test_respect_is_progression(self) -> None:
        respect_item = self.world.create_item(RESPECT_ITEM_NAME)
        self.assertEqual(respect_item.advancement, True)

    def test_traps_are_traps(self) -> None:
        for trap in TRAP_ITEMS:
            with self.subTest(trap=trap):
                item = self.world.create_item(trap)
                self.assertEqual(item.trap, True)

    def test_filler_is_filler(self) -> None:
        for item in (
            FILLER_UNLOCKABLES + WEAPON_ITEM_NAMES + CHEAT_ITEMS + MONEY_ITEM_NAMES
        ):
            with self.subTest(item=item):
                item = self.world.create_item(item)
                self.assertEqual(item.filler, True)

    def test_useful_is_useful(self) -> None:
        for item in USEFUL_UNLOCKABLES + [BONUS_RESPECT_ITEM_NAME]:
            with self.subTest(item=item):
                item = self.world.create_item(item)
                self.assertEqual(item.useful, True)
