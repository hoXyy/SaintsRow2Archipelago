import unittest
from collections import Counter

from .bases import SR2TestBase
from ..items import RESPECT_ITEM_NAME
from ..missions import (
    get_mission_by_key,
    get_mission_complete_item_name,
    create_minimum_respect_table,
    RONIN_CHAIN,
    SAMEDI_CHAIN,
    BROTHERHOOD_CHAIN,
    MISSION_CHAINS,
    ULTOR_SECRET_MISSION,
    STILWATER_CAVERNS_STRONGHOLD,
    get_mission_complete_event_name,
)
from ..options import (
    RONIN_ARC_NAME,
    BROTHERHOOD_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)


class TestMissionLogic(SR2TestBase):
    run_default_tests = False
    options = {
        "required_gang_arcs": {RONIN_ARC_NAME, BROTHERHOOD_ARC_NAME, SAMEDI_ARC_NAME}
    }

    def test_all_gang_arc_first_missions_requires_tss04(self) -> None:
        respect_requirement_table = create_minimum_respect_table()
        tss04 = get_mission_by_key("tss04")
        tss04_complete_item = self.get_item_by_name(
            get_mission_complete_item_name(tss04)
        )
        respect_item = self.get_item_by_name(RESPECT_ITEM_NAME)

        for mission_key in {"rn01", "bh01", "ss01"}:
            with self.subTest(mission=mission_key):
                state = self.get_fresh_state()
                mission = get_mission_by_key(mission_key)
                mission_location = self.world.get_location(mission.name)

                for i in range(respect_requirement_table[mission_key]):
                    state.collect(respect_item)

                self.assertFalse(mission_location.can_reach(state))

                state.collect(tss04_complete_item)

                self.assertTrue(mission_location.can_reach(state))

    def test_gang_finales_require_every_stronghold(self) -> None:
        gang_finales = [
            ("rn11", RONIN_CHAIN["strongholds"]),
            ("ss11", SAMEDI_CHAIN["strongholds"]),
            ("bh11", BROTHERHOOD_CHAIN["strongholds"]),
        ]

        for finale_key, strongholds in gang_finales:
            finale = get_mission_by_key(finale_key)
            location = self.world.get_location(finale.name)

            for missing_stronghold in strongholds:
                with self.subTest(
                    finale=finale_key,
                    missing=missing_stronghold.key,
                ):
                    state = self.multiworld.get_all_state()
                    item = self.get_item_by_name(
                        get_mission_complete_item_name(missing_stronghold)
                    )

                    state.remove(item)
                    self.assertFalse(location.access_rule(state))

                    state.collect(item, prevent_sweep=True)
                    self.assertTrue(location.access_rule(state))


class TestMissionPredecessors(SR2TestBase):
    run_default_tests = False
    options = {
        "required_gang_arcs": {
            RONIN_ARC_NAME,
            BROTHERHOOD_ARC_NAME,
            SAMEDI_ARC_NAME,
            ULTOR_EPILOGUE_ARC_NAME,
        },
        "include_secret_mission": 1,
        "include_stilwater_caverns": 1,
    }

    def test_every_mission_and_event_require_their_predecessor(self) -> None:
        location_cache = self.multiworld.regions.location_cache[self.player]
        entries = [
            entry
            for chain in MISSION_CHAINS
            for entry in (*chain["missions"], *chain["strongholds"])
        ]
        entries.append(ULTOR_SECRET_MISSION)
        entries.append(STILWATER_CAVERNS_STRONGHOLD)

        for entry in entries:
            if entry.name not in location_cache or entry.unlocked_by is None:
                continue

            with self.subTest(entry=entry.key, predecessor=entry.unlocked_by):
                predecessor = get_mission_by_key(entry.unlocked_by)
                predecessor_item = self.get_item_by_name(
                    get_mission_complete_item_name(predecessor)
                )
                location = self.world.get_location(entry.name)
                event = self.world.get_location(get_mission_complete_event_name(entry))
                state = self.multiworld.get_all_state()

                state.remove(predecessor_item)

                self.assertFalse(location.access_rule(state))
                self.assertFalse(event.access_rule(state))

                state.collect(predecessor_item, prevent_sweep=True)

                self.assertTrue(location.access_rule(state))
                self.assertTrue(event.access_rule(state))

    def test_first_mission_and_event_have_no_predecessor_requirement(self) -> None:
        first_mission = get_mission_by_key("tss01")
        location = self.world.get_location(first_mission.name)
        event = self.world.get_location(get_mission_complete_event_name(first_mission))
        state = self.get_fresh_state()

        self.assertIsNone(first_mission.unlocked_by)
        self.assertTrue(location.access_rule(state))
        self.assertTrue(event.access_rule(state))


class TestMissionPredecessorData(unittest.TestCase):
    def test_every_predecessor_key_resolves(self) -> None:
        entries = [
            entry
            for chain in MISSION_CHAINS
            for entry in (*chain["missions"], *chain["strongholds"])
        ]
        entries.append(ULTOR_SECRET_MISSION)
        entries.append(STILWATER_CAVERNS_STRONGHOLD)

        for entry in entries:
            if entry.unlocked_by is None:
                continue

            with self.subTest(entry=entry.key, predecessor=entry.unlocked_by):
                predecessor = get_mission_by_key(entry.unlocked_by)
                self.assertEqual(entry.unlocked_by, predecessor.key)


class TestStrongholds(SR2TestBase):
    run_default_tests = False
    options = {
        "required_gang_arcs": {
            RONIN_ARC_NAME,
            BROTHERHOOD_ARC_NAME,
            SAMEDI_ARC_NAME,
            ULTOR_EPILOGUE_ARC_NAME,
        }
    }

    def test_strongholds_require_unlocking_mission(self) -> None:
        for chain in MISSION_CHAINS:
            for stronghold in chain["strongholds"]:
                if (
                    stronghold.name
                    not in self.multiworld.regions.location_cache[self.player]
                ):
                    continue

                with self.subTest(stronghold=stronghold):
                    state = self.get_fresh_state()
                    location = self.world.get_location(stronghold.name)
                    required_completion_item = self.get_item_by_name(
                        get_mission_complete_item_name(
                            get_mission_by_key(stronghold.unlocked_by)
                        )
                    )

                    for _ in range(create_minimum_respect_table()[stronghold.key]):
                        state.collect(
                            self.get_item_by_name(RESPECT_ITEM_NAME), prevent_sweep=True
                        )

                    self.assertFalse(location.access_rule(state))
                    state.collect(required_completion_item)
                    self.assertTrue(location.access_rule(state))


class TestSecretMissionLocationDisabled(SR2TestBase):
    run_default_tests = False
    options = {"include_secret_mission": 0}

    def test_secret_mission_disabled(self) -> None:
        self.assertRaises(KeyError, self.world.get_location, ULTOR_SECRET_MISSION.name)


class TestStilwaterCavernsLocationDisabled(SR2TestBase):
    run_default_tests = False
    options = {"include_stilwater_caverns": 0}

    def test_stilwater_caverns_disabled(self) -> None:
        self.assertRaises(
            KeyError, self.world.get_location, STILWATER_CAVERNS_STRONGHOLD.name
        )


class TestEpilogueAccess(SR2TestBase):
    run_default_tests = False
    options = {"required_gang_arcs": {ULTOR_EPILOGUE_ARC_NAME}}

    def test_ultor_arc_requires_three_kings(self) -> None:
        ep01 = get_mission_by_key("ep01")
        ep01_location = self.world.get_location(ep01.name)
        ep01_event = self.world.get_location(get_mission_complete_event_name(ep01))
        tss04_complete_item = self.get_item_by_name(
            get_mission_complete_item_name(get_mission_by_key("tss04"))
        )
        state = self.multiworld.get_all_state()

        state.remove(tss04_complete_item)

        self.assertFalse(ep01_location.access_rule(state))
        self.assertFalse(ep01_event.access_rule(state))

        state.collect(tss04_complete_item, prevent_sweep=True)

        self.assertTrue(ep01_location.access_rule(state))
        self.assertTrue(ep01_event.access_rule(state))


class TestMinimumRespectTable(unittest.TestCase):
    def test_known_requirements(self) -> None:
        table = create_minimum_respect_table()

        expected = {
            "tss01": 0,
            "tss02": 0,
            "tss03": 1,
            "tss04": 2,
            "rn01": 1,
            "rn11": 15,
            "ss11": 15,
            "bh11": 15,
            "ep01": 1,
            "ep04": 5,
            "em01": 3,
        }

        for key, respect in expected.items():
            with self.subTest(key=key):
                self.assertEqual(respect, table[key])


class KnownRespectCountMixin:
    run_default_tests = False
    expected_respect: int

    def test_known_respect_count(self) -> None:
        counts = Counter(item.name for item in self.multiworld.itempool)

        self.assertEqual(
            self.expected_respect,
            counts[RESPECT_ITEM_NAME],
        )


class TestKnownRoninRespect(KnownRespectCountMixin, SR2TestBase):
    options = {
        "required_gang_arcs": {RONIN_ARC_NAME},
    }
    expected_respect = 17


class TestKnownThreeGangRespect(KnownRespectCountMixin, SR2TestBase):
    options = {
        "required_gang_arcs": {
            RONIN_ARC_NAME,
            SAMEDI_ARC_NAME,
            BROTHERHOOD_ARC_NAME,
        }
    }
    expected_respect = 47


class TestKnownEpilogueRespect(KnownRespectCountMixin, SR2TestBase):
    options = {
        "required_gang_arcs": {
            RONIN_ARC_NAME,
            SAMEDI_ARC_NAME,
            BROTHERHOOD_ARC_NAME,
            ULTOR_EPILOGUE_ARC_NAME,
        }
    }
    expected_respect = 52
