from Options import OptionError
from worlds.AutoWorld import WebWorld, World
from . import rules, regions, locations, items, options as sr2_options
from .items_list import (
    CHEAT_ITEMS,
    FILLER_UNLOCKABLES,
    TRAP_ITEMS,
    USEFUL_UNLOCKABLES,
    WEAPON_ITEM_NAMES,
    TRAP_CHEATS,
)
from .missions import (
    ULTOR_SECRET_MISSION,
    get_mission_by_key,
    STILWATER_CAVERNS_STRONGHOLD,
)
from .patch import generate_patched_game_files


class SR2Web(WebWorld):
    game = "Saints Row 2"
    option_groups = sr2_options.option_groups
    theme = "ocean"
    bug_report_page = "https://github.com/hoXyy/SaintsRow2Archipelago/issues"


class SR2World(World):
    """
    Saints Row 2 is a 2008 action-adventure game developed by Volition and published by THQ.
    It is the sequel to 2006's Saints Row and the second installment in the Saints Row series.

    Set years after the original, the player finds himself in a Stilwater both familiar and strange and challenged with bringing the Saints back as the rightful kings of Stilwater and bringing vengeance to those who wronged him.
    """

    game = "Saints Row 2"

    web = SR2Web()

    generate_output = generate_patched_game_files

    options_dataclass = sr2_options.SR2Options
    options: sr2_options.SR2Options
    topology_present = True

    item_name_to_id = items.ITEM_NAME_TO_ID
    location_name_to_id = locations.LOCATION_NAME_TO_ID

    origin_region_name = "Stilwater"

    ut_can_gen_without_yaml = True

    @staticmethod
    def interpret_slot_data(slot_data: dict[str, object]) -> dict[str, object]:
        return slot_data

    def __init__(self, multiworld, player):
        super().__init__(multiworld, player)
        self.ut_checked_locations: frozenset[int] = frozenset()
        self.ut_received_respect = 0
        self.ut_spent_respect = 0

    @property
    def ut_available_respect(self) -> int:
        return max(0, self.ut_received_respect - self.ut_spent_respect)

    def generate_early(self) -> None:
        re_gen_passthrough = getattr(
            self.multiworld,
            "re_gen_passthrough",
            {},
        )

        slot_data = re_gen_passthrough.get(self.game)

        if slot_data is not None:
            for option_name, value in slot_data.get("options", {}).items():
                option = getattr(self.options, option_name, None)
                if option is not None:
                    setattr(
                        self.options,
                        option_name,
                        option.from_any(value),
                    )

        enabled_gang_arcs = self.options.required_gang_arcs.value

        filler_weight_total = sum(
            (
                self.options.money_filler_item_weight.value,
                self.options.weapon_filler_item_weight.value,
                self.options.vehicle_filler_item_weight.value,
                self.options.misc_filler_item_weight.value,
            )
        )

        if filler_weight_total == 0 and self.options.trap_chance.value < 100:
            raise OptionError(
                "At least one filler category weight must be greater than zero "
                "unless Trap Chance is 100."
            )

        if (
            self.options.include_secret_mission.value == 0
            and self.options.include_secret_mission_as_goal.value == 1
        ):
            raise OptionError(
                "You can't include the secret mission as part of the goal if it's not part of the locations."
            )

        if (
            self.options.include_stilwater_caverns.value == 0
            and self.options.include_stilwater_caverns_as_goal.value == 1
        ):
            raise OptionError(
                "You can't include the Stilwater Caverns as part of the goal if it's not part of the locations."
            )

        if len(enabled_gang_arcs) == 0:
            raise OptionError("You didn't enable any gang arcs.")

    def get_filler_item_name(self) -> str:
        return items.get_random_filler_item_name(self)

    def create_regions(self) -> None:
        regions.create_regions(self)
        locations.create_locations(self)

    def set_rules(self) -> None:
        rules.set_all_rules(self)

    def create_items(self) -> None:
        items.get_all_items(self)

    def create_item(self, name: str) -> items.SR2Item:
        return items.create_item_with_correct_classification(self, name)

    def fill_slot_data(self) -> dict[str, object]:
        selected_names = {
            item.name
            for item in (
                *self.multiworld.get_items(),
                *self.multiworld.precollected_items[self.player],
            )
            if item.player == self.player and item.code is not None
        }
        unlockable_names = set(USEFUL_UNLOCKABLES) | set(FILLER_UNLOCKABLES)
        cheat_names = set(CHEAT_ITEMS) | set(WEAPON_ITEM_NAMES) | set(TRAP_CHEATS)
        selected_arcs = self.options.required_gang_arcs.value
        goal_locations = []

        for arc_name, finale_key in (
            (sr2_options.RONIN_ARC_NAME, "rn11"),
            (sr2_options.SAMEDI_ARC_NAME, "ss11"),
            (sr2_options.BROTHERHOOD_ARC_NAME, "bh11"),
            (sr2_options.ULTOR_EPILOGUE_ARC_NAME, "ep04"),
        ):
            if arc_name in selected_arcs:
                goal_locations.append(get_mission_by_key(finale_key).id)

        if self.options.include_secret_mission_as_goal.value == 1:
            goal_locations.append(ULTOR_SECRET_MISSION.id)

        if self.options.include_stilwater_caverns_as_goal.value == 1:
            goal_locations.append(STILWATER_CAVERNS_STRONGHOLD.id)

        return {
            "protocol": 4,
            "goal_locations": goal_locations,
            "managed_unlockables": sorted(selected_names & unlockable_names),
            "managed_cheats": sorted(selected_names & cheat_names),
            "persistent_items": sorted(
                selected_names & set(items.PERSISTENT_ACTIVITY_UNLOCK_ITEMS)
            ),
            "features": {
                "exclusive_respect": bool(
                    selected_names
                    & {items.RESPECT_ITEM_NAME, items.BONUS_RESPECT_ITEM_NAME}
                ),
                "block_vanilla_unlockables": True,
                "notoriety_traps": bool(selected_names & set(TRAP_ITEMS)),
            },
            "enabled_progression": {
                "missions": True,
                "activities": any(
                    (
                        self.options.include_crowd_control.value,
                        self.options.include_demo_derby.value,
                        self.options.include_drug_trafficking.value,
                        self.options.include_escort.value,
                        self.options.include_fight_club.value,
                        self.options.include_fraud.value,
                        self.options.include_fuzz.value,
                        self.options.include_heli_assault.value,
                        self.options.include_mayhem.value,
                        self.options.include_sewage.value,
                        self.options.include_snatch.value,
                        self.options.include_torch.value,
                    )
                ),
                "hitman": bool(self.options.include_hitman.value),
                "chop_shop": bool(self.options.include_chop_shop.value),
                "cds": bool(self.options.include_cds.value),
                "races": bool(self.options.include_races.value),
                "tags": bool(self.options.include_tags.value),
                "style_level": self.options.style_level_location_count > 0,
            },
            "options": self.options.as_dict(
                *sr2_options.SR2Options.__annotations__.keys()
            ),
        }
