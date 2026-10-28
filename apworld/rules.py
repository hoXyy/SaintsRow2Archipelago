from __future__ import annotations

import dataclasses
from typing import TYPE_CHECKING, ClassVar, cast

from typing_extensions import override

from BaseClasses import CollectionState, Location, LocationProgressType
from rule_builder.rules import Has, HasAll, Rule
from worlds.generic.Rules import add_item_rule, forbid_items

from .collectibles import (
    CD_MAPPING,
    MISSABLE_STUNT_JUMP_MAPPING,
    PROGRESSIVE_CD_LOCATIONS,
    PROGRESSIVE_STUNT_JUMP_LOCATIONS,
    PROGRESSIVE_TAGS_LOCATIONS,
    STUNT_JUMP_MAPPING,
    STYLE_LEVEL_LOCATIONS,
    TAGS_MAPPING,
)

if TYPE_CHECKING:
    from .world import SR2World

from .activities import (
    ACTIVITIES_LEVEL_BASED,
    ACTIVITY_STARTING_ACTIVITY_ITEM_MAPPING,
    CHOP_SHOP_LISTS,
    HITMAN_LISTS,
    MEDAL_STRINGS,
    RACES,
    is_activity_enabled_in_options,
)
from .diversions import (
    FIREFIGHTER_DIVERSION_LOCATIONS,
    HOING_DIVERSION_LOCATIONS,
    PARAMEDIC_DIVERSION_LOCATIONS,
    TAXI_DIVERSION_LOCATIONS,
    TOWING_DIVERSION_LOCATIONS,
)
from .items import (
    CD_UNLOCK_ITEM,
    CHOP_SHOP_UNLOCK_ITEM,
    EM01_UNLOCK_ITEM,
    FIREFIGHTER_DIVERSION_UNLOCK_ITEM,
    HITMAN_UNLOCK_ITEM,
    HOING_DIVERSION_UNLOCK_ITEM,
    LEVEL_ACTIVITY_UNLOCK_ITEMS,
    PARAMEDIC_DIVERSION_UNLOCK_ITEM,
    PERSISTENT_ACTIVITY_UNLOCK_ITEMS,
    RESPECT_ITEM_NAME,
    STUNT_JUMPS_UNLOCK_ITEM,
    TAGS_UNLOCK_ITEM,
    TAXI_DIVERSION_UNLOCK_ITEM,
    TOWING_DIVERSION_UNLOCK_ITEM,
    TSS_SH_CAVERNS_UNLOCK_ITEM,
    get_race_unlock_item,
)
from .missions import (
    BROTHERHOOD_CHAIN,
    MISSION_CHAINS,
    RONIN_CHAIN,
    SAMEDI_CHAIN,
    STILWATER_CAVERNS_STRONGHOLD,
    ULTOR_EPILOGUE_CHAIN,
    ULTOR_SECRET_MISSION,
    Mission,
    create_minimum_respect_table,
    get_mission_by_key,
    get_mission_complete_event_name,
    get_mission_complete_item_name,
)
from .options import (
    BROTHERHOOD_ARC_NAME,
    RONIN_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)

# Access rules are collected per location name and combined, then set once at the end,
# since world.set_rule replaces a location's rule rather than adding to it.
AccessRules = dict[str, Rule["SR2World"]]


@dataclasses.dataclass()
class HasEnoughUTRespect(Rule["SR2World"], game="Saints Row 2"):
    """Universal Tracker only: checks the player's unspent respect against what the mission's unchecked dependencies cost"""

    dependencies: tuple[tuple[int, int], ...]
    """(location id, required respect) for the mission and everything it depends on"""

    @override
    def _instantiate(self, world: SR2World) -> Rule.Resolved:
        return self.Resolved(self.dependencies, player=world.player)

    class Resolved(Rule.Resolved):
        dependencies: tuple[tuple[int, int], ...]

        # Depends on the tracker's checked locations and received respect, not on collected items
        force_recalculate: ClassVar[bool] = True

        @override
        def _evaluate(self, state: CollectionState) -> bool:
            world = cast("SR2World", state.multiworld.worlds[self.player])
            required_respect = sum(
                required
                for location_id, required in self.dependencies
                if location_id not in world.ut_checked_locations
            )
            return world.ut_available_respect >= required_respect


def set_all_rules(world: SR2World) -> None:
    access_rules: AccessRules = {}

    set_mission_rules(world, access_rules)
    set_activity_rules(world, access_rules)
    set_collectible_rules(world, access_rules)
    set_diversion_rules(world, access_rules)

    for location_name, rule in access_rules.items():
        world.set_rule(world.get_location(location_name), rule)

    set_completion_rules(world)
    set_respect_placement_rules(world)
    set_item_rules(world)
    set_style_level_rules(world)


def require(access_rules: AccessRules, location_name: str, rule: Rule) -> None:
    existing = access_rules.get(location_name)
    access_rules[location_name] = rule if existing is None else existing & rule


def require_for_mission(
    access_rules: AccessRules, mission: Mission, rule: Rule
) -> None:
    require(access_rules, mission.name, rule)

    if mission.creates_unlock_item:
        require(access_rules, get_mission_complete_event_name(mission), rule)


def set_mission_rules(world: SR2World, access_rules: AccessRules) -> None:
    location_cache = world.multiworld.regions.location_cache[world.player]
    minimum_respect_needed_table = create_minimum_respect_table()
    is_ut = getattr(world.multiworld, "generation_is_fake", False)

    # Generic respect requirement rules for both missions and strongholds
    for chain in MISSION_CHAINS:
        for mission in chain["missions"]:
            add_mission_predecessor_rule(access_rules, location_cache, mission)

            add_mission_respect_rules(
                access_rules,
                location_cache,
                mission,
                minimum_respect_needed_table[mission.key],
                is_ut=is_ut,
            )

        for stronghold in chain["strongholds"]:
            add_mission_predecessor_rule(access_rules, location_cache, stronghold)

            add_mission_respect_rules(
                access_rules,
                location_cache,
                stronghold,
                minimum_respect_needed_table[stronghold.key],
                is_ut=is_ut,
            )

    # Mission access rules

    for mission, unlock_item in (
        (ULTOR_SECRET_MISSION, EM01_UNLOCK_ITEM),
        (STILWATER_CAVERNS_STRONGHOLD, TSS_SH_CAVERNS_UNLOCK_ITEM),
    ):
        if mission.name not in location_cache:
            continue

        require_for_mission(access_rules, mission, Has(unlock_item))

    # Mark each arc finale as needing all strongholds done
    ronin_finale = get_mission_by_key("rn11")
    samedi_finale = get_mission_by_key("ss11")
    brotherhood_finale = get_mission_by_key("bh11")
    ultor_finale = get_mission_by_key("ep04")

    mark_as_needing_all_strongholds(
        access_rules, location_cache, ronin_finale, RONIN_CHAIN["strongholds"]
    )
    mark_as_needing_all_strongholds(
        access_rules, location_cache, samedi_finale, SAMEDI_CHAIN["strongholds"]
    )
    mark_as_needing_all_strongholds(
        access_rules,
        location_cache,
        brotherhood_finale,
        BROTHERHOOD_CHAIN["strongholds"],
    )
    mark_as_needing_all_strongholds(
        access_rules,
        location_cache,
        ultor_finale,
        ULTOR_EPILOGUE_CHAIN["strongholds"],
    )


def set_activity_rules(world: SR2World, access_rules: AccessRules) -> None:
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

                if level == 1:
                    prerequisite_item = unlock_item
                else:
                    prev_key = f"{activity} ({district}) - Level {level - 1}"
                    prerequisite_item = f"Item: {prev_key} Complete"

                level_rule = HasAll(tss02_complete_item, prerequisite_item)
                require(access_rules, curr_key, level_rule)

                if level < 6:
                    require(access_rules, f"Event: {curr_key} Complete", level_rule)

    for location in CHOP_SHOP_LISTS:
        vehicles = [value for d in CHOP_SHOP_LISTS[location] for value in d.values()]

        for vehicle in vehicles:
            curr_key = f"Chop Shop ({location}) - {vehicle}"
            if curr_key in location_cache:
                require(
                    access_rules,
                    curr_key,
                    HasAll(tss02_complete_item, CHOP_SHOP_UNLOCK_ITEM),
                )

    for location in HITMAN_LISTS:
        targets = [value for d in HITMAN_LISTS[location] for value in d.values()]

        for target in targets:
            curr_key = f"Hitman ({location}) - {target}"
            if curr_key in location_cache:
                require(
                    access_rules,
                    curr_key,
                    HasAll(tss02_complete_item, HITMAN_UNLOCK_ITEM),
                )

    for [race_key, race_name] in RACES.items():
        race_unlock_item = get_race_unlock_item(race_key)
        for medal in MEDAL_STRINGS.values():
            curr_key = f"{race_name} - {medal}"
            if curr_key in location_cache:
                require(
                    access_rules,
                    curr_key,
                    HasAll(tss02_complete_item, race_unlock_item),
                )

    # This target needs tags to be unlocked
    if "Hitman (Barrio) - Brad" in location_cache and bool(
        world.options.include_tags.value
    ):
        require(access_rules, "Hitman (Barrio) - Brad", Has(TAGS_UNLOCK_ITEM))

    # This target needs ho-ing to be unlocked
    if "Hitman (Barrio) - James" in location_cache and is_activity_enabled_in_options(
        world, "Ho-ing"
    ):
        require(
            access_rules, "Hitman (Barrio) - James", Has(HOING_DIVERSION_UNLOCK_ITEM)
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

    if world.options.include_stilwater_caverns_as_goal.value == 1:
        needed_completion_items.append(
            get_mission_complete_item_name(STILWATER_CAVERNS_STRONGHOLD)
        )

    world.set_completion_rule(HasAll(*needed_completion_items))


def set_collectible_rules(world: SR2World, access_rules: AccessRules) -> None:
    location_cache = world.multiworld.regions.location_cache[world.player]

    for location in [*CD_MAPPING.values(), *PROGRESSIVE_CD_LOCATIONS]:
        if location in location_cache:
            require(access_rules, location, Has(CD_UNLOCK_ITEM))

    for location in [*TAGS_MAPPING.values(), *PROGRESSIVE_TAGS_LOCATIONS]:
        if location in location_cache:
            require(access_rules, location, Has(TAGS_UNLOCK_ITEM))

    for location in [
        *(STUNT_JUMP_MAPPING | MISSABLE_STUNT_JUMP_MAPPING).values(),
        *PROGRESSIVE_STUNT_JUMP_LOCATIONS,
    ]:
        if location in location_cache:
            require(access_rules, location, Has(STUNT_JUMPS_UNLOCK_ITEM))

    if any(jump in location_cache for jump in MISSABLE_STUNT_JUMP_MAPPING.values()):
        pyramid_scheme = get_mission_by_key("ep02")
        require_for_mission(access_rules, pyramid_scheme, Has(STUNT_JUMPS_UNLOCK_ITEM))


def set_diversion_rules(world: SR2World, access_rules: AccessRules) -> None:
    location_cache = world.multiworld.regions.location_cache[world.player]

    for location in TAXI_DIVERSION_LOCATIONS:
        if location in location_cache:
            require(access_rules, location, Has(TAXI_DIVERSION_UNLOCK_ITEM))

    for location in TOWING_DIVERSION_LOCATIONS:
        if location in location_cache:
            require(access_rules, location, Has(TOWING_DIVERSION_UNLOCK_ITEM))

    for location in PARAMEDIC_DIVERSION_LOCATIONS:
        if location in location_cache:
            require(access_rules, location, Has(PARAMEDIC_DIVERSION_UNLOCK_ITEM))

    for location in HOING_DIVERSION_LOCATIONS:
        if location in location_cache:
            require(access_rules, location, Has(HOING_DIVERSION_UNLOCK_ITEM))

    for location in FIREFIGHTER_DIVERSION_LOCATIONS:
        if location in location_cache:
            require(access_rules, location, Has(FIREFIGHTER_DIVERSION_UNLOCK_ITEM))


def set_style_level_rules(world: SR2World):
    location_cache = world.multiworld.regions.location_cache[world.player]

    # Have a chance of a progression item to exist in the middle of the style level chain in addition to right at the end
    if world.random.random() < 0.5:
        candidates = [
            name
            for name in STYLE_LEVEL_LOCATIONS[3:6]  # Levels 4-6
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
            LEVEL_ACTIVITY_UNLOCK_ITEMS
            + PERSISTENT_ACTIVITY_UNLOCK_ITEMS
            + [EM01_UNLOCK_ITEM, TSS_SH_CAVERNS_UNLOCK_ITEM],
        )


def mark_as_needing_all_strongholds(
    access_rules: AccessRules,
    location_cache: dict[str, Location],
    mission: Mission,
    strongholds: list[Mission],
):
    if mission.name in location_cache:
        require_for_mission(
            access_rules,
            mission,
            HasAll(
                *(
                    get_mission_complete_item_name(stronghold)
                    for stronghold in strongholds
                )
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
    access_rules: AccessRules,
    location_cache: dict[str, Location],
    mission: Mission,
) -> None:
    if mission.name not in location_cache or mission.unlocked_by is None:
        return

    predecessor = get_mission_by_key(mission.unlocked_by)
    require_for_mission(
        access_rules, mission, Has(get_mission_complete_item_name(predecessor))
    )


def add_mission_respect_rules(
    access_rules: AccessRules,
    location_cache: dict[str, Location],
    mission: Mission,
    minimum_respect: int,
    *,
    is_ut: bool,
) -> None:
    if mission.name not in location_cache:
        return

    if is_ut:
        dependencies = tuple(
            (dependency.id, dependency.required_respect)
            for dependency in get_mission_respect_dependencies(mission)
        )
        require_for_mission(access_rules, mission, HasEnoughUTRespect(dependencies))
    elif minimum_respect > 0:
        require_for_mission(
            access_rules, mission, Has(RESPECT_ITEM_NAME, count=minimum_respect)
        )


def get_mission_respect_dependencies(mission: Mission) -> tuple[Mission, ...]:
    dependencies: dict[str, Mission] = {}

    def add_mission_and_predecessors(entry: Mission) -> None:
        if entry.key in dependencies:
            return

        dependencies[entry.key] = entry
        if entry.unlocked_by is not None:
            add_mission_and_predecessors(get_mission_by_key(entry.unlocked_by))

    add_mission_and_predecessors(mission)

    for chain in MISSION_CHAINS:
        if chain["missions"] and mission is chain["missions"][-1]:
            for stronghold in chain["strongholds"]:
                add_mission_and_predecessors(stronghold)
            break

    return tuple(dependencies.values())
