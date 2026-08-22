import unittest

from ..items import ITEM_NAME_TO_ID, DEFAULT_ITEM_CLASSIFICATION
from ..locations import LOCATION_NAME_TO_ID
from ..missions import MISSION_CHAINS, get_mission_by_key


class TestStaticData(unittest.TestCase):
    def test_item_tables_have_the_same_names(self) -> None:
        self.assertEqual(
            set(ITEM_NAME_TO_ID),
            set(DEFAULT_ITEM_CLASSIFICATION),
        )

    def test_item_ids_are_unique(self) -> None:
        ids = list(ITEM_NAME_TO_ID.values())
        self.assertEqual(len(ids), len(set(ids)))

    def test_location_ids_are_unique(self) -> None:
        ids = list(LOCATION_NAME_TO_ID.values())
        self.assertEqual(len(ids), len(set(ids)))

    def test_stronghold_unlock_keys_exist(self) -> None:
        for chain in MISSION_CHAINS:
            for stronghold in chain["strongholds"]:
                with self.subTest(stronghold=stronghold.key):
                    get_mission_by_key(stronghold.unlocked_by)
