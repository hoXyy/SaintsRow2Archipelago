from ..items import EM01_UNLOCK_ITEM, TSS_SH_CAVERNS_UNLOCK_ITEM
from ..missions import (
    STILWATER_CAVERNS_STRONGHOLD,
    ULTOR_SECRET_MISSION,
    get_mission_by_key,
    get_mission_complete_event_name,
    get_mission_complete_item_name,
)
from ..options import (
    BROTHERHOOD_ARC_NAME,
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)
from .bases import SR2TestBase

arc_finales = {
    RONIN_ARC_NAME: "rn11",
    BROTHERHOOD_ARC_NAME: "bh11",
    SAMEDI_ARC_NAME: "ss11",
}


class TestCompletionRules(SR2TestBase):
    auto_construct = False

    def test_each_gang_arc_finale_is_required(self) -> None:
        for arc in {RONIN_ARC_NAME, SAMEDI_ARC_NAME, BROTHERHOOD_ARC_NAME}:
            with self.subTest(arc=arc):
                self.options = {
                    "required_gang_arcs": {arc},
                }
                self.world_setup()
                state = self.multiworld.get_all_state()
                finale_item = self.get_item_by_name(
                    get_mission_complete_item_name(get_mission_by_key(arc_finales[arc]))
                )

                self.assertTrue(
                    self.multiworld.completion_condition[self.player](state)
                )

                state.remove(finale_item)

                self.assertFalse(
                    self.multiworld.completion_condition[self.player](state)
                )

    def test_all_gang_arc_finales_are_required(self) -> None:
        self.options = {
            "required_gang_arcs": {
                RONIN_ARC_NAME,
                SAMEDI_ARC_NAME,
                BROTHERHOOD_ARC_NAME,
            }
        }
        self.world_setup()
        state = self.multiworld.get_all_state()
        ronin_finale_item = self.get_item_by_name(
            get_mission_complete_item_name(
                get_mission_by_key(arc_finales[RONIN_ARC_NAME])
            )
        )
        brotherhood_finale_item = self.get_item_by_name(
            get_mission_complete_item_name(
                get_mission_by_key(arc_finales[BROTHERHOOD_ARC_NAME])
            )
        )
        samedi_finale_item = self.get_item_by_name(
            get_mission_complete_item_name(
                get_mission_by_key(arc_finales[SAMEDI_ARC_NAME])
            )
        )

        self.assertTrue(self.multiworld.completion_condition[self.player](state))

        state.remove(ronin_finale_item)

        self.assertFalse(self.multiworld.completion_condition[self.player](state))

        state.collect(ronin_finale_item)
        state.remove(samedi_finale_item)

        self.assertFalse(self.multiworld.completion_condition[self.player](state))

        state.collect(samedi_finale_item)
        state.remove(brotherhood_finale_item)

        self.assertFalse(self.multiworld.completion_condition[self.player](state))

        state.collect(brotherhood_finale_item)

        self.assertTrue(self.multiworld.completion_condition[self.player](state))

    def test_every_selected_finale_with_epilogue_is_required(self) -> None:
        self.options = {
            "required_gang_arcs": {
                RONIN_ARC_NAME,
                SAMEDI_ARC_NAME,
                BROTHERHOOD_ARC_NAME,
                ULTOR_EPILOGUE_ARC_NAME,
            }
        }

        for key in ("rn11", "ss11", "bh11", "ep04"):
            with self.subTest(missing=key):
                self.world_setup()
                state = self.multiworld.get_all_state()
                item = self.get_item_by_name(
                    get_mission_complete_item_name(get_mission_by_key(key))
                )

                state.remove(item)

                self.assertFalse(
                    self.multiworld.completion_condition[self.player](state)
                )

    def test_secret_mission_is_additional_goal(self) -> None:
        self.options = {
            "required_gang_arcs": {RONIN_ARC_NAME},
            "include_secret_mission": 1,
            "include_secret_mission_as_goal": 1,
        }
        self.world_setup()
        state = self.multiworld.get_all_state()
        secret_item = self.get_item_by_name(
            get_mission_complete_item_name(ULTOR_SECRET_MISSION)
        )

        state.remove(secret_item)

        self.assertFalse(self.multiworld.completion_condition[self.player](state))

    def test_stilwater_caverns_is_additional_goal(self) -> None:
        self.options = {
            "required_gang_arcs": {RONIN_ARC_NAME},
            "include_stilwater_caverns": 1,
            "include_stilwater_caverns_as_goal": 1,
        }
        self.world_setup()
        state = self.multiworld.get_all_state()
        secret_item = self.get_item_by_name(
            get_mission_complete_item_name(STILWATER_CAVERNS_STRONGHOLD)
        )

        state.remove(secret_item)

        self.assertFalse(self.multiworld.completion_condition[self.player](state))


class TestOptionalMissionUnlockRules(SR2TestBase):
    run_default_tests = False
    options = {
        "required_gang_arcs": {RONIN_ARC_NAME},
        "include_secret_mission": 1,
        "include_stilwater_caverns": 1,
    }

    def test_mission_and_completion_event_need_unlock_item(self) -> None:
        for mission, unlock_item in (
            (ULTOR_SECRET_MISSION, EM01_UNLOCK_ITEM),
            (STILWATER_CAVERNS_STRONGHOLD, TSS_SH_CAVERNS_UNLOCK_ITEM),
        ):
            with self.subTest(mission=mission.key):
                location = self.world.get_location(mission.name)
                event = self.world.get_location(
                    get_mission_complete_event_name(mission)
                )
                state = self.multiworld.get_all_state()

                self.assertTrue(location.can_reach(state))
                self.assertTrue(event.can_reach(state))

                state.remove(self.get_item_by_name(unlock_item))

                self.assertFalse(location.can_reach(state))
                self.assertFalse(event.can_reach(state))

    def test_unlock_item_is_enough_without_respect(self) -> None:
        for mission, unlock_item in (
            (ULTOR_SECRET_MISSION, EM01_UNLOCK_ITEM),
            (STILWATER_CAVERNS_STRONGHOLD, TSS_SH_CAVERNS_UNLOCK_ITEM),
        ):
            with self.subTest(mission=mission.key):
                state = self.get_fresh_state()
                state.collect(self.get_item_by_name(unlock_item))

                self.assertTrue(self.world.get_location(mission.name).can_reach(state))
                self.assertTrue(
                    self.world.get_location(
                        get_mission_complete_event_name(mission)
                    ).can_reach(state)
                )
