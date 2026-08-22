from .bases import SR2TestBase
from ..items_list import (
    USEFUL_UNLOCKABLES,
    FILLER_UNLOCKABLES,
    CHEAT_ITEMS,
    WEAPON_ITEM_NAMES,
    MONEY_ITEM_NAMES,
    TRAP_CHEATS,
    TRAP_ITEMS,
)
from ..missions import get_mission_by_key, ULTOR_SECRET_MISSION
from ..options import (
    RONIN_ARC_NAME,
    BROTHERHOOD_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)


class TestSlotData(SR2TestBase):
    auto_construct = False
    run_default_tests = False

    def test_protocol_version(self) -> None:
        self.world_setup()
        slot_data = self.world.fill_slot_data()

        self.assertEqual(slot_data["protocol"], 3)

    def test_default_enabled_progression(self) -> None:
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_enabled_progression = slot_data["enabled_progression"]

        expected_enabled_progression = {
            "missions": True,
            "activities": True,
            "cds": True,
            "chop_shop": True,
            "hitman": True,
            "races": True,
        }

        self.assertEqual(slot_enabled_progression, expected_enabled_progression)

    def test_enabled_progression_no_cds(self) -> None:
        self.options = {"include_cds": 0}
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_enabled_progression = slot_data["enabled_progression"]

        expected_enabled_progression = {
            "missions": True,
            "activities": True,
            "cds": False,
            "chop_shop": True,
            "hitman": True,
            "races": True,
        }

        self.assertEqual(slot_enabled_progression, expected_enabled_progression)

    def test_enabled_progression_no_races(self) -> None:
        self.options = {"include_races": 0}
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_enabled_progression = slot_data["enabled_progression"]

        expected_enabled_progression = {
            "missions": True,
            "activities": True,
            "cds": True,
            "chop_shop": True,
            "hitman": True,
            "races": False,
        }

        self.assertEqual(slot_enabled_progression, expected_enabled_progression)

    def test_enabled_progression_no_chop_shop(self) -> None:
        self.options = {"include_chop_shop": 0}
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_enabled_progression = slot_data["enabled_progression"]

        expected_enabled_progression = {
            "missions": True,
            "activities": True,
            "cds": True,
            "chop_shop": False,
            "hitman": True,
            "races": True,
        }

        self.assertEqual(slot_enabled_progression, expected_enabled_progression)

    def test_enabled_progression_no_hitman(self) -> None:
        self.options = {"include_hitman": 0}
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_enabled_progression = slot_data["enabled_progression"]

        expected_enabled_progression = {
            "missions": True,
            "activities": True,
            "cds": True,
            "chop_shop": True,
            "hitman": False,
            "races": True,
        }

        self.assertEqual(slot_enabled_progression, expected_enabled_progression)

    def test_enabled_progression_no_activities(self) -> None:
        self.options = {
            "include_crowd_control": 0,
            "include_demo_derby": 0,
            "include_drug_trafficking": 0,
            "include_escort": 0,
            "include_fight_club": 0,
            "include_fraud": 0,
            "include_fuzz": 0,
            "include_heli_assault": 0,
            "include_mayhem": 0,
            "include_sewage": 0,
            "include_snatch": 0,
            "include_torch": 0,
        }
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_enabled_progression = slot_data["enabled_progression"]

        expected_enabled_progression = {
            "missions": True,
            "activities": False,
            "cds": True,
            "chop_shop": True,
            "hitman": True,
            "races": True,
        }

        self.assertEqual(slot_enabled_progression, expected_enabled_progression)

    def test_default_goal_locations(self) -> None:
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_goal_locations = slot_data["goal_locations"]
        expected_goal_locations = [get_mission_by_key("rn11").id]

        self.assertEqual(slot_goal_locations, expected_goal_locations)

    def test_all_arcs_goal_locations(self) -> None:
        self.options = {
            "required_gang_arcs": {
                RONIN_ARC_NAME,
                BROTHERHOOD_ARC_NAME,
                SAMEDI_ARC_NAME,
                ULTOR_EPILOGUE_ARC_NAME,
            }
        }
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_goal_locations = slot_data["goal_locations"]
        expected_goal_locations = [
            get_mission_by_key("rn11").id,
            get_mission_by_key("ss11").id,
            get_mission_by_key("bh11").id,
            get_mission_by_key("ep04").id,
        ]

        self.assertEqual(slot_goal_locations, expected_goal_locations)

    def test_secret_mission_goal_location(self) -> None:
        self.options = {
            "include_secret_mission": 1,
            "include_secret_mission_as_goal": 1,
        }
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_goal_locations = slot_data["goal_locations"]
        expected_goal_locations = [
            get_mission_by_key("rn11").id,
            ULTOR_SECRET_MISSION.id,
        ]

        self.assertEqual(slot_goal_locations, expected_goal_locations)

    def test_features_exclusive_respect(self) -> None:
        self.world_setup()
        slot_data = self.world.fill_slot_data()
        slot_exclusive_respect = slot_data["features"]["exclusive_respect"]

        self.assertTrue(slot_exclusive_respect)

    def test_managed_items_match_generated_pool(self) -> None:
        self.world_setup()

        slot_data = self.world.fill_slot_data()
        generated_names = {
            item.name
            for item in self.multiworld.get_items()
            if item.player == self.player and item.code is not None
        }

        expected_unlockables = sorted(
            generated_names & (set(USEFUL_UNLOCKABLES) | set(FILLER_UNLOCKABLES))
        )
        expected_cheats = sorted(
            generated_names
            & (
                set(CHEAT_ITEMS)
                | set(WEAPON_ITEM_NAMES)
                | set(MONEY_ITEM_NAMES)
                | set(TRAP_CHEATS)
            )
        )

        self.assertEqual(
            expected_unlockables,
            slot_data["managed_unlockables"],
        )
        self.assertEqual(
            expected_cheats,
            slot_data["managed_cheats"],
        )

    def test_feature_defaults(self) -> None:
        self.world_setup()
        features = self.world.fill_slot_data()["features"]

        self.assertTrue(features["exclusive_respect"])
        self.assertTrue(features["block_vanilla_unlockables"])
        self.assertIsInstance(features["notoriety_traps"], bool)

    def test_notoriety_trap_feature_matches_pool(self) -> None:
        self.world_setup()

        generated_names = {
            item.name
            for item in self.multiworld.get_items()
            if item.player == self.player
        }

        expected = bool(generated_names & set(TRAP_ITEMS))
        actual = self.world.fill_slot_data()["features"]["notoriety_traps"]

        self.assertEqual(expected, actual)
