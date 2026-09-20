from __future__ import annotations

from typing import TYPE_CHECKING


from worlds.generic.Rules import add_rule, add_item_rule, forbid_items
from BaseClasses import Location, LocationProgressType
from .collectibles import CD_MAPPING, STYLE_LEVEL_LOCATIONS, TAGS_MAPPING

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
)
from .activities import (
    ACTIVITIES_LEVEL_BASED,
    CHOP_SHOP_LISTS,
    HITMAN_LISTS,
    RACES,
    MEDAL_STRINGS,
    ACTIVITY_STARTING_ACTIVITY_ITEM_MAPPING,
)
from .items import (
    RESPECT_ITEM_NAME,
    HITMAN_UNLOCK_ITEM,
    get_race_unlock_item,
    CHOP_SHOP_UNLOCK_ITEM,
    LEVEL_ACTIVITY_UNLOCK_ITEMS,
    PERSISTENT_ACTIVITY_UNLOCK_ITEMS,
    CD_UNLOCK_ITEM,
    TAGS_UNLOCK_ITEM,
)
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
    set_item_rules(world)
    set_collectible_rules(world)
    set_style_level_rules(world)


def set_mission_rules(world: SR2World) -> None:
    location_cache = world.multiworld.regions.location_cache[world.player]
    minimum_respect_needed_table = create_minimum_respect_table()

    # Generic respect requirement rules for both missions and strongholds
    for chain in MISSION_CHAINS:
        for mission in chain["missions"]:
            add_mission_predecessor_rule(world, location_cache, mission)

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
            add_mission_predecessor_rule(world, location_cache, stronghold)

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

    # Add required respect count to Revelation
    if ULTOR_SECRET_MISSION.name in location_cache:
        respect_needed = minimum_respect_needed_table[ULTOR_SECRET_MISSION.key]
        location = world.get_location(ULTOR_SECRET_MISSION.name)

        add_mission_predecessor_rule(world, location_cache, ULTOR_SECRET_MISSION)

        add_rule(
            location,
            lambda state, respect_needed=respect_needed: state.has(
                RESPECT_ITEM_NAME,
                world.player,
                respect_needed,
            ),
        )

    # Mark each arc finale as needing all strongholds done
    ronin_finale = get_mission_by_key("rn11")
    samedi_finale = get_mission_by_key("ss11")
    brotherhood_finale = get_mission_by_key("bh11")
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


def set_activity_rules(world: SR2World) -> None:
    location_cache = world.multiworld.regions.location_cache[world.player]
    tss02_complete_item = get_mission_complete_item_name(get_mission_by_key("tss02"))

    for activity in ACTIVITIES_LEVEL_BASED:
        activity_locations = [
            value for d in ACTIVITIES_LEVEL_BASED[activity] for value in d.values()
        ]

        unlock_item = ACTIVITY_STARTING_ACTIVITY_ITEM_MAPPING[activity]

        for district in activity_locations:
            for level in range(1, 7):
                curr_key = f"{activity} ({district}) - Level {level}"

                if curr_key not in location_cache:
                    continue

                curr_location = world.get_location(curr_key)

                if level == 1:
                    prerequisite_item = unlock_item

                    add_rule(
                        curr_location,
                        lambda state, tss02_item=tss02_complete_item, activity_unlock_item=unlock_item: state.has(
                            tss02_item, world.player
                        )
                        and state.has(activity_unlock_item, world.player),
                    )
                else:
                    prev_key = f"{activity} ({district}) - Level {level - 1}"
                    prerequisite_item = f"Item: {prev_key} Complete"

                add_rule(
                    curr_location,
                    lambda state, tss02_item=tss02_complete_item, level_unlock_item=prerequisite_item: state.has(
                        level_unlock_item, world.player
                    )
                    and state.has(tss02_item, world.player),
                )

                if level < 6:
                    completion_event = world.get_location(f"Event: {curr_key} Complete")

                    add_rule(
                        completion_event,
                        lambda state, tss02_item=tss02_complete_item, level_unlock_item=prerequisite_item: (
                            state.has(level_unlock_item, world.player)
                            and state.has(tss02_item, world.player)
                        ),
                    )

    for location in CHOP_SHOP_LISTS:
        vehicles = [value for d in CHOP_SHOP_LISTS[location] for value in d.values()]

        for vehicle in vehicles:
            curr_key = f"Chop Shop ({location}) - {vehicle}"
            if curr_key in location_cache:
                add_rule(
                    world.get_location(curr_key),
                    lambda state, tss02_item=tss02_complete_item, chop_shop_unlock_item=CHOP_SHOP_UNLOCK_ITEM: state.has(
                        tss02_item, world.player
                    )
                    and state.has(chop_shop_unlock_item, world.player),
                )

    for location in HITMAN_LISTS:
        targets = [value for d in HITMAN_LISTS[location] for value in d.values()]

        for target in targets:
            curr_key = f"Hitman ({location}) - {target}"
            if curr_key in location_cache:
                add_rule(
                    world.get_location(curr_key),
                    lambda state, tss02_item=tss02_complete_item, hitman_unlock_item=HITMAN_UNLOCK_ITEM: state.has(
                        tss02_item, world.player
                    )
                    and state.has(hitman_unlock_item, world.player),
                )

    for [race_key, race_name] in RACES.items():
        race_unlock_item = get_race_unlock_item(race_key)
        for medal in MEDAL_STRINGS.values():
            curr_key = f"{race_name} - {medal}"
            if curr_key in location_cache:
                add_rule(
                    world.get_location(curr_key),
                    lambda state, tss02_item=tss02_complete_item, race_activity_unlock_item=race_unlock_item: state.has(
                        tss02_item, world.player
                    )
                    and state.has(race_activity_unlock_item, world.player),
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


def set_collectible_rules(world: SR2World):
    location_cache = world.multiworld.regions.location_cache[world.player]

    for location in CD_MAPPING.values():
        if location in location_cache:
            add_rule(
                world.get_location(location),
                lambda state: state.has(CD_UNLOCK_ITEM, world.player),
            )

    for location in TAGS_MAPPING.values():
        if location in location_cache:
            add_rule(
                world.get_location(location),
                lambda state: state.has(TAGS_UNLOCK_ITEM, world.player),
            )


def set_style_level_rules(world: SR2World):
    location_cache = world.multiworld.regions.location_cache[world.player]

    # Too much money is needed to do these, so I'm not allowing putting progression items on these (unless the player wants it)
    if bool(world.options.allow_progression_items_on_high_style_level.value) == False:
        for location in [
            "Style Level - Level 8",
            "Style Level - Level 9",
            "Style Level - Level 10",
        ]:
            if location in location_cache:
                world_location = world.get_location(location)
                world_location.progress_type = LocationProgressType.EXCLUDED

    # Have a chance of a progression item to exist in the middle of the style level chain in addition to right at the end
    if world.random.random() < 0.5:
        candidates = [
            name
            for name in STYLE_LEVEL_LOCATIONS[3:6]  # Levels 4–6
            if name in location_cache
        ]

        if candidates:
            chosen = world.random.choice(candidates)
            world.get_location(chosen).progress_type = LocationProgressType.PRIORITY


def set_item_rules(world: SR2World):
    intro_missions = [
        get_mission_by_key("tss01").name,
    ]

    # I don't want unlock items to be sent out until Appointed Defender is done, and Appointed Defender has a hardcoded item on it so this'll work fine
    for mission in intro_missions:
        forbid_items(
            world.get_location(mission),
            LEVEL_ACTIVITY_UNLOCK_ITEMS + PERSISTENT_ACTIVITY_UNLOCK_ITEMS,
        )


def mark_as_needing_all_strongholds(
    world: SR2World,
    location_cache: dict[str, Location],
    mission: Mission,
    strongholds: list[Mission],
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


def add_mission_predecessor_rule(
    world: SR2World,
    location_cache: dict[str, Location],
    mission: Mission,
) -> None:
    if mission.name not in location_cache or mission.unlocked_by is None:
        return

    predecessor = get_mission_by_key(mission.unlocked_by)
    predecessor_item = get_mission_complete_item_name(predecessor)

    def predecessor_rule(
        state,
        required_item=predecessor_item,
    ) -> bool:
        return state.has(required_item, world.player)

    add_rule(
        world.get_location(mission.name),
        predecessor_rule,
    )

    if mission.creates_unlock_item:
        add_rule(
            world.get_location(get_mission_complete_event_name(mission)),
            predecessor_rule,
        )
