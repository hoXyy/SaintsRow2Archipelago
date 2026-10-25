import unittest

from ..activities import ACTIVITY_LEVEL_IDS
from ..collectibles import CD_IDS, STUNT_JUMP_IDS, STYLE_LEVEL_IDS, TAGS_IDS
from ..items import (
    DEFAULT_ITEM_CLASSIFICATION,
    ITEM_NAME_TO_ID,
    LEVEL_ACTIVITY_UNLOCK_ITEMS,
    PERSISTENT_ACTIVITY_UNLOCK_ITEMS,
)
from ..items_list import (
    CHEAT_ITEMS,
    FILLER_UNLOCKABLES,
    MONEY_ITEM_NAMES,
    TRAP_ITEMS,
    USEFUL_UNLOCKABLES,
    WEAPON_ITEM_NAMES,
)
from ..locations import LOCATION_NAME_TO_ID
from ..missions import ALL_MISSIONS, MISSION_CHAINS, get_mission_by_key

# Each block of ids starts at a fixed base, so a block that grows past the
# next base would shift ids that existing seeds rely on.
ITEM_ID_BLOCKS = (
    ("weapons", WEAPON_ITEM_NAMES, 100, 200),
    ("useful unlockables", USEFUL_UNLOCKABLES, 200, 300),
    ("filler unlockables", FILLER_UNLOCKABLES, 300, 400),
    ("cheats", CHEAT_ITEMS, 400, 500),
    ("traps", TRAP_ITEMS, 500, 550),
    ("money", MONEY_ITEM_NAMES, 550, 600),
    (
        "unlock passes",
        LEVEL_ACTIVITY_UNLOCK_ITEMS + PERSISTENT_ACTIVITY_UNLOCK_ITEMS,
        600,
        700,
    ),
)

LOCATION_ID_BLOCKS = (
    ("missions", {mission.id for mission in ALL_MISSIONS}, 1, 100),
    ("activities", set(ACTIVITY_LEVEL_IDS.values()), 100, 500),
    ("CDs", set(CD_IDS.values()), 500, 600),
    ("style levels", set(STYLE_LEVEL_IDS.values()), 600, 650),
    ("tags", set(TAGS_IDS.values()), 650, 700),
    ("stunt jumps", set(STUNT_JUMP_IDS.values()), 700, 800),
)


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

    def test_item_id_blocks_stay_in_their_range(self) -> None:
        for block, names, start, end in ITEM_ID_BLOCKS:
            with self.subTest(block=block):
                ids = [ITEM_NAME_TO_ID[name] for name in names]
                self.assertGreaterEqual(min(ids), start)
                self.assertLess(max(ids), end)

    def test_location_id_blocks_stay_in_their_range(self) -> None:
        for block, ids, start, end in LOCATION_ID_BLOCKS:
            with self.subTest(block=block):
                self.assertGreaterEqual(min(ids), start)
                self.assertLess(max(ids), end)
