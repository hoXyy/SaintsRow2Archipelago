from ..collectibles import CD_MAPPING, PROGRESSIVE_CD_LOCATIONS
from ..items import CD_UNLOCK_ITEM
from .bases import SR2TestBase


class TestCDsEnabled(SR2TestBase):
    run_default_tests = False
    options = {"include_cds": 1}

    def test_all_cds_exist(self):
        for cd in CD_MAPPING.values():
            with self.subTest(cd=cd):
                try:
                    self.world.get_location(cd)
                except KeyError:
                    self.fail(f"CD {cd} does not exist, even though it should!")

    def test_all_cds_require_the_unlock_item(self):
        state = self.get_fresh_state()
        unlock_item = self.get_item_by_name(CD_UNLOCK_ITEM)

        for cd in CD_MAPPING.values():
            with self.subTest(cd=cd):
                location = self.world.get_location(cd)
                self.assertFalse(location.can_reach(state))

        state.collect(unlock_item, prevent_sweep=True)

        for cd in CD_MAPPING.values():
            with self.subTest(cd=cd):
                location = self.world.get_location(cd)
                self.assertTrue(location.can_reach(state))


class TestCDsDisabled(SR2TestBase):
    run_default_tests = False
    options = {"include_cds": 0}

    def test_all_cds_dont_exist(self):
        for cd in CD_MAPPING.values():
            self.assertRaises(KeyError, self.world.get_location, cd)


class TestCDsProgressive(SR2TestBase):
    run_default_tests = False
    options = {"include_cds": 2, "progressive_cd_count": 20}

    def test_progressive_cds_up_to_the_count_exist(self):
        for cd in PROGRESSIVE_CD_LOCATIONS[:20]:
            with self.subTest(cd=cd):
                try:
                    self.world.get_location(cd)
                except KeyError:
                    self.fail(f"{cd} does not exist, even though it should!")

    def test_progressive_cds_past_the_count_dont_exist(self):
        for cd in PROGRESSIVE_CD_LOCATIONS[20:]:
            self.assertRaises(KeyError, self.world.get_location, cd)

    def test_individual_cds_dont_exist(self):
        for cd in CD_MAPPING.values():
            self.assertRaises(KeyError, self.world.get_location, cd)

    def test_progressive_cds_require_the_unlock_item(self):
        state = self.get_fresh_state()
        unlock_item = self.get_item_by_name(CD_UNLOCK_ITEM)

        for cd in PROGRESSIVE_CD_LOCATIONS[:20]:
            with self.subTest(cd=cd):
                location = self.world.get_location(cd)
                self.assertFalse(location.can_reach(state))

        state.collect(unlock_item, prevent_sweep=True)

        for cd in PROGRESSIVE_CD_LOCATIONS[:20]:
            with self.subTest(cd=cd):
                location = self.world.get_location(cd)
                self.assertTrue(location.can_reach(state))
