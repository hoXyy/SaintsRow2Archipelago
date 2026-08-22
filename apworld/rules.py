from __future__ import annotations

from typing import TYPE_CHECKING
from worlds.generic.Rules import add_rule, add_item_rule
from BaseClasses import Location

if TYPE_CHECKING:
    from .world import SR2World

from .missions import (
    ULTOR_SECRET_MISSION,
    MISSION_CHAINS,
    RONIN_CHAIN,
    SAMEDI_CHAIN,
    BROTHERHOOD_CHAIN,
    ULTOR_EPILOGUE_CHAIN,
    get_mission_by_key,
    get_mission_complete_event_name,
    get_mission_complete_item_name,
    create_minimum_respect_table,
    Mission,
    Stronghold,
)
from .activities import (
    ACTIVITIES_LEVEL_BASED,
    CHOP_SHOP_LISTS,
    HITMAN_LISTS,
    RACES,
    MEDAL_STRINGS,
)
from .items import RESPECT_ITEM_NAME
from .options import (
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    BROTHERHOOD_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)


def set_all_rules(world: SR2World) -> None:
    set_mission_rules(world)
    set_activity_rules(world)
    set_completion_rules(world)
    set_respect_placement_rules(world)


def set_mission_rules(world: SR2World) -> None:
    location_cache = world.multiworld.regions.location_cache[world.player]
    minimum_respect_needed_table = create_minimum_respect_table()

    # Generic respect requirement rules for both missions and strongholds
    for chain in MISSION_CHAINS:
        for mission in chain["missions"]:
            if mission.name in location_cache:
                respect_needed = minimum_respect_needed_table[mission.key]

                if respect_needed > 0:
                    add_rule(
                        world.get_location(mission.name),
                        lambda state, respect_needed=respect_needed: state.has(
                            RESPECT_ITEM_NAME, world.player, respect_needed
                        ),
                    )

                    if mission.creates_unlock_item:
                        add_rule(
                            world.get_location(
                                get_mission_complete_event_name(mission)
                            ),
                            lambda state, respect_needed=respect_needed: state.has(
                                RESPECT_ITEM_NAME, world.player, respect_needed
                            ),
                        )

        for stronghold in chain["strongholds"]:
            if stronghold.name in location_cache:
                respect_needed = minimum_respect_needed_table[stronghold.key]

                if respect_needed > 0:
                    add_rule(
                        world.get_location(stronghold.name),
                        lambda state, respect_needed=respect_needed: state.has(
                            RESPECT_ITEM_NAME, world.player, respect_needed
                        ),
                    )

                    if stronghold.creates_unlock_item:
                        add_rule(
                            world.get_location(
                                get_mission_complete_event_name(stronghold)
                            ),
                            lambda state, respect_needed=respect_needed: state.has(
                                RESPECT_ITEM_NAME, world.player, respect_needed
                            ),
                        )

    # Mission access rules

    # All gang arcs are unlocked by Three Kings
    rn01 = get_mission_by_key("rn01")
    ss01 = get_mission_by_key("ss01")
    bh01 = get_mission_by_key("bh01")
    em01 = ULTOR_SECRET_MISSION

    mark_as_unlocked_after_intro(world, location_cache, [rn01, ss01, bh01, em01])

    # Add required respect count to Revelation
    if ULTOR_SECRET_MISSION.name in location_cache:
        respect_needed = minimum_respect_needed_table[ULTOR_SECRET_MISSION.key]
        location = world.get_location(ULTOR_SECRET_MISSION.name)

        add_rule(
            location,
            lambda state, respect_needed=respect_needed: state.has(
                RESPECT_ITEM_NAME,
                world.player,
                respect_needed,
            ),
        )

    # Mark epilogue chain start as needing all gang arcs done + Stilwater Caverns Stronghold
    ronin_finale = get_mission_by_key("rn11")
    samedi_finale = get_mission_by_key("ss11")
    brotherhood_finale = get_mission_by_key("bh11")
    stilwater_caverns_sh = get_mission_by_key("sh_tss_caverns")
    epilogue_start = get_mission_by_key("ep01")

    if epilogue_start.name in location_cache:
        add_rule(
            world.get_location(epilogue_start.name),
            lambda state: all(
                state.has(get_mission_complete_item_name(finale), world.player)
                for finale in [
                    ronin_finale,
                    samedi_finale,
                    brotherhood_finale,
                    stilwater_caverns_sh,
                ]
            ),
        )

        if epilogue_start.creates_unlock_item:
            add_rule(
                world.get_location(get_mission_complete_event_name(epilogue_start)),
                lambda state: all(
                    state.has(get_mission_complete_item_name(finale), world.player)
                    for finale in [
                        ronin_finale,
                        samedi_finale,
                        brotherhood_finale,
                        stilwater_caverns_sh,
                    ]
                ),
            )

    # Mark each arc finale as needing all strongholds done
    ultor_finale = get_mission_by_key("ep04")
    mark_as_needing_all_strongholds(
        world, location_cache, ronin_finale, RONIN_CHAIN["strongholds"]
    )
    mark_as_needing_all_strongholds(
        world, location_cache, samedi_finale, SAMEDI_CHAIN["strongholds"]
    )
    mark_as_needing_all_strongholds(
        world, location_cache, brotherhood_finale, BROTHERHOOD_CHAIN["strongholds"]
    )
    mark_as_needing_all_strongholds(
        world, location_cache, ultor_finale, ULTOR_EPILOGUE_CHAIN["strongholds"]
    )

    # Stronghold access rules
    for chain in MISSION_CHAINS:
        for stronghold in chain["strongholds"]:
            if stronghold.name in location_cache:
                add_rule(
                    world.get_location(stronghold.name),
                    lambda state, stronghold=stronghold: state.has(
                        get_mission_complete_item_name(
                            get_mission_by_key(stronghold.unlocked_by)
                        ),
                        world.player,
                    ),
                )

                if stronghold.creates_unlock_item:
                    add_rule(
                        world.get_location(get_mission_complete_event_name(stronghold)),
                        lambda state, stronghold=stronghold: state.has(
                            get_mission_complete_item_name(
                                get_mission_by_key(stronghold.unlocked_by)
                            ),
                            world.player,
                        ),
                    )


def set_activity_rules(world: SR2World) -> None:
    location_cache = world.multiworld.regions.location_cache[world.player]
    tss02_complete_item = get_mission_complete_item_name(get_mission_by_key("tss02"))
    tss04_complete_item = get_mission_complete_item_name(get_mission_by_key("tss04"))

    for activity in ACTIVITIES_LEVEL_BASED:
        # Activities unlock after Appointed Defender except Heli Assault which unlocks after Three Kings
        is_heli_assault = activity == "Heli Assault"
        activity_locations = [
            value for d in ACTIVITIES_LEVEL_BASED[activity] for value in d.values()
        ]

        unlock_item = tss04_complete_item if is_heli_assault else tss02_complete_item

        for district in activity_locations:
            for level in range(1, 7):
                curr_key = f"{activity} ({district}) - Level {level}"

                if curr_key not in location_cache:
                    continue
                    
                curr_location = world.get_location(curr_key)

                if level == 1:
                    prerequisite_item = unlock_item
                else:
                    prev_key = f"{activity} ({district}) - Level {level - 1}"
                    prerequisite_item = f"Item: {prev_key} Complete"

                add_rule(
                    curr_location,
                    lambda state, prerequisite_item=prerequisite_item: state.has(
                        prerequisite_item, world.player
                    ),
                )

                if level < 6:
                    add_rule(
                        world.get_location(f"Event: {curr_key} Complete"),
                        lambda state, prerequisite_item=prerequisite_item: state.has(
                            prerequisite_item, world.player
                        ),
                    )

    for location in CHOP_SHOP_LISTS:
        vehicles = [value for d in CHOP_SHOP_LISTS[location] for value in d.values()]

        for vehicle in vehicles:
            curr_key = f"Chop Shop ({location}) - {vehicle}"
            if curr_key in location_cache:
                add_rule(
                    world.get_location(curr_key),
                    lambda state, chop_shop_unlock_item=tss02_complete_item: state.has(
                        chop_shop_unlock_item, world.player
                    ),
                )

    for location in HITMAN_LISTS:
        targets = [value for d in HITMAN_LISTS[location] for value in d.values()]

        for target in targets:
            curr_key = f"Hitman ({location}) - {target}"
            if curr_key in location_cache:
                add_rule(
                    world.get_location(curr_key),
                    lambda state, hitman_unlock_item=tss02_complete_item: state.has(
                        hitman_unlock_item, world.player
                    ),
                )

    for race in RACES.values():
        for medal in MEDAL_STRINGS.values():
            curr_key = f"{race} - {medal}"
            if curr_key in location_cache:
                add_rule(
                    world.get_location(curr_key),
                    lambda state, race_unlock_item=tss02_complete_item: state.has(
                        race_unlock_item, world.player
                    ),
                )


def set_completion_rules(world: SR2World) -> None:
    selected_gang_arcs = world.options.required_gang_arcs.value
    needed_completion_items: list[str] = []

    if RONIN_ARC_NAME in selected_gang_arcs:
        needed_completion_items.append(
            get_mission_complete_item_name(get_mission_by_key("rn11"))
        )

    if SAMEDI_ARC_NAME in selected_gang_arcs:
        needed_completion_items.append(
            get_mission_complete_item_name(get_mission_by_key("ss11"))
        )

    if BROTHERHOOD_ARC_NAME in selected_gang_arcs:
        needed_completion_items.append(
            get_mission_complete_item_name(get_mission_by_key("bh11"))
        )

    if ULTOR_EPILOGUE_ARC_NAME in selected_gang_arcs:
        needed_completion_items.append(
            get_mission_complete_item_name(get_mission_by_key("ep04"))
        )

    if world.options.include_secret_mission_as_goal.value == 1:
        needed_completion_items.append(
            get_mission_complete_item_name(ULTOR_SECRET_MISSION)
        )

    world.set_completion_rule(
        lambda state: all(
            state.has(item, world.player) for item in needed_completion_items
        )
    )


def mark_as_unlocked_after_intro(
        world: SR2World,
        location_cache: dict[str, Location],
        missions: list[Mission | Stronghold],
):
    tss04_complete_item = get_mission_complete_item_name(get_mission_by_key("tss04"))

    for mission in missions:
        if mission.name in location_cache:
            add_rule(
                world.get_location(mission.name),
                lambda state: state.has(
                    tss04_complete_item,
                    world.player,
                ),
            )

            if mission.creates_unlock_item:
                add_rule(
                    world.get_location(get_mission_complete_event_name(mission)),
                    lambda state: state.has(
                        tss04_complete_item,
                        world.player,
                    ),
                )


def mark_as_needing_all_strongholds(
        world: SR2World,
        location_cache: dict[str, Location],
        mission: Mission | Stronghold,
        strongholds: list[Stronghold],
):
    if mission.name in location_cache:
        add_rule(
            world.get_location(mission.name),
            lambda state: all(
                state.has(get_mission_complete_item_name(stronghold), world.player)
                for stronghold in strongholds
            ),
        )

        if mission.creates_unlock_item:
            add_rule(
                world.get_location(get_mission_complete_event_name(mission)),
                lambda state: all(
                    state.has(get_mission_complete_item_name(stronghold), world.player)
                    for stronghold in strongholds
                ),
            )


def set_respect_placement_rules(world: SR2World) -> None:
    for location in world.multiworld.get_unfilled_locations(world.player):
        if not getattr(location, "respect_safe", False):
            add_item_rule(
                location,
                lambda item: (
                        item.name != RESPECT_ITEM_NAME or item.player != world.player
                ),
            )
