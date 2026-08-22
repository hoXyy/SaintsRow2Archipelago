from .bases import SR2TestBase
from ..activities import (
    ACTIVITIES_LEVEL_BASED,
    CHOP_SHOP_LISTS,
    HITMAN_LISTS,
    RACES,
    MEDAL_STRINGS,
)
from ..missions import get_mission_complete_item_name, get_mission_by_key


class TestActivityLocations(SR2TestBase):
    run_default_tests = False

    def test_activity_levels_require_previous_completion(self) -> None:
        for activity, instances in ACTIVITIES_LEVEL_BASED.items():
            districts = [
                district for instance in instances for district in instance.values()
            ]

            unlock_mission_key = "tss04" if activity == "Heli Assault" else "tss02"
            unlock_item = self.get_item_by_name(
                get_mission_complete_item_name(get_mission_by_key(unlock_mission_key))
            )

            for district in districts:
                with self.subTest(activity=activity, district=district):
                    state = self.get_fresh_state()

                    level_1_complete = self.get_item_by_name(
                        f"Item: {activity} ({district}) - Level 1 Complete"
                    )

                    level_1 = self.world.get_location(
                        f"{activity} ({district}) - Level 1"
                    )
                    level_2 = self.world.get_location(
                        f"{activity} ({district}) - Level 2"
                    )
                    level_2_event = self.world.get_location(
                        f"Event: {activity} ({district}) - Level 2 Complete"
                    )

                    self.assertFalse(level_1.access_rule(state))
                    self.assertFalse(level_2.access_rule(state))
                    self.assertFalse(level_2_event.access_rule(state))

                    state.collect(unlock_item, prevent_sweep=True)

                    self.assertTrue(level_1.access_rule(state))
                    self.assertFalse(level_2.access_rule(state))
                    self.assertFalse(level_2_event.access_rule(state))

                    state.collect(level_1_complete, prevent_sweep=True)

                    self.assertTrue(level_2.access_rule(state))
                    self.assertTrue(level_2_event.access_rule(state))

    def test_each_instance_has_six_locations_and_five_events(self) -> None:
        for activity in ACTIVITIES_LEVEL_BASED:
            districts = [
                value for d in ACTIVITIES_LEVEL_BASED[activity] for value in d.values()
            ]

            for district in districts:
                with self.subTest(activity=activity, district=district):
                    locations = [
                        candidate
                        for candidate in self.multiworld.get_locations(self.player)
                        if candidate.name.startswith(
                            f"{activity} ({district}) - Level "
                        )
                        and candidate.address is not None
                    ]

                    events = [
                        candidate
                        for candidate in self.multiworld.get_locations(self.player)
                        if candidate.name.startswith(
                            f"Event: {activity} ({district}) - Level "
                        )
                    ]

                    self.assertEqual(6, len(locations))
                    self.assertEqual(5, len(events))

    def test_activity_start_access_rules(self) -> None:
        for activity in ACTIVITIES_LEVEL_BASED:
            is_heli_assault = activity == "Heli Assault"

            districts = [
                value for d in ACTIVITIES_LEVEL_BASED[activity] for value in d.values()
            ]

            for district in districts:
                with self.subTest(activity=activity, district=district):
                    state = self.get_fresh_state()
                    lvl1_location = self.world.get_location(
                        f"{activity} ({district}) - Level 1"
                    )
                    complete_item = self.get_item_by_name(
                        get_mission_complete_item_name(
                            get_mission_by_key("tss04" if is_heli_assault else "tss02")
                        )
                    )

                    self.assertFalse(lvl1_location.can_reach(state))

                    state.collect(complete_item)

                    self.assertTrue(lvl1_location.can_reach(state))


class TestChopShopDisabled(SR2TestBase):
    run_default_tests = False
    options = {"include_chop_shop": 0}

    def test_chop_shop_disabled(self) -> None:
        for location, vehicles in CHOP_SHOP_LISTS.items():
            for vehicle_dict in vehicles:
                for vehicle in vehicle_dict.values():
                    name = f"Chop Shop ({location}) - {vehicle}"
                    with self.subTest(name=name):
                        self.assertRaises(KeyError, self.world.get_location, name)


class TestHitmanDisabled(SR2TestBase):
    run_default_tests = False
    options = {"include_hitman": 0}

    def test_hitman_disabled(self) -> None:
        for location, targets in HITMAN_LISTS.items():
            for target_dict in targets:
                for target in target_dict.values():
                    name = f"Hitman ({location}) - {target}"
                    with self.subTest(name=name):
                        self.assertRaises(KeyError, self.world.get_location, name)


class TestRacesDisabled(SR2TestBase):
    run_default_tests = False
    options = {"include_races": 0}

    def test_race_disabled(self) -> None:
        for race in RACES.values():
            for medal in MEDAL_STRINGS.values():
                with self.subTest(race=race, medal=medal):
                    self.assertRaises(
                        KeyError, self.world.get_location, f"{race} - {medal}"
                    )


class TestLevelBasedActivityDisabled(SR2TestBase):
    run_default_tests = False
    options = {"include_fraud": 0}

    def test_activity_has_no_events_and_checks(self) -> None:
        activity = "Insurance Fraud"
        districts = [
            value for d in ACTIVITIES_LEVEL_BASED[activity] for value in d.values()
        ]

        for district in districts:
            with self.subTest(activity=activity, district=district):
                locations = [
                    candidate
                    for candidate in self.multiworld.get_locations(self.player)
                    if candidate.name.startswith(f"{activity} ({district}) - Level ")
                    and candidate.address is not None
                ]

                events = [
                    candidate
                    for candidate in self.multiworld.get_locations(self.player)
                    if candidate.name.startswith(
                        f"Event: {activity} ({district}) - Level "
                    )
                ]

                self.assertEqual(0, len(locations))
                self.assertEqual(0, len(events))
