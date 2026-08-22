from .bases import SR2TestBase
from ..collectibles import CD_MAPPING


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

    def test_all_cds_are_reachable(self):
        for cd in CD_MAPPING.values():
            with self.subTest(cd=cd):
                location = self.world.get_location(cd)
                self.assertTrue(location.can_reach(self.multiworld.state))


class TestCDsDisabled(SR2TestBase):
    run_default_tests = False
    options = {"include_cds": 0}

    def test_all_cds_dont_exist(self):
        for cd in CD_MAPPING.values():
            self.assertRaises(KeyError, self.world.get_location, cd)
