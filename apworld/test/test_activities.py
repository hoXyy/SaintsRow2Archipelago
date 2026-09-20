from .bases import SR2TestBase
from ..activities import (
    ACTIVITIES_LEVEL_BASED,
    ACTIVITY_STARTING_ACTIVITY_ITEM_MAPPING,
    CHOP_SHOP_LISTS,
    HITMAN_LISTS,
    RACES,
    MEDAL_STRINGS,
)
from ..missions import get_mission_complete_item_name, get_mission_by_key
from ..items import (
    CHOP_SHOP_UNLOCK_ITEM,
    HITMAN_UNLOCK_ITEM,
    get_race_unlock_item,
)


class TestActivityLocations(SR2TestBase):
    run_default_tests = False

    def test_activity_levels_require_previous_completion(self) -> None:
        for activity, instances in ACTIVITIES_LEVEL_BASED.items():
            districts = [
                district for instance in instances for district in instance.values()
            ]

            tss02_complete_item = self.get_item_by_name(
                get_mission_complete_item_name(get_mission_by_key("tss02"))
            )
            activity_unlock_item = self.get_item_by_name(
                ACTIVITY_STARTING_ACTIVITY_ITEM_MAPPING[activity]
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

                    state.collect(tss02_complete_item, prevent_sweep=True)
                    state.collect(activity_unlock_item, prevent_sweep=True)

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
            districts = [
                value for d in ACTIVITIES_LEVEL_BASED[activity] for value in d.values()
            ]

            for district in districts:
                with self.subTest(activity=activity, district=district):
                    state = self.get_fresh_state()
                    lvl1_location = self.world.get_location(
                        f"{activity} ({district}) - Level 1"
                    )
                    tss02_complete_item = self.get_item_by_name(
                        get_mission_complete_item_name(get_mission_by_key("tss02"))
                    )
                    activity_unlock_item = self.get_item_by_name(
                        ACTIVITY_STARTING_ACTIVITY_ITEM_MAPPING[activity]
                    )

                    self.assertFalse(lvl1_location.can_reach(state))

                    state.collect(tss02_complete_item, prevent_sweep=True)
                    state.collect(activity_unlock_item, prevent_sweep=True)

                    self.assertTrue(lvl1_location.can_reach(state))

    def test_chop_shop_requires_story_gate_and_unlock_pass(self) -> None:
        list_name, vehicles = next(iter(CHOP_SHOP_LISTS.items()))
        vehicle = next(iter(vehicles[0].values()))
        location = self.world.get_location(f"Chop Shop ({list_name}) - {vehicle}")
        state = self.get_fresh_state()

        story_item = self.get_item_by_name(
            get_mission_complete_item_name(get_mission_by_key("tss02"))
        )
        unlock_item = self.get_item_by_name(CHOP_SHOP_UNLOCK_ITEM)

        self.assertFalse(location.access_rule(state))
        state.collect(story_item, prevent_sweep=True)
        self.assertFalse(location.access_rule(state))

        state = self.get_fresh_state()
        state.collect(unlock_item, prevent_sweep=True)
        self.assertFalse(location.access_rule(state))
        state.collect(story_item, prevent_sweep=True)
        self.assertTrue(location.access_rule(state))

    def test_hitman_requires_story_gate_and_unlock_pass(self) -> None:
        list_name, targets = next(iter(HITMAN_LISTS.items()))
        target = next(iter(targets[0].values()))
        location = self.world.get_location(f"Hitman ({list_name}) - {target}")
        state = self.get_fresh_state()

        story_item = self.get_item_by_name(
            get_mission_complete_item_name(get_mission_by_key("tss02"))
        )
        unlock_item = self.get_item_by_name(HITMAN_UNLOCK_ITEM)

        self.assertFalse(location.access_rule(state))
        state.collect(story_item, prevent_sweep=True)
        self.assertFalse(location.access_rule(state))

        state = self.get_fresh_state()
        state.collect(unlock_item, prevent_sweep=True)
        self.assertFalse(location.access_rule(state))
        state.collect(story_item, prevent_sweep=True)
        self.assertTrue(location.access_rule(state))

    def test_each_race_class_requires_story_gate_and_its_unlock_pass(self) -> None:
        representative_keys = [
            "car_air1",
            "bike_air",
            "plane_air",
            "heli_dt",
            "boat_ht",
            "jetski_cv",
        ]
        story_item_name = get_mission_complete_item_name(get_mission_by_key("tss02"))

        for race_key in representative_keys:
            race_name = RACES[race_key]
            location = self.world.get_location(
                f"{race_name} - {MEDAL_STRINGS['bronze']}"
            )
            state = self.get_fresh_state()
            story_item = self.get_item_by_name(story_item_name)
            unlock_item = self.get_item_by_name(get_race_unlock_item(race_key))

            with self.subTest(race=race_key):
                self.assertFalse(location.access_rule(state))
                state.collect(story_item, prevent_sweep=True)
                self.assertFalse(location.access_rule(state))

                state = self.get_fresh_state()
                unlock_item = self.get_item_by_name(get_race_unlock_item(race_key))
                story_item = self.get_item_by_name(story_item_name)
                state.collect(unlock_item, prevent_sweep=True)
                self.assertFalse(location.access_rule(state))
                state.collect(story_item, prevent_sweep=True)
                self.assertTrue(location.access_rule(state))


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
