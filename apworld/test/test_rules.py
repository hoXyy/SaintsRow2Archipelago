from .bases import SR2TestBase
from ..missions import (
    ULTOR_SECRET_MISSION,
    get_mission_by_key,
    get_mission_complete_item_name,
)
from ..options import (
    BROTHERHOOD_ARC_NAME,
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)

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
