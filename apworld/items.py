from __future__ import annotations

from typing import TYPE_CHECKING

from BaseClasses import Item, ItemClassification, LocationProgressType
from Options import OptionError

from .missions import get_mission_by_key
from .options import StartingActivity

if TYPE_CHECKING:
    from .world import SR2World

from .items_list import (
    CHEAT_ITEMS,
    CONSUMABLE_FILLER_ITEMS,
    FILLER_UNLOCKABLES,
    MISC_CHEAT_ITEMS,
    MONEY_ITEM_NAMES,
    TRAP_CHEATS,
    TRAP_ITEMS,
    TRAP_NOTORIETY_ITEMS,
    USEFUL_UNLOCKABLES,
    VEHICLE_CHEAT_ITEMS,
    WEAPON_ITEM_NAMES,
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

HITMAN_UNLOCK_ITEM = "Hitman Activity Unlock Pass"
CHOP_SHOP_UNLOCK_ITEM = "Chop Shop Activity Unlock Pass"
CD_UNLOCK_ITEM = "CD Collectible Unlock Pass"
TAGS_UNLOCK_ITEM = "Tags Collectible Unlock Pass"
STUNT_JUMPS_UNLOCK_ITEM = "Stunt Jumps Unlock Pass"

CAR_RACES_UNLOCK_ITEM = "Car Races Activity Unlock Pass"
BIKE_RACES_UNLOCK_ITEM = "Bike Races Activity Unlock Pass"
PLANE_RACES_UNLOCK_ITEM = "Plane Races Activity Unlock Pass"
HELICOPTER_RACES_UNLOCK_ITEM = "Helicopter Races Activity Unlock Pass"
BOAT_RACES_UNLOCK_ITEM = "Boat Races Activity Unlock Pass"

TAXI_DIVERSION_UNLOCK_ITEM = "Taxi Diversion Unlock Pass"
PARAMEDIC_DIVERSION_UNLOCK_ITEM = "Paramedic Diversion Unlock Pass"
FIREFIGHTER_DIVERSION_UNLOCK_ITEM = "Firefighter Diversion Unlock Pass"
TOWING_DIVERSION_UNLOCK_ITEM = "Towing Diversion Unlock Pass"
HOING_DIVERSION_UNLOCK_ITEM = "Ho-ing Diversion Unlock Pass"

EM01_UNLOCK_ITEM = "Dex's Phone Number"
TSS_SH_CAVERNS_UNLOCK_ITEM = "Stilwater Caverns Stronghold Unlock"

LEVEL_ACTIVITY_UNLOCK_ITEMS = [
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

PERSISTENT_ACTIVITY_UNLOCK_ITEMS = [
    HITMAN_UNLOCK_ITEM,
    CHOP_SHOP_UNLOCK_ITEM,
    CD_UNLOCK_ITEM,
    TAGS_UNLOCK_ITEM,
    CAR_RACES_UNLOCK_ITEM,
    BIKE_RACES_UNLOCK_ITEM,
    PLANE_RACES_UNLOCK_ITEM,
    HELICOPTER_RACES_UNLOCK_ITEM,
    BOAT_RACES_UNLOCK_ITEM,
    STUNT_JUMPS_UNLOCK_ITEM,
    TAXI_DIVERSION_UNLOCK_ITEM,
    PARAMEDIC_DIVERSION_UNLOCK_ITEM,
    FIREFIGHTER_DIVERSION_UNLOCK_ITEM,
    TOWING_DIVERSION_UNLOCK_ITEM,
    HOING_DIVERSION_UNLOCK_ITEM,
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
    StartingActivity.option_hitman: HITMAN_UNLOCK_ITEM,
    StartingActivity.option_chop_shop: CHOP_SHOP_UNLOCK_ITEM,
    StartingActivity.option_cd: CD_UNLOCK_ITEM,
    StartingActivity.option_tags: TAGS_UNLOCK_ITEM,
    StartingActivity.option_bike_races: BIKE_RACES_UNLOCK_ITEM,
    StartingActivity.option_plane_races: PLANE_RACES_UNLOCK_ITEM,
    StartingActivity.option_car_races: CAR_RACES_UNLOCK_ITEM,
    StartingActivity.option_boat_races: BOAT_RACES_UNLOCK_ITEM,
    StartingActivity.option_helicopter_races: HELICOPTER_RACES_UNLOCK_ITEM,
    StartingActivity.option_stunt_jumps: STUNT_JUMPS_UNLOCK_ITEM,
    StartingActivity.option_taxi_diversion: TAXI_DIVERSION_UNLOCK_ITEM,
    StartingActivity.option_firefighter_diversion: FIREFIGHTER_DIVERSION_UNLOCK_ITEM,
    StartingActivity.option_towing_diversion: TOWING_DIVERSION_UNLOCK_ITEM,
    StartingActivity.option_paramedic_diversion: PARAMEDIC_DIVERSION_UNLOCK_ITEM,
    StartingActivity.option_hoing_diversion: HOING_DIVERSION_UNLOCK_ITEM,
}

ITEM_NAME_TO_ID = {
    RESPECT_ITEM_NAME: 2,
    BONUS_RESPECT_ITEM_NAME: 3,
    EM01_UNLOCK_ITEM: 4,
    TSS_SH_CAVERNS_UNLOCK_ITEM: 5,
    **{name: 100 + i for i, name in enumerate(WEAPON_ITEM_NAMES)},
    **{name: 200 + i for i, name in enumerate(USEFUL_UNLOCKABLES)},
    **{name: 300 + i for i, name in enumerate(FILLER_UNLOCKABLES)},
    **{name: 400 + i for i, name in enumerate(CHEAT_ITEMS)},
    **{name: 500 + i for i, name in enumerate(TRAP_ITEMS)},
    **{name: 550 + i for i, name in enumerate(MONEY_ITEM_NAMES)},
    **{
        name: 600 + i
        for i, name in enumerate(
            LEVEL_ACTIVITY_UNLOCK_ITEMS + PERSISTENT_ACTIVITY_UNLOCK_ITEMS
        )
    },
    **{name: 700 + i for i, name in enumerate(CONSUMABLE_FILLER_ITEMS)},
}

DEFAULT_ITEM_CLASSIFICATION = {
    RESPECT_ITEM_NAME: ItemClassification.progression_skip_balancing,
    BONUS_RESPECT_ITEM_NAME: ItemClassification.useful,
    EM01_UNLOCK_ITEM: ItemClassification.progression,
    TSS_SH_CAVERNS_UNLOCK_ITEM: ItemClassification.progression,
    **dict.fromkeys(MONEY_ITEM_NAMES, ItemClassification.filler),
    **dict.fromkeys(WEAPON_ITEM_NAMES, ItemClassification.filler),
    **dict.fromkeys(USEFUL_UNLOCKABLES, ItemClassification.useful),
    **dict.fromkeys(FILLER_UNLOCKABLES, ItemClassification.filler),
    **dict.fromkeys(TRAP_ITEMS, ItemClassification.trap),
    **dict.fromkeys(CHEAT_ITEMS, ItemClassification.filler),
    **dict.fromkeys(
        LEVEL_ACTIVITY_UNLOCK_ITEMS + PERSISTENT_ACTIVITY_UNLOCK_ITEMS,
        ItemClassification.progression,
    ),
    **dict.fromkeys(CONSUMABLE_FILLER_ITEMS, ItemClassification.filler),
}


class SR2Item(Item):
    game = "Saints Row 2"


def get_random_filler_item_name(world: SR2World) -> str:
    trap_item_list: list[str] = []

    if bool(world.options.include_weather_trap_items.value):
        trap_item_list.extend(TRAP_CHEATS)

    if bool(world.options.include_notoriety_trap_items.value):
        trap_item_list.extend(TRAP_NOTORIETY_ITEMS)

    if len(trap_item_list) == 0:
        return get_random_nontrap_filler_item_name(world)

    if world.random.randint(0, 99) < world.options.trap_chance.value:
        return world.random.choice(trap_item_list)

    return get_random_nontrap_filler_item_name(world)


def get_random_nontrap_filler_item_name(world: SR2World) -> str:
    categories = [
        MONEY_ITEM_NAMES,
        WEAPON_ITEM_NAMES,
        VEHICLE_CHEAT_ITEMS,
        MISC_CHEAT_ITEMS,
        CONSUMABLE_FILLER_ITEMS,
    ]

    weights = [
        world.options.money_filler_item_weight.value,
        world.options.weapon_filler_item_weight.value,
        world.options.vehicle_filler_item_weight.value,
        world.options.misc_filler_item_weight.value,
        world.options.consumable_filler_item_weight.value,
    ]

    category = world.random.choices(categories, weights=weights, k=1)[0]
    return world.random.choice(category)


def create_item_with_correct_classification(world: SR2World, item: str) -> SR2Item:
    return SR2Item(
        item, DEFAULT_ITEM_CLASSIFICATION[item], ITEM_NAME_TO_ID[item], world.player
    )


def get_all_items(world: SR2World) -> None:
    from .activities import get_starting_activity_item_name
    from .missions import get_required_respect_count

    starting_activity_item = get_starting_activity_item_name(world)

    world.get_location(get_mission_by_key("tss02").name).place_locked_item(
        world.create_item(starting_activity_item)
    )

    unfilled_locations_count = len(
        world.multiworld.get_unfilled_locations(world.player)
    )

    respect_count = get_required_respect_count(world)

    bonus_respect_count = round(
        respect_count * world.options.bonus_respect_percentage.value / 100
    )

    if world.options.bonus_respect_percentage.value > 0:
        bonus_respect_count = max(1, bonus_respect_count)

    # Respect is progression, so excluded locations can't hold it either.
    respect_safe_locations = [
        location
        for location in world.multiworld.get_unfilled_locations(world.player)
        if getattr(location, "respect_safe", False)
        and location.progress_type != LocationProgressType.EXCLUDED
    ]

    if respect_count > len(respect_safe_locations):
        raise OptionError(
            f"Saints Row 2 ({world.player_name}): requires "
            f"{respect_count} progression Respect items, but the selected "
            f"options provide only {len(respect_safe_locations)} "
            "Respect-safe locations. Enable more activities."
        )

    enabled_activity_items = get_enabled_unlock_items_list(world)
    enabled_activity_items.remove(starting_activity_item)

    optional_mission_unlock_items = []

    if world.options.include_secret_mission.value == 1:
        optional_mission_unlock_items.append(EM01_UNLOCK_ITEM)

    if world.options.include_stilwater_caverns.value == 1:
        optional_mission_unlock_items.append(TSS_SH_CAVERNS_UNLOCK_ITEM)

    required_item_count = (
        respect_count
        + bonus_respect_count
        + len(enabled_activity_items)
        + len(optional_mission_unlock_items)
    )

    if required_item_count > unfilled_locations_count:
        raise OptionError(
            f"Saints Row 2 ({world.player_name}): requires "
            f"{required_item_count} Respect and unlock items, but only has "
            f"{unfilled_locations_count} available locations. "
            "Enable more activities or lower Bonus Respect Percentage."
        )

    item_pool: list[Item] = [
        world.create_item(RESPECT_ITEM_NAME) for _ in range(respect_count)
    ]

    item_pool.extend(
        [world.create_item(BONUS_RESPECT_ITEM_NAME) for _ in range(bonus_respect_count)]
    )

    item_pool.extend([world.create_item(name) for name in enabled_activity_items])

    item_pool.extend(
        [world.create_item(name) for name in optional_mission_unlock_items]
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


def get_race_unlock_item(race_key: str) -> str:
    if race_key.startswith("car_"):
        return CAR_RACES_UNLOCK_ITEM
    if race_key.startswith("bike_"):
        return BIKE_RACES_UNLOCK_ITEM
    if race_key.startswith("plane_"):
        return PLANE_RACES_UNLOCK_ITEM
    if race_key.startswith("heli_"):
        return HELICOPTER_RACES_UNLOCK_ITEM
    if race_key.startswith(("boat_", "jetski_")):
        return BOAT_RACES_UNLOCK_ITEM
    raise ValueError(f"Unknown race class: {race_key}")


def get_enabled_unlock_items_list(world: SR2World) -> list[str]:
    from .activities import get_enabled_activity_list

    enabled_items = []

    enabled_activities = get_enabled_activity_list(world)

    for [key, item] in ACTIVITY_UNLOCK_ITEMS_SETTINGS_MAPPING.items():
        if key in enabled_activities:
            enabled_items.append(item)

    return enabled_items
