from __future__ import annotations

from BaseClasses import Item, ItemClassification
from Options import OptionError

from typing import TYPE_CHECKING


from .missions import get_mission_by_key
from .options import StartingActivity

if TYPE_CHECKING:
    from .world import SR2World

from .items_list import (
    FILLER_UNLOCKABLES,
    USEFUL_UNLOCKABLES,
    CHEAT_ITEMS,
    WEAPON_ITEM_NAMES,
    TRAP_ITEMS,
    MONEY_ITEM_NAMES,
)

RESPECT_ITEM_NAME = "+1 Respect"
BONUS_RESPECT_ITEM_NAME = "+1 Bonus Respect"

INSURANCE_FRAUD_UNLOCK_ITEM = "Insurance Fraud Activity Unlock Pass"
CROWD_CONTROL_UNLOCK_ITEM = "Crowd Control Activity Unlock Pass"
DEMO_DERBY_UNLOCK_ITEM = "Demolition Derby Activity Unlock Pass"
TRAFFICKING_UNLOCK_ITEM = "Drug Trafficking Activity Unlock Pass"
ESCORT_UNLOCK_ITEM = "Escort Activity Unlock Pass"
FIGHT_CLUB_UNLOCK_ITEM = "Fight Club Activity Unlock Pass"
FUZZ_UNLOCK_ITEM = "FUZZ Activity Unlock Pass"
HELI_ASSAULT_UNLOCK_ITEM = "Heli Assault Activity Unlock Pass"
MAYHEM_UNLOCK_ITEM = "Mayhem Activity Unlock Pass"
SEWAGE_UNLOCK_ITEM = "Septic Avenger Activity Unlock Pass"
SNATCH_UNLOCK_ITEM = "Snatch Activity Unlock Pass"
TORCH_UNLOCK_ITEM = "Trail Blazing Activity Unlock Pass"

ACTIVITY_UNLOCK_ITEMS = [
    INSURANCE_FRAUD_UNLOCK_ITEM,
    CROWD_CONTROL_UNLOCK_ITEM,
    DEMO_DERBY_UNLOCK_ITEM,
    TRAFFICKING_UNLOCK_ITEM,
    ESCORT_UNLOCK_ITEM,
    FUZZ_UNLOCK_ITEM,
    FIGHT_CLUB_UNLOCK_ITEM,
    HELI_ASSAULT_UNLOCK_ITEM,
    MAYHEM_UNLOCK_ITEM,
    SEWAGE_UNLOCK_ITEM,
    SNATCH_UNLOCK_ITEM,
    TORCH_UNLOCK_ITEM,
]

ACTIVITY_UNLOCK_ITEMS_SETTINGS_MAPPING = {
    StartingActivity.option_crowd_control: CROWD_CONTROL_UNLOCK_ITEM,
    StartingActivity.option_demo_derby: DEMO_DERBY_UNLOCK_ITEM,
    StartingActivity.option_drug_trafficking: TRAFFICKING_UNLOCK_ITEM,
    StartingActivity.option_escort: ESCORT_UNLOCK_ITEM,
    StartingActivity.option_fight_club: FIGHT_CLUB_UNLOCK_ITEM,
    StartingActivity.option_fuzz: FUZZ_UNLOCK_ITEM,
    StartingActivity.option_heli_assault: HELI_ASSAULT_UNLOCK_ITEM,
    StartingActivity.option_insurance_fraud: INSURANCE_FRAUD_UNLOCK_ITEM,
    StartingActivity.option_mayhem: MAYHEM_UNLOCK_ITEM,
    StartingActivity.option_septic_avenger: SEWAGE_UNLOCK_ITEM,
    StartingActivity.option_snatch: SNATCH_UNLOCK_ITEM,
    StartingActivity.option_trail_blazing: TORCH_UNLOCK_ITEM,
}

ITEM_NAME_TO_ID = {
    RESPECT_ITEM_NAME: 2,
    BONUS_RESPECT_ITEM_NAME: 3,
    **{name: 100 + i for i, name in enumerate(WEAPON_ITEM_NAMES)},
    **{name: 200 + i for i, name in enumerate(USEFUL_UNLOCKABLES)},
    **{name: 300 + i for i, name in enumerate(FILLER_UNLOCKABLES)},
    **{name: 400 + i for i, name in enumerate(CHEAT_ITEMS)},
    **{name: 500 + i for i, name in enumerate(TRAP_ITEMS)},
    **{name: 550 + i for i, name in enumerate(MONEY_ITEM_NAMES)},
    **{name: 600 + i for i, name in enumerate(ACTIVITY_UNLOCK_ITEMS)},
}

DEFAULT_ITEM_CLASSIFICATION = {
    RESPECT_ITEM_NAME: ItemClassification.progression,
    BONUS_RESPECT_ITEM_NAME: ItemClassification.useful,
    **{name: ItemClassification.filler for name in MONEY_ITEM_NAMES},
    **{name: ItemClassification.filler for name in WEAPON_ITEM_NAMES},
    **{name: ItemClassification.useful for name in USEFUL_UNLOCKABLES},
    **{name: ItemClassification.filler for name in FILLER_UNLOCKABLES},
    **{name: ItemClassification.trap for name in TRAP_ITEMS},
    **{name: ItemClassification.filler for name in CHEAT_ITEMS},
    **{name: ItemClassification.progression for name in ACTIVITY_UNLOCK_ITEMS},
}


class SR2Item(Item):
    game = "Saints Row 2"


def get_random_filler_item_name(world: SR2World) -> str:
    if world.random.randint(0, 99) < world.options.trap_chance.value:
        return world.random.choice(TRAP_ITEMS)

    all_filler_items = MONEY_ITEM_NAMES + WEAPON_ITEM_NAMES + CHEAT_ITEMS

    return world.random.choice(all_filler_items)


def create_item_with_correct_classification(world: SR2World, item: str) -> SR2Item:
    return SR2Item(
        item, DEFAULT_ITEM_CLASSIFICATION[item], ITEM_NAME_TO_ID[item], world.player
    )


def get_all_items(world: SR2World) -> None:
    from .activities import get_starting_activity_item_name
    from .missions import get_required_respect_count

    unfilled_locations_count = len(
        world.multiworld.get_unfilled_locations(world.player)
    )

    respect_count = get_required_respect_count(world)

    bonus_respect_count = round(
        respect_count * world.options.bonus_respect_percentage.value / 100
    )

    if world.options.bonus_respect_percentage.value > 0:
        bonus_respect_count = max(1, bonus_respect_count)

    respect_safe_locations = [
        location
        for location in world.multiworld.get_unfilled_locations(world.player)
        if getattr(location, "respect_safe", False)
    ]

    if respect_count > len(respect_safe_locations):
        raise OptionError(
            f"Saints Row 2 ({world.player_name}): requires "
            f"{respect_count} progression Respect items, but the selected "
            f"options provide only {len(respect_safe_locations)} "
            "Respect-safe locations. Enable more non-Heli activities."
        )

    total_respect_count = respect_count + bonus_respect_count

    if total_respect_count > unfilled_locations_count:
        raise OptionError(
            f"Saints Row 2 ({world.player_name}): requires "
            f"{total_respect_count} total Respect items, but only has "
            f"{unfilled_locations_count} available locations."
        )

    item_pool: list[Item] = [
        world.create_item(RESPECT_ITEM_NAME) for _ in range(respect_count)
    ]

    item_pool.extend(
        [world.create_item(BONUS_RESPECT_ITEM_NAME) for _ in range(bonus_respect_count)]
    )

    starting_activity_item = get_starting_activity_item_name(world)

    world.get_location(get_mission_by_key("tss02").name).place_locked_item(
        world.create_item(starting_activity_item)
    )

    activity_unlock_items_without_starting_activity = ACTIVITY_UNLOCK_ITEMS.copy()
    activity_unlock_items_without_starting_activity.remove(starting_activity_item)

    item_pool.extend(
        [
            world.create_item(name)
            for name in activity_unlock_items_without_starting_activity
        ]
    )

    available_slots = unfilled_locations_count - len(item_pool)

    unlockables = USEFUL_UNLOCKABLES + FILLER_UNLOCKABLES

    selected_unlockables = world.random.sample(
        unlockables,
        min(available_slots, len(unlockables)),
    )

    item_pool.extend(world.create_item(name) for name in selected_unlockables)

    remaining_slots = unfilled_locations_count - len(item_pool)

    item_pool.extend(world.create_filler() for _ in range(remaining_slots))

    world.multiworld.itempool += item_pool
