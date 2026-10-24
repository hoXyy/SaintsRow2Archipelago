from __future__ import annotations

from typing import TYPE_CHECKING

from BaseClasses import ItemClassification, Location, Region

if TYPE_CHECKING:
    from .world import SR2World

from .activities import (
    ACTIVITIES_LEVEL_BASED,
    ACTIVITY_LEVEL_IDS,
    BIKE_RACES,
    BOAT_RACES,
    CAR_RACES,
    CHOP_SHOP_LISTS,
    HELI_RACES,
    HITMAN_LISTS,
    MEDAL_STRINGS,
    PLANE_RACES,
    is_activity_enabled_in_options,
)
from .collectibles import (
    CD_IDS,
    CD_MAPPING,
    MISSABLE_STUNT_JUMP_MAPPING,
    STUNT_JUMP_IDS,
    STUNT_JUMP_MAPPING,
    STYLE_LEVEL_IDS,
    STYLE_LEVEL_LOCATIONS,
    TAGS_IDS,
    TAGS_MAPPING,
)
from .items import SR2Item
from .missions import (
    BROTHERHOOD_CHAIN,
    MISSION_CHAINS,
    RONIN_CHAIN,
    SAMEDI_CHAIN,
    STILWATER_CAVERNS_STRONGHOLD,
    TSS_INTRO_CHAIN,
    ULTOR_EPILOGUE_CHAIN,
    ULTOR_SECRET_MISSION,
    Mission,
    get_mission_complete_event_name,
    get_mission_complete_item_name,
)
from .options import (
    BROTHERHOOD_ARC_NAME,
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)


class SR2Location(Location):
    game = "Saints Row 2"

    def __init__(
        self,
        player: int,
        name: str,
        address: int | None,
        parent: Region,
        *,
        respect_safe: bool = False,
    ) -> None:
        super().__init__(player, name, address, parent)
        self.respect_safe = respect_safe


def create_locations(world: SR2World) -> None:
    create_mission_locations(world)
    create_activities_location(world)
    create_collectible_locations(world)
    create_style_level_locations(world)


def create_mission_locations(world: SR2World) -> None:
    region = world.get_region("Stilwater")
    enabled_arcs = world.options.required_gang_arcs.value

    for mission in TSS_INTRO_CHAIN["missions"]:
        add_check_with_completion_event(world, region, mission)

    if RONIN_ARC_NAME in enabled_arcs:
        for mission in RONIN_CHAIN["missions"]:
            add_check_with_completion_event(world, region, mission)

        for stronghold in RONIN_CHAIN["strongholds"]:
            add_check_with_completion_event(world, region, stronghold)

    if SAMEDI_ARC_NAME in enabled_arcs:
        for mission in SAMEDI_CHAIN["missions"]:
            add_check_with_completion_event(world, region, mission)

        for stronghold in SAMEDI_CHAIN["strongholds"]:
            add_check_with_completion_event(world, region, stronghold)

    if BROTHERHOOD_ARC_NAME in enabled_arcs:
        for mission in BROTHERHOOD_CHAIN["missions"]:
            add_check_with_completion_event(world, region, mission)

        for stronghold in BROTHERHOOD_CHAIN["strongholds"]:
            add_check_with_completion_event(world, region, stronghold)

    if ULTOR_EPILOGUE_ARC_NAME in enabled_arcs:
        for mission in ULTOR_EPILOGUE_CHAIN["missions"]:
            add_check_with_completion_event(world, region, mission)

        for stronghold in ULTOR_EPILOGUE_CHAIN["strongholds"]:
            add_check_with_completion_event(world, region, stronghold)

    if world.options.include_secret_mission.value == 1:
        add_check_with_completion_event(world, region, ULTOR_SECRET_MISSION)

    if world.options.include_stilwater_caverns == 1:
        add_check_with_completion_event(world, region, STILWATER_CAVERNS_STRONGHOLD)


def create_activities_location(world: SR2World) -> None:
    region = world.get_region("Stilwater")

    for activity in ACTIVITIES_LEVEL_BASED:
        if is_activity_enabled_in_options(world, activity):
            activity_locations = [
                value for d in ACTIVITIES_LEVEL_BASED[activity] for value in d.values()
            ]

            for location in activity_locations:
                # each level based activity has 6 levels
                for level in range(1, 7):
                    curr_key = f"{activity} ({location}) - Level {level}"
                    region.locations.append(
                        SR2Location(
                            world.player,
                            curr_key,
                            ACTIVITY_LEVEL_IDS[curr_key],
                            region,
                            respect_safe=True,
                        )
                    )

                    # add completion event and item to all levels below the last one to have the proper progression tree
                    if level < 6:
                        completion_event = SR2Location(
                            world.player,
                            f"Event: {curr_key} Complete",
                            None,
                            region,
                            respect_safe=True,
                        )

                        completion_event.place_locked_item(
                            SR2Item(
                                f"Item: {curr_key} Complete",
                                ItemClassification.progression,
                                None,
                                world.player,
                            )
                        )

                        region.locations.append(completion_event)

    if is_activity_enabled_in_options(world, "Chop Shop"):
        for location in CHOP_SHOP_LISTS:
            vehicles = [
                value for d in CHOP_SHOP_LISTS[location] for value in d.values()
            ]

            for vehicle in vehicles:
                curr_key = f"Chop Shop ({location}) - {vehicle}"
                region.locations.append(
                    SR2Location(
                        world.player,
                        curr_key,
                        ACTIVITY_LEVEL_IDS[curr_key],
                        region,
                        respect_safe=True,
                    )
                )

    if is_activity_enabled_in_options(world, "Hitman"):
        for location in HITMAN_LISTS:
            targets = [value for d in HITMAN_LISTS[location] for value in d.values()]

            for target in targets:
                curr_key = f"Hitman ({location}) - {target}"
                region.locations.append(
                    SR2Location(
                        world.player,
                        curr_key,
                        ACTIVITY_LEVEL_IDS[curr_key],
                        region,
                        respect_safe=True,
                    )
                )

    if is_activity_enabled_in_options(world, "Boat Races"):
        for race in BOAT_RACES.values():
            for medal in MEDAL_STRINGS.values():
                curr_key = f"{race} - {medal}"
                region.locations.append(
                    SR2Location(
                        world.player,
                        curr_key,
                        ACTIVITY_LEVEL_IDS[curr_key],
                        region,
                        respect_safe=True,
                    )
                )

    if is_activity_enabled_in_options(world, "Bike Races"):
        for race in BIKE_RACES.values():
            for medal in MEDAL_STRINGS.values():
                curr_key = f"{race} - {medal}"
                region.locations.append(
                    SR2Location(
                        world.player,
                        curr_key,
                        ACTIVITY_LEVEL_IDS[curr_key],
                        region,
                        respect_safe=True,
                    )
                )

    if is_activity_enabled_in_options(world, "Car Races"):
        for race in CAR_RACES.values():
            for medal in MEDAL_STRINGS.values():
                curr_key = f"{race} - {medal}"
                region.locations.append(
                    SR2Location(
                        world.player,
                        curr_key,
                        ACTIVITY_LEVEL_IDS[curr_key],
                        region,
                        respect_safe=True,
                    )
                )

    if is_activity_enabled_in_options(world, "Heli Races"):
        for race in HELI_RACES.values():
            for medal in MEDAL_STRINGS.values():
                curr_key = f"{race} - {medal}"
                region.locations.append(
                    SR2Location(
                        world.player,
                        curr_key,
                        ACTIVITY_LEVEL_IDS[curr_key],
                        region,
                        respect_safe=True,
                    )
                )

    if is_activity_enabled_in_options(world, "Plane Races"):
        for race in PLANE_RACES.values():
            for medal in MEDAL_STRINGS.values():
                curr_key = f"{race} - {medal}"
                region.locations.append(
                    SR2Location(
                        world.player,
                        curr_key,
                        ACTIVITY_LEVEL_IDS[curr_key],
                        region,
                        respect_safe=True,
                    )
                )


def create_collectible_locations(world: SR2World) -> None:
    region = world.get_region("Stilwater")

    if world.options.include_cds.value == 1:
        for cd in CD_MAPPING.values():
            region.locations.append(
                SR2Location(world.player, cd, CD_IDS[cd], region, respect_safe=True)
            )

    if world.options.include_tags.value == 1:
        for tag in TAGS_MAPPING.values():
            region.locations.append(
                SR2Location(world.player, tag, TAGS_IDS[tag], region, respect_safe=True)
            )

    if world.options.include_stunt_jumps.value == 1:
        for jump in STUNT_JUMP_MAPPING.values():
            region.locations.append(
                SR2Location(
                    world.player, jump, STUNT_JUMP_IDS[jump], region, respect_safe=True
                )
            )

        # This jump is missable if the player doesn't do Pyramid Scheme, so only include it if the Ultor Arc is enabled
        if ULTOR_EPILOGUE_ARC_NAME in world.options.required_gang_arcs:
            for jump in MISSABLE_STUNT_JUMP_MAPPING.values():
                region.locations.append(
                    SR2Location(
                        world.player,
                        jump,
                        STUNT_JUMP_IDS[jump],
                        region,
                        respect_safe=True,
                    )
                )


def create_style_level_locations(world: SR2World) -> None:
    region = world.get_region("Stilwater")

    for location in STYLE_LEVEL_LOCATIONS[
        : world.options.style_level_location_count.value
    ]:
        region.locations.append(
            SR2Location(
                world.player,
                location,
                STYLE_LEVEL_IDS[location],
                region,
                respect_safe=True,
            )
        )


def add_check_with_completion_event(
    world: SR2World, region: Region, mission: Mission
) -> None:
    region.locations.append(
        SR2Location(
            world.player,
            mission.name,
            mission.id,
            region,
            respect_safe=(
                isinstance(mission, Mission) and mission.required_respect == 0
            ),
        )
    )

    if mission.creates_unlock_item:
        event = SR2Location(
            world.player,
            get_mission_complete_event_name(mission),
            None,
            region,
        )

        event.place_locked_item(
            SR2Item(
                get_mission_complete_item_name(mission),
                ItemClassification.progression,
                None,
                world.player,
            )
        )

        region.locations.append(event)


def generate_location_name_to_id() -> dict[str, int]:
    mission_locations = {
        location.name: location.id
        for chain in MISSION_CHAINS
        for location in (*chain["missions"], *chain["strongholds"])
    }

    mission_locations[ULTOR_SECRET_MISSION.name] = ULTOR_SECRET_MISSION.id
    mission_locations[STILWATER_CAVERNS_STRONGHOLD.name] = (
        STILWATER_CAVERNS_STRONGHOLD.id
    )

    return {
        **mission_locations,
        **ACTIVITY_LEVEL_IDS,
        **CD_IDS,
        **STYLE_LEVEL_IDS,
        **TAGS_IDS,
        **STUNT_JUMP_IDS,
    }


LOCATION_NAME_TO_ID = generate_location_name_to_id()
