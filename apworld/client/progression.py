from __future__ import annotations

from ..activities import (
    ACTIVITIES_LEVEL_BASED,
    ACTIVITY_LEVEL_IDS,
    CHOP_SHOP_LISTS,
    HITMAN_LISTS,
    MEDAL_STRINGS,
    RACES,
)
from ..collectibles import (
    CD_IDS,
    CD_MAPPING,
    MISSABLE_STUNT_JUMP_MAPPING,
    PROGRESSIVE_CD_IDS,
    PROGRESSIVE_CD_LOCATIONS,
    PROGRESSIVE_STUNT_JUMP_IDS,
    PROGRESSIVE_STUNT_JUMP_LOCATIONS,
    PROGRESSIVE_TAGS_IDS,
    PROGRESSIVE_TAGS_LOCATIONS,
    STUNT_JUMP_IDS,
    STUNT_JUMP_MAPPING,
    STYLE_LEVEL_IDS,
    STYLE_LEVEL_LOCATIONS,
    TAGS_IDS,
    TAGS_MAPPING,
)
from ..diversions import (
    FIREFIGHTER_DIVERSION_IDS,
    FIREFIGHTER_DIVERSION_LOCATIONS,
    HOING_DIVERSION_IDS,
    HOING_DIVERSION_LOCATIONS,
    PARAMEDIC_DIVERSION_IDS,
    PARAMEDIC_DIVERSION_LOCATIONS,
    TAXI_DIVERSION_IDS,
    TAXI_DIVERSION_LOCATIONS,
    TOWING_DIVERSION_IDS,
    TOWING_DIVERSION_LOCATIONS,
)
from ..missions import (
    MISSION_CHAINS,
    STILWATER_CAVERNS_STRONGHOLD,
    ULTOR_SECRET_MISSION,
)

# Race medals are reported as 1 = bronze, 2 = silver, 3 = gold.
MEDALS_BY_RANK = ("bronze", "silver", "gold")


def _mission_locations() -> dict[str, int]:
    result: dict[str, int] = {}
    for chain in MISSION_CHAINS:
        for mission in (*chain["missions"], *chain["strongholds"]):
            result[mission.key] = mission.id
    result[ULTOR_SECRET_MISSION.key] = ULTOR_SECRET_MISSION.id
    result[STILWATER_CAVERNS_STRONGHOLD.key] = STILWATER_CAVERNS_STRONGHOLD.id
    return result


def _level_activities_locations() -> dict[str, tuple[str, str]]:
    result: dict[str, tuple[str, str]] = {}
    for activity, instances in ACTIVITIES_LEVEL_BASED.items():
        for instance in instances:
            for key, district in instance.items():
                result[key] = (activity, district)
    return result


def _list_activities_locations(
    prefix: str, lists: dict[str, list[dict[str, str]]]
) -> dict[str, int]:
    result: dict[str, int] = {}
    for district, entries in lists.items():
        for entry in entries:
            for key, target in entry.items():
                name = f"{prefix} ({district}) - {target}"
                result[key] = ACTIVITY_LEVEL_IDS[name]
    return result


MISSION_LOCATIONS = _mission_locations()
ACTIVITY_LOCATIONS = _level_activities_locations()
CHOP_SHOP_LOCATIONS = _list_activities_locations("Chop Shop", CHOP_SHOP_LISTS)
HITMAN_LOCATIONS = _list_activities_locations("Hitman", HITMAN_LISTS)

# Categories where completing the key checks exactly one location.
SINGLE_LOCATIONS: dict[str, dict[str, int]] = {
    "mission": MISSION_LOCATIONS,
    "hitman": HITMAN_LOCATIONS,
    "chop_shop": CHOP_SHOP_LOCATIONS,
    "cd": {key: CD_IDS[name] for key, name in CD_MAPPING.items()},
    "tags": {key: TAGS_IDS[name] for key, name in TAGS_MAPPING.items()},
    "stunt_jumps": {
        key: STUNT_JUMP_IDS[name]
        for key, name in (STUNT_JUMP_MAPPING | MISSABLE_STUNT_JUMP_MAPPING).items()
    },
}

# Categories whose count of collected keys also checks progressive locations.
PROGRESSIVE_LOCATIONS: dict[str, list[int]] = {
    "cd": [PROGRESSIVE_CD_IDS[name] for name in PROGRESSIVE_CD_LOCATIONS],
    "tags": [PROGRESSIVE_TAGS_IDS[name] for name in PROGRESSIVE_TAGS_LOCATIONS],
    "stunt_jumps": [
        PROGRESSIVE_STUNT_JUMP_IDS[name] for name in PROGRESSIVE_STUNT_JUMP_LOCATIONS
    ],
}

# Diversions report their completed level count, so level N is checked once
# the count reaches N.
DIVERSION_LEVEL_LOCATIONS: dict[str, list[int]] = {
    "taxi": [TAXI_DIVERSION_IDS[name] for name in TAXI_DIVERSION_LOCATIONS],
    "paramedic": [
        PARAMEDIC_DIVERSION_IDS[name] for name in PARAMEDIC_DIVERSION_LOCATIONS
    ],
    "firefighter": [
        FIREFIGHTER_DIVERSION_IDS[name] for name in FIREFIGHTER_DIVERSION_LOCATIONS
    ],
    "towing": [TOWING_DIVERSION_IDS[name] for name in TOWING_DIVERSION_LOCATIONS],
    "hoing": [HOING_DIVERSION_IDS[name] for name in HOING_DIVERSION_LOCATIONS],
}


class CollectibleCounts:
    """Distinct collectible keys reported during one AP session."""

    def __init__(self) -> None:
        self.collected: dict[str, set[str]] = {
            category: set() for category in PROGRESSIVE_LOCATIONS
        }

    def clear(self) -> None:
        for keys in self.collected.values():
            keys.clear()

    def locations_for_progress(
        self, category: str | None, key: str | None, current: int
    ) -> list[int]:
        """Return the progressive location ids reached after this event."""
        keys = self.collected.get(category) if category is not None else None
        if keys is None or key is None or not current:
            return []
        keys.add(key)
        return PROGRESSIVE_LOCATIONS[category][: len(keys)]


def locations_for_progress(
    category: str | None, key: str | None, previous: int, current: int
) -> list[int]:
    """Return the location ids that a progression event checks."""
    if current <= previous:
        return []

    if category in SINGLE_LOCATIONS:
        location = SINGLE_LOCATIONS[category].get(key)
        return [] if location is None else [location]
    if category == "activity" and key in ACTIVITY_LOCATIONS:
        return _activity_levels(key, previous, current)
    if category == "racing" and key in RACES:
        return _race_medals(key, current)
    if category == "style_level":
        return [STYLE_LEVEL_IDS[name] for name in STYLE_LEVEL_LOCATIONS[:current]]
    if category in DIVERSION_LEVEL_LOCATIONS:
        return DIVERSION_LEVEL_LOCATIONS[category][previous:current]
    return []


def _activity_levels(key: str, previous: int, current: int) -> list[int]:
    # Bit n of the value is set once level n + 1 is complete.
    activity, district = ACTIVITY_LOCATIONS[key]
    newly_completed = current & ~previous
    return [
        ACTIVITY_LEVEL_IDS[f"{activity} ({district}) - Level {level}"]
        for level in range(1, 7)
        if newly_completed & (1 << (level - 1))
    ]


def _race_medals(key: str, current: int) -> list[int]:
    # A medal also awards every lower medal.
    race = RACES[key]
    return [
        ACTIVITY_LEVEL_IDS[f"{race} - {MEDAL_STRINGS[medal]}"]
        for medal in MEDALS_BY_RANK[:current]
    ]
