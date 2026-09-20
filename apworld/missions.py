from __future__ import annotations

from dataclasses import dataclass

from typing import TypedDict, TYPE_CHECKING
from .options import (
    RONIN_ARC_NAME,
    BROTHERHOOD_ARC_NAME,
    SAMEDI_ARC_NAME,
    ULTOR_EPILOGUE_ARC_NAME,
)

if TYPE_CHECKING:
    from .world import SR2World


@dataclass
class Mission:
    id: int
    key: str
    name: str
    required_respect: int = 1
    unlocked_by: str | None = None
    creates_unlock_item: bool = True


class MissionChain(TypedDict):
    missions: list[Mission]
    strongholds: list[Mission]


TSS_INTRO_CHAIN: MissionChain = {
    "missions": [
        Mission(
            id=1, key="tss01", name="Saints Mission #01: Jailbreak", required_respect=0
        ),
        Mission(
            id=2,
            key="tss02",
            name="Saints Mission #02: Appointed Defender",
            required_respect=0,
            unlocked_by="tss01",
        ),
        Mission(
            id=3,
            key="tss03",
            name="Saints Mission #03: Down Payment",
            unlocked_by="tss02",
        ),
        Mission(
            id=4,
            key="tss04",
            name="Saints Mission #04: Three Kings",
            unlocked_by="tss03",
        ),
    ],
    "strongholds": [],
}


RONIN_CHAIN: MissionChain = {
    "missions": [
        Mission(
            id=6,
            key="rn01",
            name="Ronin Mission #01: Saint's Seven",
            unlocked_by="tss04",
        ),
        Mission(
            id=7,
            key="rn02",
            name="Ronin Mission #02: Laundry Day",
            unlocked_by="rn01",
        ),
        Mission(
            id=8, key="rn03", name="Ronin Mission #03: Road Rage", unlocked_by="rn02"
        ),
        Mission(
            id=9, key="rn04", name="Ronin Mission #04: Bleeding Out", unlocked_by="rn03"
        ),
        Mission(
            id=10,
            key="rn05",
            name="Ronin Mission #05: Orange Threat Level",
            unlocked_by="rn04",
        ),
        Mission(
            id=11,
            key="rn06",
            name="Ronin Mission #06: Kanto Connection",
            unlocked_by="rn05",
        ),
        Mission(
            id=12,
            key="rn07",
            name="Ronin Mission #07: Visiting Hours",
            unlocked_by="rn06",
        ),
        Mission(
            id=13,
            key="rn08",
            name="Ronin Mission #08: Room Service",
            unlocked_by="rn07",
        ),
        Mission(
            id=14,
            key="rn09",
            name="Ronin Mission #09: Rest in Peace",
            unlocked_by="rn08",
        ),
        Mission(
            id=15, key="rn10", name="Ronin Mission #10: Good D", unlocked_by="rn09"
        ),
        Mission(
            id=16,
            key="rn11",
            name="Ronin Final Mission: One Man's Junk...",
            unlocked_by="rn10",
        ),
    ],
    "strongholds": [
        Mission(
            id=17,
            key="sh_rn_stripclub",
            name="Ronin Stronghold: Suburbs Strip Club",
            unlocked_by="rn01",
        ),
        Mission(
            id=18,
            key="sh_rn_sciencemuseum",
            name="Ronin Stronghold: Humbolt Park Science Museum",
            unlocked_by="rn02",
        ),
        Mission(
            id=19,
            key="sh_rn_museum_pier",
            name="Ronin Stronghold: Amberbrook Museum Pier",
            unlocked_by="rn03",
        ),
        Mission(
            id=20,
            key="sh_rn_rec_center",
            name="Ronin Stronghold: New Hennequet Rec Center",
            unlocked_by="rn05",
        ),
    ],
}


SAMEDI_CHAIN: MissionChain = {
    "missions": [
        Mission(
            id=21,
            key="ss01",
            name="Sons of Samedi Mission #01: Got Dust, Will Travel",
            unlocked_by="tss04",
        ),
        Mission(
            id=22,
            key="ss02",
            name="Sons of Samedi Mission #02: File in the Cake",
            unlocked_by="ss01",
        ),
        Mission(
            id=23,
            key="ss03",
            name="Sons of Samedi Mission #03: Airborne Assault",
            unlocked_by="ss02",
        ),
        Mission(
            id=24,
            key="ss04",
            name="Sons of Samedi Mission #04: Veteran Child",
            unlocked_by="ss03",
        ),
        Mission(
            id=25,
            key="ss05",
            name="Sons of Samedi Mission #05: Burning Down The House",
            unlocked_by="ss04",
        ),
        Mission(
            id=26,
            key="ss06",
            name="Sons of Samedi Mission #06: Bad Trip",
            unlocked_by="ss05",
        ),
        Mission(
            id=27,
            key="ss07",
            name="Sons of Samedi Mission #07: Bonding Experience",
            unlocked_by="ss06",
        ),
        Mission(
            id=28,
            key="ss08",
            name="Sons of Samedi Mission #08: Riot Control",
            unlocked_by="ss07",
        ),
        Mission(
            id=29,
            key="ss09",
            name="Sons of Samedi Mission #09: Eternal Sunshine",
            unlocked_by="ss08",
        ),
        Mission(
            id=30,
            key="ss10",
            name="Sons of Samedi Mission #10: Assault on Precinct 31",
            unlocked_by="ss09",
        ),
        Mission(
            id=31,
            key="ss11",
            name="Sons of Samedi Final Mission: The Shopping Maul",
            unlocked_by="ss10",
        ),
    ],
    "strongholds": [
        Mission(
            id=32,
            key="sh_ss_trailerpark",
            name="Sons of Samedi Stronghold: Elysian Fields Trailer Park",
            unlocked_by="ss01",
        ),
        Mission(
            id=33,
            key="sh_ss_crackhouse",
            name="Sons of Samedi Stronghold: Bavogian Plaza Drug Labs",
            unlocked_by="ss02",
        ),
        Mission(
            id=34,
            key="sh_ss_student_union",
            name="Sons of Samedi Stronghold: Stilwater University Student Union",
            unlocked_by="ss03",
        ),
        Mission(
            id=35,
            key="sh_ss_fishingdock",
            name="Sons of Samedi Stronghold: Sunnyvale Gardens Fishing Dock",
            unlocked_by="ss05",
        ),
    ],
}


BROTHERHOOD_CHAIN: MissionChain = {
    "missions": [
        Mission(
            id=36,
            key="bh01",
            name="Brotherhood Mission #01: First Impressions",
            unlocked_by="tss04",
        ),
        Mission(
            id=37,
            key="bh02",
            name="Brotherhood Mission #02: Reunion Tour",
            unlocked_by="bh01",
        ),
        Mission(
            id=38,
            key="bh03",
            name="Brotherhood Mission #03: Waste Not Want Not",
            unlocked_by="bh02",
        ),
        Mission(
            id=39,
            key="bh04",
            name="Brotherhood Mission #04: Red Asphalt",
            unlocked_by="bh03",
        ),
        Mission(
            id=40,
            key="bh05",
            name="Brotherhood Mission #05: Bank Error in Your Favor",
            unlocked_by="bh04",
        ),
        Mission(
            id=41,
            key="bh06",
            name="Brotherhood Mission #06: Thank you and Goodnight!",
            unlocked_by="bh05",
        ),
        Mission(
            id=42,
            key="bh07",
            name="Brotherhood Mission #07: Retribution",
            unlocked_by="bh06",
        ),
        Mission(
            id=43,
            key="bh08",
            name="Brotherhood Mission #08: Jail Bait",
            unlocked_by="bh07",
        ),
        Mission(
            id=44,
            key="bh09",
            name="Brotherhood Mission #09: The Enemy of my Enemy",
            unlocked_by="bh08",
        ),
        Mission(
            id=45,
            key="bh10",
            name="Brotherhood Mission #10: The Siege",
            unlocked_by="bh09",
        ),
        Mission(
            id=46,
            key="bh11",
            name="Brotherhood Final Mission: Showdown",
            unlocked_by="bh10",
        ),
    ],
    "strongholds": [
        Mission(
            id=47,
            key="sh_bh_apartments",
            name="Brotherhood Stronghold: Sommerset Apartments",
            unlocked_by="bh02",
        ),
        Mission(
            id=48,
            key="sh_bh_chinatown",
            name="Brotherhood Stronghold: Imperial Square Pagodas",
            unlocked_by="bh03",
        ),
        Mission(
            id=49,
            key="sh_bh_docks",
            name="Brotherhood Stronghold: Poseidon Alley Docks",
            unlocked_by="bh04",
        ),
        Mission(
            id=50,
            key="sh_bh_airport",
            name="Brotherhood Stronghold: Wardill Airport Hangars",
            unlocked_by="bh05",
        ),
    ],
}


ULTOR_EPILOGUE_CHAIN: MissionChain = {
    "missions": [
        Mission(
            id=51,
            key="ep01",
            name="Ultor Mission #01: Picking a Fight",
            unlocked_by="tss04",
        ),
        Mission(
            id=52,
            key="ep02",
            name="Ultor Mission #02: Pyramid Scheme",
            unlocked_by="ep01",
        ),
        Mission(
            id=53,
            key="ep03",
            name="Ultor Mission #03: Salting the Earth... Again",
            unlocked_by="ep02",
        ),
        Mission(
            id=54,
            key="ep04",
            name="Ultor Final Mission: ...and a Better Life",
            unlocked_by="ep03",
        ),
    ],
    "strongholds": [
        Mission(
            id=55,
            key="sh_tss_ugmall",
            name="Ultor Stronghold: Rounds Square Shopping Center",
            unlocked_by="ep01",
        )
    ],
}

ULTOR_SECRET_MISSION = Mission(
    id=56, key="em01", name="Ultor Secret Mission: Revelation", unlocked_by="tss04"
)

STILWATER_CAVERNS_STRONGHOLD = Mission(
    id=5,
    key="sh_tss_caverns",
    name="Saints Stronghold: Stilwater Caverns",
    unlocked_by="tss03",
)

MISSION_CHAINS = [
    TSS_INTRO_CHAIN,
    RONIN_CHAIN,
    BROTHERHOOD_CHAIN,
    SAMEDI_CHAIN,
    ULTOR_EPILOGUE_CHAIN,
]

ALL_MISSIONS = (
    *(
        mission
        for chain in MISSION_CHAINS
        for mission in (*chain["missions"], *chain["strongholds"])
    ),
    ULTOR_SECRET_MISSION,
    STILWATER_CAVERNS_STRONGHOLD,
)


def get_mission_by_id(id: int) -> Mission:
    for chain in MISSION_CHAINS:
        for entry in (*chain["missions"], *chain["strongholds"]):
            if entry.id == id:
                return entry

    raise LookupError(f"Mission or stronghold with id {id} not found")


def get_mission_by_key(key: str) -> Mission:
    for chain in MISSION_CHAINS:
        for entry in (*chain["missions"], *chain["strongholds"]):
            if entry.key == key:
                return entry

    raise LookupError(f"Mission or stronghold with key {key} not found")


# Gets amount of missions that need respect that the current options will end up with, needed to create enough respect items.
def get_required_respect_count(world: SR2World) -> int:
    missions_to_complete = sum(
        mission.required_respect for mission in TSS_INTRO_CHAIN["missions"]
    )

    if RONIN_ARC_NAME in world.options.required_gang_arcs.value:
        missions_to_complete += sum(
            mission.required_respect for mission in RONIN_CHAIN["missions"]
        ) + len(RONIN_CHAIN["strongholds"])

    if BROTHERHOOD_ARC_NAME in world.options.required_gang_arcs.value:
        missions_to_complete += sum(
            mission.required_respect for mission in BROTHERHOOD_CHAIN["missions"]
        ) + len(BROTHERHOOD_CHAIN["strongholds"])

    if SAMEDI_ARC_NAME in world.options.required_gang_arcs.value:
        missions_to_complete += sum(
            mission.required_respect for mission in SAMEDI_CHAIN["missions"]
        ) + len(SAMEDI_CHAIN["strongholds"])

    if ULTOR_EPILOGUE_ARC_NAME in world.options.required_gang_arcs.value:
        missions_to_complete += sum(
            mission.required_respect for mission in ULTOR_EPILOGUE_CHAIN["missions"]
        ) + len(ULTOR_EPILOGUE_CHAIN["strongholds"])

    if world.options.include_secret_mission.value == 1:
        missions_to_complete += 1

    if world.options.include_stilwater_caverns.value == 1:
        missions_to_complete += 1

    return missions_to_complete


def get_mission_complete_event_name(mission: Mission) -> str:
    return f"Event: {mission.name} Complete"


def get_mission_complete_item_name(mission: Mission) -> str:
    return f"Item: {mission.name} Complete"


def create_minimum_respect_table() -> dict[str, int]:
    table: dict[str, int] = {}

    def add_chain(
        chain: MissionChain,
        *,
        base_respect: int = 0,
        strongholds_required_for_finale: bool = True,
    ) -> int:
        """
        Add a chain's minimum Respect requirements to `table`.

        Returns the total Respect needed to fully complete the chain.
        """

        missions = chain["missions"]
        strongholds = chain["strongholds"]

        running_respect = base_respect
        mission_requirements: dict[str, int] = {}

        for mission in missions:
            running_respect += mission.required_respect

            mission_requirements[mission.key] = running_respect
            table[mission.key] = running_respect

        for stronghold in strongholds:
            table[stronghold.key] = mission_requirements[stronghold.unlocked_by] + 1

        # Completing the whole chain means paying for every mission
        # and every required stronghold.
        total_chain_cost = (
            base_respect
            + sum(mission.required_respect for mission in missions)
            + len(strongholds)
        )

        if strongholds_required_for_finale and missions:
            # The final mission cannot be started until all strongholds
            # in this arc have been completed.
            table[missions[-1].key] = total_chain_cost

        return total_chain_cost

    add_chain(TSS_INTRO_CHAIN)
    add_chain(RONIN_CHAIN)
    add_chain(SAMEDI_CHAIN)
    add_chain(BROTHERHOOD_CHAIN)
    add_chain(ULTOR_EPILOGUE_CHAIN)

    # special entry for Revelation
    table[ULTOR_SECRET_MISSION.key] = 3
    table[STILWATER_CAVERNS_STRONGHOLD.key] = 2

    return table
