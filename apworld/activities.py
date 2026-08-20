from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .world import SR2World


def is_activity_enabled_in_options(world: SR2World, name: str) -> bool:
    match name:
        case "Crowd Control":
            return world.options.include_crowd_control.value == 1
        case "Demolition Derby":
            return world.options.include_demo_derby.value == 1
        case "Drug Trafficking":
            return world.options.include_drug_trafficking.value == 1
        case "Escort":
            return world.options.include_escort.value == 1
        case "Fight Club":
            return world.options.include_fight_club.value == 1
        case "Insurance Fraud":
            return world.options.include_fraud.value == 1
        case "FUZZ":
            return world.options.include_fuzz.value == 1
        case "Heli Assault":
            return world.options.include_heli_assault.value == 1
        case "Mayhem":
            return world.options.include_mayhem.value == 1
        case "Septic Avenger":
            return world.options.include_sewage.value == 1
        case "Snatch":
            return world.options.include_snatch.value == 1
        case "Trail Blazing":
            return world.options.include_torch.value == 1
        case "Chop Shop":
            return world.options.include_chop_shop.value == 1
        case "Hitman":
            return world.options.include_hitman.value == 1
        case "Races":
            return world.options.include_races.value == 1
        case _:
            return False


def generate_level_ids(
    activities: dict,
    chop_shop_lists: dict,
    hitman_lists: dict,
    races: dict,
    start_id: int = 100,
    levels: int = 6,
) -> dict[str, int]:
    result = {}
    current_id = start_id

    # Activities
    for activity, locations in activities.items():
        for location_dict in locations:
            for location in location_dict.values():
                for level in range(1, levels + 1):
                    name = f"{activity} ({location}) - Level {level}"
                    result[name] = current_id
                    current_id += 1

    # Chop Shop
    for location, vehicles in chop_shop_lists.items():
        for vehicle_dict in vehicles:
            for vehicle in vehicle_dict.values():
                name = f"Chop Shop ({location}) - {vehicle}"
                result[name] = current_id
                current_id += 1

    # Hitman
    for location, targets in hitman_lists.items():
        for target_dict in targets:
            for target in target_dict.values():
                name = f"Hitman ({location}) - {target}"
                result[name] = current_id
                current_id += 1

    # Races
    for race in races.values():
        for medal in MEDAL_STRINGS.values():
            name = f"{race} - {medal}"
            result[name] = current_id
            current_id += 1

    return result


ACTIVITIES_LEVEL_BASED = {
    "Crowd Control": [{"crowd_ht": "Hotels & Marina"}, {"crowd_su": "Suburbs"}],
    "Demolition Derby": [{"demoderby_un": "Stilwater University"}],
    "Drug Trafficking": [
        {"drug_ht": "Hotels & Marina"},
        {"drug_ai": "Wardill Airport"},
    ],
    "Escort": [
        {"escort_un": "Stilwater University"},
        {"escort_rl": "Red Light District"},
    ],
    "Fight Club": [{"fight_ar": "Ultor Dome"}, {"fight_pr": "Stilwater Prison"}],
    "Insurance Fraud": [
        {"fraud_mu": "Museum District"},
        {"fraud_fc": "Factories District"},
    ],
    "FUZZ": [{"fuzz_pj": "Project District"}, {"fuzz_sx": "Suburbs Expansion"}],
    "Heli Assault": [{"heli_br": "Barrio District"}, {"heli_tp": "Trailer Park"}],
    "Mayhem": [
        {"mayhem_nu": "Nuclear Power Plant"},
        {"mayhem_st": "Red Light District"},
    ],
    "Septic Avenger": [
        {"sewage_rl": "Red Light District"},
        {"sewage_sx": "Suburbs Expansion"},
    ],
    "Snatch": [{"snatch_ct": "Chinatown"}, {"snatch_dt": "Downtown"}],
    "Trail Blazing": [{"torch_ap": "Apartments District"}, {"torch_dt": "Downtown"}],
}

CHOP_SHOP_LISTS = {
    "Suburbs": [
        {"CHOP_SHOP_TARGET_1_1": "Mockingbird"},
        {"CHOP_SHOP_TARGET_1_2": "Churchill"},
        {"CHOP_SHOP_TARGET_1_3": "Hammerhead"},
        {"CHOP_SHOP_TARGET_1_4": "Compton"},
        {"CHOP_SHOP_TARGET_1_5": "Topher"},
        {"CHOP_SHOP_TARGET_1_6": "Quota"},
        {"CHOP_SHOP_TARGET_1_7": "Five-O"},
        {"CHOP_SHOP_TARGET_1_8": "Ambulance"},
    ],
    "Downtown": [
        {"CHOP_SHOP_TARGET_2_1": "Go!"},
        {"CHOP_SHOP_TARGET_2_2": "Taxi"},
        {"CHOP_SHOP_TARGET_2_3": "Wellington"},
        {"CHOP_SHOP_TARGET_2_4": "Alaskan"},
        {"CHOP_SHOP_TARGET_2_5": "Varsity"},
        {"CHOP_SHOP_TARGET_2_6": "Zenith"},
        {"CHOP_SHOP_TARGET_2_7": "Justice"},
        {"CHOP_SHOP_TARGET_2_8": "Titan"},
    ],
    "Apartments": [
        {"CHOP_SHOP_TARGET_3_1": "Swindle"},
        {"CHOP_SHOP_TARGET_3_2": "Betsy"},
        {"CHOP_SHOP_TARGET_3_3": "NRG V8"},
        {"CHOP_SHOP_TARGET_3_4": "Raycaster"},
        {"CHOP_SHOP_TARGET_3_5": "Melbourne"},
        {"CHOP_SHOP_TARGET_3_6": "Bezier"},
        {"CHOP_SHOP_TARGET_3_7": "Cosmos"},
        {"CHOP_SHOP_TARGET_3_8": "Status Quo"},
    ],
    "Truckyard": [
        {"CHOP_SHOP_TARGET_4_1": "Voxel"},
        {"CHOP_SHOP_TARGET_4_2": "Magma"},
        {"CHOP_SHOP_TARGET_4_3": "Attrazione"},
        {"CHOP_SHOP_TARGET_4_4": "Superiore"},
        {"CHOP_SHOP_TARGET_4_5": "Venom Classic"},
        {"CHOP_SHOP_TARGET_4_6": "Eiswolf"},
        {"CHOP_SHOP_TARGET_4_7": "Socialite"},
        {"CHOP_SHOP_TARGET_4_8": "Peacekeeper"},
    ],
    "Factories": [
        {"CHOP_SHOP_TARGET_5_1": "Voyage"},
        {"CHOP_SHOP_TARGET_5_2": "Danville"},
        {"CHOP_SHOP_TARGET_5_3": "Mag"},
        {"CHOP_SHOP_TARGET_5_4": "Bag Boy"},
        {"CHOP_SHOP_TARGET_5_5": "Backhoe"},
        {"CHOP_SHOP_TARGET_5_6": "Bulldozer"},
        {"CHOP_SHOP_TARGET_5_7": "Delivery Truck"},
        {"CHOP_SHOP_TARGET_5_8": "Longhauler"},
    ],
}

HITMAN_LISTS = {
    "Barrio": [
        {"HITMAN_LOC_1_1": "Alvan"},
        {"HITMAN_LOC_1_2": "Brad"},
        {"HITMAN_LOC_1_3": "Anoop"},
        {"HITMAN_LOC_1_4": "Frank"},
        {"HITMAN_LOC_1_5": "Scott"},
        {"HITMAN_LOC_1_6": "James"},
    ],
    "Hotels & Marina": [
        {"HITMAN_LOC_2_1": "Jeremiah"},
        {"HITMAN_LOC_2_2": "Nate"},
        {"HITMAN_LOC_2_3": "Brian"},
        {"HITMAN_LOC_2_4": "Chris"},
        {"HITMAN_LOC_2_5": "Randy"},
        {"HITMAN_LOC_2_6": "Nick"},
    ],
    "Prison": [
        {"HITMAN_LOC_3_1": "Everett"},
        {"HITMAN_LOC_3_2": "Justin"},
        {"HITMAN_LOC_3_3": "Chris"},
        {"HITMAN_LOC_3_4": "Tim"},
        {"HITMAN_LOC_3_5": "Mitri"},
        {"HITMAN_LOC_3_6": "Frank"},
    ],
    "Trailer Park": [
        {"HITMAN_LOC_4_1": "Shannon"},
        {"HITMAN_LOC_4_2": "Jim"},
        {"HITMAN_LOC_4_3": "Roje"},
        {"HITMAN_LOC_4_4": "Mike"},
        {"HITMAN_LOC_4_5": "Clint"},
        {"HITMAN_LOC_4_6": "Greg"},
    ],
    "Saint's Row": [
        {"HITMAN_LOC_5_1": "Apoop"},
        {"HITMAN_LOC_5_2": "Larry"},
        {"HITMAN_LOC_5_3": "Seabaugh"},
        {"HITMAN_LOC_5_4": "Mr. Flegel"},
        {"HITMAN_LOC_5_5": "Lt. Freeball"},
        {"HITMAN_LOC_5_6": "Russell"},
    ],
}

RACES = {
    "bike_air": "Race (Bike, Huntersfield)",
    "bike_ht": "Race (Bike, Stilwater Boardwalk)",
    "bike_mu": "Race (Bike, Amberbrook)",
    "bike_tp": "Race (Bike, Elysian Fields)",
    "bike_un": "Race (Bike, Sunsinger)",
    "boat_ht": "Race (Boat, Centennial Beach (Ocean))",
    "boat_pr": "Race (Boat, Hangman's Wharf (Ocean))",
    "car_air1": "Race (Car, Wardill Airport (Carpark))",
    "car_air2": "Race (Car, Wardill Airport (South Runway))",
    "car_dt": "Race (Car, Union Square)",
    "car_ht": "Race (Car, Stilwater Boardwalk)",
    "car_mu": "Race (Car, Amberbrook)",
    "car_nu": "Race (Car, Stilwater Nuclear)",
    "car_pj": "Race (Car, Prawn Court)",
    "car_sr": "Race (Car, Mission Beach)",
    "car_sx": "Race (Car, Misty Lane)",
    "car_tp": "Race (Car, Elysian Fields)",
    "heli_dt": "Race (Helicopter, Brighton)",
    "heli_mu": "Race (Helicopter, Humbolt Park)",
    "heli_sr": "Race (Helicopter, Athos Bay)",
    "jetski_cv": "Race (Jetski, Stilwater Caverns)",
    "jetski_fa": "Race (Jetski, Black Bottom (Ocean))",
    "jetski_nu": "Race (Jetski, Stilwater Nuclear (Ocean))",
    "jetski_pr": "Race (Jetski, Stilwater Penitentiary (Ocean))",
    "jetski_sr": "Race (Jetski, Harrowgate (Ocean))",
    "plane_air": "Race (Plane, Wardill Airport (East Runway))",
    "plane_un": "Race (Plane, Pleasant View)",
}

MEDAL_STRINGS = {
    "gold": "Gold Medal",
    "silver": "Silver Medal",
    "bronze": "Bronze Medal",
}

ACTIVITY_LEVEL_IDS = generate_level_ids(
    ACTIVITIES_LEVEL_BASED, CHOP_SHOP_LISTS, HITMAN_LISTS, RACES
)
