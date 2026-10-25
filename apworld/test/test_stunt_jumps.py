from BaseClasses import LocationProgressType

from ..collectibles import MISSABLE_STUNT_JUMP_MAPPING, STUNT_JUMP_MAPPING
from ..items import STUNT_JUMPS_UNLOCK_ITEM
from ..missions import get_mission_by_key
from ..options import RONIN_ARC_NAME, ULTOR_EPILOGUE_ARC_NAME
from .bases import SR2TestBase


class TestStuntJumpsEnabled(SR2TestBase):
    run_default_tests = False
    options = {"include_stunt_jumps": 1}

    def test_all_stunt_jumps_exist(self):
        for jump in STUNT_JUMP_MAPPING.values():
            with self.subTest(jump=jump):
                try:
                    self.world.get_location(jump)
                except KeyError:
                    self.fail(
                        f"Stunt jump {jump} does not exist, even though it should!"
                    )

    def test_missable_stunt_jumps_dont_exist_without_ultor_arc(self):
        for jump in MISSABLE_STUNT_JUMP_MAPPING.values():
            self.assertRaises(KeyError, self.world.get_location, jump)

    def test_all_stunt_jumps_require_the_unlock_item(self):
        state = self.get_fresh_state()
        unlock_item = self.get_item_by_name(STUNT_JUMPS_UNLOCK_ITEM)

        for jump in STUNT_JUMP_MAPPING.values():
            with self.subTest(jump=jump):
                location = self.world.get_location(jump)
                self.assertFalse(location.can_reach(state))

        state.collect(unlock_item, prevent_sweep=True)

        for jump in STUNT_JUMP_MAPPING.values():
            with self.subTest(jump=jump):
                location = self.world.get_location(jump)
                self.assertTrue(location.can_reach(state))


class TestStuntJumpsWithUltorArc(SR2TestBase):
    run_default_tests = False
    options = {
        "include_stunt_jumps": 1,
        "required_gang_arcs": {RONIN_ARC_NAME, ULTOR_EPILOGUE_ARC_NAME},
    }

    def test_missable_stunt_jumps_exist(self):
        for jump in MISSABLE_STUNT_JUMP_MAPPING.values():
            with self.subTest(jump=jump):
                try:
                    self.world.get_location(jump)
                except KeyError:
                    self.fail(
                        f"Stunt jump {jump} does not exist, even though it should!"
                    )

    def test_missable_stunt_jumps_are_excluded(self):
        for jump in MISSABLE_STUNT_JUMP_MAPPING.values():
            with self.subTest(jump=jump):
                location = self.world.get_location(jump)
                self.assertEqual(LocationProgressType.EXCLUDED, location.progress_type)

    def test_missable_stunt_jumps_require_the_unlock_item(self):
        state = self.get_fresh_state()
        unlock_item = self.get_item_by_name(STUNT_JUMPS_UNLOCK_ITEM)

        for jump in MISSABLE_STUNT_JUMP_MAPPING.values():
            with self.subTest(jump=jump):
                location = self.world.get_location(jump)
                self.assertFalse(location.can_reach(state))

        state.collect(unlock_item, prevent_sweep=True)

        for jump in MISSABLE_STUNT_JUMP_MAPPING.values():
            with self.subTest(jump=jump):
                location = self.world.get_location(jump)
                self.assertTrue(location.can_reach(state))

    def test_pyramid_scheme_requires_the_unlock_item(self):
        pyramid_scheme = self.world.get_location(get_mission_by_key("ep02").name)

        self.collect_all_but(STUNT_JUMPS_UNLOCK_ITEM)
        self.assertFalse(pyramid_scheme.can_reach(self.multiworld.state))

        self.collect_by_name(STUNT_JUMPS_UNLOCK_ITEM)
        self.assertTrue(pyramid_scheme.can_reach(self.multiworld.state))


class TestStuntJumpsDisabled(SR2TestBase):
    run_default_tests = False
    options = {
        "include_stunt_jumps": 0,
        "required_gang_arcs": {RONIN_ARC_NAME, ULTOR_EPILOGUE_ARC_NAME},
    }

    def test_all_stunt_jumps_dont_exist(self):
        for jump in (STUNT_JUMP_MAPPING | MISSABLE_STUNT_JUMP_MAPPING).values():
            self.assertRaises(KeyError, self.world.get_location, jump)

    def test_pyramid_scheme_doesnt_require_the_unlock_item(self):
        pyramid_scheme = self.world.get_location(get_mission_by_key("ep02").name)

        self.collect_all_but(STUNT_JUMPS_UNLOCK_ITEM)
        self.assertTrue(pyramid_scheme.can_reach(self.multiworld.state))
