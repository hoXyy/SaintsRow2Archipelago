from __future__ import annotations

from BaseClasses import Location, Region, ItemClassification

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .world import SR2World

from .missions import (
    RONIN_CHAIN,
    BROTHERHOOD_CHAIN,
    SAMEDI_CHAIN,
    TSS_INTRO_CHAIN,
    ULTOR_EPILOGUE_CHAIN,
    ULTOR_SECRET_MISSION,
    MISSION_CHAINS,
    get_mission_complete_event_name,
    get_mission_complete_item_name,
    Mission,
    Stronghold,
)
from .activities import (
    ACTIVITIES_LEVEL_BASED,
    CHOP_SHOP_LISTS,
    HITMAN_LISTS,
    ACTIVITY_LEVEL_IDS,
    is_activity_enabled_in_options,
)
from .options import (
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    BROTHERHOOD_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)
from .collectibles import CD_MAPPING, CD_IDS

from .items import SR2Item


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

        # This stronghold is only required to beat the game, not to beat all of the gang arcs
        for stronghold in TSS_INTRO_CHAIN["strongholds"]:
            add_check_with_completion_event(world, region, stronghold)

    if world.options.include_secret_mission.value == 1:
        add_check_with_completion_event(world, region, ULTOR_SECRET_MISSION)


def create_activities_location(world: SR2World) -> None:
    region = world.get_region("Stilwater")

    for activity in ACTIVITIES_LEVEL_BASED:
        if is_activity_enabled_in_options(world, activity):
            is_respect_safe = activity != "Heli Assault"
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
                            respect_safe=is_respect_safe,
                        )
                    )

                    # add completion event and item to all levels below the last one to have the proper progression tree
                    if level < 6:
                        completion_event = SR2Location(
                            world.player,
                            f"Event: {curr_key} Complete",
                            None,
                            region,
                            respect_safe=is_respect_safe,
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


def create_collectible_locations(world: SR2World) -> None:
    region = world.get_region("Stilwater")

    if world.options.include_cds.value == 1:
        for cd in CD_MAPPING.values():
            region.locations.append(
                SR2Location(world.player, cd, CD_IDS[cd], region, respect_safe=True)
            )


def add_check_with_completion_event(
    world: SR2World, region: Region, mission: Mission | Stronghold
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

    return {
        **mission_locations,
        **ACTIVITY_LEVEL_IDS,
        **CD_IDS,
    }


LOCATION_NAME_TO_ID = generate_location_name_to_id()
