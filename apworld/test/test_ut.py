from types import SimpleNamespace
from unittest import TestCase
from unittest.mock import patch

from BaseClasses import CollectionState, MultiWorld

from .bases import SR2TestBase
from ..client import calculate_ut_respect
from ..items import (
    BONUS_RESPECT_ITEM_NAME,
    ITEM_NAME_TO_ID,
    RESPECT_ITEM_NAME,
)
from ..missions import (
    ALL_MISSIONS,
    RONIN_CHAIN,
    STILWATER_CAVERNS_STRONGHOLD,
    ULTOR_SECRET_MISSION,
    get_mission_by_key,
    get_mission_complete_event_name,
)
from ..options import (
    BROTHERHOOD_ARC_NAME,
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    SR2Options,
    StartingActivity,
)


class SR2UTTestBase(SR2TestBase):
    run_default_tests = False

    def world_setup(self, seed=None) -> None:
        with patch.object(MultiWorld, "generation_is_fake", True, create=True):
            super().world_setup(seed)

    def tracker_state(
        self,
        checked_locations: set[int],
        received_respect: int,
    ) -> CollectionState:
        spent_respect = sum(
            mission.required_respect
            for mission in ALL_MISSIONS
            if mission.id in checked_locations
        )
        self.world.ut_checked_locations = frozenset(checked_locations)
        self.world.ut_received_respect = received_respect
        self.world.ut_spent_respect = spent_respect

        state = CollectionState(self.multiworld)
        state.sweep_for_advancements(
            locations=[
                location
                for location in self.world.get_locations()
                if location.address is None
            ]
        )
        return state


class TestUTRespectCounts(TestCase):
    def test_counts_progression_and_bonus_respect(self) -> None:
        checked_locations = {
            get_mission_by_key("tss03").id,
            RONIN_CHAIN["strongholds"][0].id,
        }
        items_received = [
            SimpleNamespace(item=ITEM_NAME_TO_ID[RESPECT_ITEM_NAME]),
            SimpleNamespace(item=ITEM_NAME_TO_ID[RESPECT_ITEM_NAME]),
            SimpleNamespace(item=ITEM_NAME_TO_ID[BONUS_RESPECT_ITEM_NAME]),
            SimpleNamespace(item=999_999),
        ]

        received, spent = calculate_ut_respect(
            items_received,
            checked_locations,
        )

        self.assertEqual(3, received)
        self.assertEqual(2, spent)


class TestUTMissionAccess(SR2UTTestBase):
    options = {
        "required_gang_arcs": {
            RONIN_ARC_NAME,
            SAMEDI_ARC_NAME,
            BROTHERHOOD_ARC_NAME,
        },
        "include_secret_mission": 1,
        "include_stilwater_caverns": 1,
    }

    @staticmethod
    def intro_checked() -> set[int]:
        return {
            get_mission_by_key(key).id for key in ("tss01", "tss02", "tss03", "tss04")
        }

    def test_one_available_respect_opens_each_eligible_gang_mission(self) -> None:
        state = self.tracker_state(self.intro_checked(), received_respect=3)

        for key in ("rn01", "ss01", "bh01"):
            with self.subTest(mission=key):
                mission = get_mission_by_key(key)
                self.assertTrue(self.world.get_location(mission.name).can_reach(state))

    def test_checked_paid_mission_spends_respect_and_unlocks_its_event(self) -> None:
        ronin_one = get_mission_by_key("rn01")
        checked_locations = self.intro_checked() | {ronin_one.id}
        state = self.tracker_state(checked_locations, received_respect=3)

        ronin_one_event = self.world.get_location(
            get_mission_complete_event_name(ronin_one)
        )
        self.assertTrue(ronin_one_event.can_reach(state))

        for key in ("rn02", "ss01", "bh01"):
            with self.subTest(mission=key):
                mission = get_mission_by_key(key)
                self.assertFalse(self.world.get_location(mission.name).can_reach(state))

    def test_free_mission_chain_is_in_logic_before_locations_are_checked(self) -> None:
        state = self.tracker_state(set(), received_respect=0)

        for key in ("tss01", "tss02"):
            with self.subTest(mission=key):
                mission = get_mission_by_key(key)
                self.assertTrue(self.world.get_location(mission.name).can_reach(state))
                self.assertTrue(
                    self.world.get_location(
                        get_mission_complete_event_name(mission)
                    ).can_reach(state)
                )

        self.assertFalse(
            self.world.get_location(get_mission_by_key("tss03").name).can_reach(state)
        )

    def test_unchecked_predecessors_add_to_required_respect(self) -> None:
        one_respect = self.tracker_state(set(), received_respect=1)
        self.assertTrue(
            self.world.get_location(get_mission_by_key("tss03").name).can_reach(
                one_respect
            )
        )
        self.assertFalse(
            self.world.get_location(get_mission_by_key("tss04").name).can_reach(
                one_respect
            )
        )

        two_respect = self.tracker_state(set(), received_respect=2)
        self.assertTrue(
            self.world.get_location(get_mission_by_key("tss04").name).can_reach(
                two_respect
            )
        )

    def test_stronghold_uses_available_respect_and_checked_event(self) -> None:
        ronin_one = get_mission_by_key("rn01")
        stronghold = RONIN_CHAIN["strongholds"][0]
        checked_locations = self.intro_checked() | {ronin_one.id}

        available_state = self.tracker_state(
            checked_locations,
            received_respect=4,
        )
        self.assertTrue(
            self.world.get_location(stronghold.name).can_reach(available_state)
        )

        checked_state = self.tracker_state(
            checked_locations | {stronghold.id},
            received_respect=4,
        )
        self.assertEqual(0, self.world.ut_available_respect)
        self.assertTrue(
            self.world.get_location(
                get_mission_complete_event_name(stronghold)
            ).can_reach(checked_state)
        )

    def test_special_missions_and_events_use_tracker_respect(self) -> None:
        checked_locations = self.intro_checked()

        for mission in (ULTOR_SECRET_MISSION, STILWATER_CAVERNS_STRONGHOLD):
            available_state = self.tracker_state(
                checked_locations,
                received_respect=3,
            )
            with self.subTest(mission=mission.key, status="available"):
                self.assertTrue(
                    self.world.get_location(mission.name).can_reach(available_state)
                )
                self.assertTrue(
                    self.world.get_location(
                        get_mission_complete_event_name(mission)
                    ).can_reach(available_state)
                )

            checked_state = self.tracker_state(
                checked_locations | {mission.id},
                received_respect=3,
            )
            with self.subTest(mission=mission.key, status="checked"):
                self.assertTrue(
                    self.world.get_location(
                        get_mission_complete_event_name(mission)
                    ).can_reach(checked_state)
                )


class TestUTSlotData(SR2TestBase):
    auto_construct = False
    run_default_tests = False

    def test_options_round_trip_through_regeneration_passthrough(self) -> None:
        original_options = {
            "required_gang_arcs": {RONIN_ARC_NAME, SAMEDI_ARC_NAME},
            "starting_activity": StartingActivity.option_tags,
            "include_secret_mission": 1,
            "include_stilwater_caverns": 1,
            "include_cds": 0,
            "bonus_respect_percentage": 37,
        }
        self.options = original_options
        self.world_setup(seed=12345)
        slot_data = self.world.fill_slot_data()

        self.assertEqual(
            set(SR2Options.__annotations__),
            set(slot_data["options"]),
        )

        self.options = {}
        with patch.object(
            MultiWorld,
            "re_gen_passthrough",
            {self.game: slot_data},
            create=True,
        ):
            self.world_setup(seed=54321)

        for option_name, expected in original_options.items():
            actual = getattr(self.world.options, option_name).value
            if isinstance(expected, set):
                self.assertEqual(expected, set(actual))
            else:
                self.assertEqual(expected, actual)
