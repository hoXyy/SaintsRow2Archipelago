from .bases import SR2TestBase
from ..collectibles import TAGS_MAPPING
from ..items import TAGS_UNLOCK_ITEM


class TestTagsEnabled(SR2TestBase):
    run_default_tests = False
    options = {"include_tags": 1}

    def test_all_tags_exist(self):
        for tag in TAGS_MAPPING.values():
            with self.subTest(tag=tag):
                try:
                    self.world.get_location(tag)
                except KeyError:
                    self.fail(f"Tag {tag} does not exist, even though it should!")

    def test_all_tags_require_the_unlock_item(self):
        state = self.get_fresh_state()
        unlock_item = self.get_item_by_name(TAGS_UNLOCK_ITEM)

        for tag in TAGS_MAPPING.values():
            with self.subTest(tag=tag):
                location = self.world.get_location(tag)
                self.assertFalse(location.can_reach(state))

        state.collect(unlock_item, prevent_sweep=True)

        for tag in TAGS_MAPPING.values():
            with self.subTest(tag=tag):
                location = self.world.get_location(tag)
                self.assertTrue(location.can_reach(state))


class TestTagsDisabled(SR2TestBase):
    run_default_tests = False
    options = {"include_tags": 0}

    def test_all_cds_dont_exist(self):
        for tag in TAGS_MAPPING.values():
            self.assertRaises(KeyError, self.world.get_location, tag)
