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
    creates_unlock_item: bool = False


@dataclass
class Stronghold:
    id: int
    key: str
    name: str
    unlocked_by: str
    creates_unlock_item: bool = (
        True  # All strongholds are needed to start the final mission of a gang arc
    )


class MissionChain(TypedDict):
    missions: list[Mission]
    strongholds: list[Stronghold]


TSS_INTRO_CHAIN: MissionChain = {
    "missions": [
        Mission(1, "tss01", "Saints Mission #01: Jailbreak", 0),
        Mission(
            2,
            "tss02",
            "Saints Mission #02: Appointed Defender",
            0,
            creates_unlock_item=True,
        ),
        Mission(
            3, "tss03", "Saints Mission #03: Down Payment", creates_unlock_item=True
        ),
        Mission(
            4, "tss04", "Saints Mission #04: Three Kings", creates_unlock_item=True
        ),
    ],
    "strongholds": [
        Stronghold(
            5,
            "sh_tss_caverns",
            "Saints Stronghold: Stilwater Caverns",
            "tss03",
            creates_unlock_item=True,
        )
    ],
}


RONIN_CHAIN: MissionChain = {
    "missions": [
        Mission(
            6, "rn01", "Ronin Mission #01: Saint's Seven", creates_unlock_item=True
        ),
        Mission(7, "rn02", "Ronin Mission #02: Laundry Day", creates_unlock_item=True),
        Mission(8, "rn03", "Ronin Mission #03: Road Rage", creates_unlock_item=True),
        Mission(9, "rn04", "Ronin Mission #04: Bleeding Out"),
        Mission(
            10,
            "rn05",
            "Ronin Mission #05: Orange Threat Level",
            creates_unlock_item=True,
        ),
        Mission(
            11,
            "rn06",
            "Ronin Mission #06: Kanto Connection",
        ),
        Mission(
            12,
            "rn07",
            "Ronin Mission #07: Visiting Hours",
        ),
        Mission(13, "rn08", "Ronin Mission #08: Room Service"),
        Mission(14, "rn09", "Ronin Mission #09: Rest in Peace"),
        Mission(15, "rn10", "Ronin Mission #10: Good D"),
        Mission(
            16,
            "rn11",
            "Ronin Final Mission: One Man's Junk...",
            creates_unlock_item=True,
        ),
    ],
    "strongholds": [
        Stronghold(
            17, "sh_rn_stripclub", "Ronin Stronghold: Suburbs Strip Club", "rn01"
        ),
        Stronghold(
            18,
            "sh_rn_sciencemuseum",
            "Ronin Stronghold: Humbolt Park Science Museum",
            "rn02",
        ),
        Stronghold(
            19, "sh_rn_museum_pier", "Ronin Stronghold: Amberbrook Museum Pier", "rn03"
        ),
        Stronghold(
            20, "sh_rn_rec_center", "Ronin Stronghold: New Hennequet Rec Center", "rn05"
        ),
    ],
}


SAMEDI_CHAIN: MissionChain = {
    "missions": [
        Mission(
            21,
            "ss01",
            "Sons of Samedi Mission #01: Got Dust, Will Travel",
            creates_unlock_item=True,
        ),
        Mission(
            22,
            "ss02",
            "Sons of Samedi Mission #02: File in the Cake",
            creates_unlock_item=True,
        ),
        Mission(
            23,
            "ss03",
            "Sons of Samedi Mission #03: Airborne Assault",
            creates_unlock_item=True,
        ),
        Mission(24, "ss04", "Sons of Samedi Mission #04: Veteran Child"),
        Mission(
            25,
            "ss05",
            "Sons of Samedi Mission #05: Burning Down The House",
            creates_unlock_item=True,
        ),
        Mission(26, "ss06", "Sons of Samedi Mission #06: Bad Trip"),
        Mission(
            27,
            "ss07",
            "Sons of Samedi Mission #07: Bonding Experience",
        ),
        Mission(28, "ss08", "Sons of Samedi Mission #08: Riot Control"),
        Mission(
            29,
            "ss09",
            "Sons of Samedi Mission #09: Eternal Sunshine",
        ),
        Mission(
            30,
            "ss10",
            "Sons of Samedi Mission #10: Assault on Precinct 31",
        ),
        Mission(
            31,
            "ss11",
            "Sons of Samedi Final Mission: The Shopping Maul",
            creates_unlock_item=True,
        ),
    ],
    "strongholds": [
        Stronghold(
            32,
            "sh_ss_trailerpark",
            "Sons of Samedi Stronghold: Elysian Fields Trailer Park",
            "ss01",
        ),
        Stronghold(
            33,
            "sh_ss_crackhouse",
            "Sons of Samedi Stronghold: Bavogian Plaza Drug Labs",
            "ss02",
        ),
        Stronghold(
            34,
            "sh_ss_student_union",
            "Sons of Samedi Stronghold: Stilwater University Student Union",
            "ss03",
        ),
        Stronghold(
            35,
            "sh_ss_fishingdock",
            "Sons of Samedi Stronghold: Sunnyvale Gardens Fishing Dock",
            "ss05",
        ),
    ],
}


BROTHERHOOD_CHAIN: MissionChain = {
    "missions": [
        Mission(36, "bh01", "Brotherhood Mission #01: First Impressions"),
        Mission(
            37,
            "bh02",
            "Brotherhood Mission #02: Reunion Tour",
            creates_unlock_item=True,
        ),
        Mission(
            38,
            "bh03",
            "Brotherhood Mission #03: Waste Not Want Not",
            creates_unlock_item=True,
        ),
        Mission(
            39, "bh04", "Brotherhood Mission #04: Red Asphalt", creates_unlock_item=True
        ),
        Mission(
            40,
            "bh05",
            "Brotherhood Mission #05: Bank Error in Your Favor",
            creates_unlock_item=True,
        ),
        Mission(
            41,
            "bh06",
            "Brotherhood Mission #06: Thank you and Goodnight!",
        ),
        Mission(42, "bh07", "Brotherhood Mission #07: Retribution"),
        Mission(43, "bh08", "Brotherhood Mission #08: Jail Bait"),
        Mission(
            44,
            "bh09",
            "Brotherhood Mission #09: The Enemy of my Enemy",
        ),
        Mission(45, "bh10", "Brotherhood Mission #10: The Siege"),
        Mission(
            46, "bh11", "Brotherhood Final Mission: Showdown", creates_unlock_item=True
        ),
    ],
    "strongholds": [
        Stronghold(
            47,
            "sh_bh_apartments",
            "Brotherhood Stronghold: Sommerset Apartments",
            "bh02",
        ),
        Stronghold(
            48,
            "sh_bh_chinatown",
            "Brotherhood Stronghold: Imperial Square Pagodas",
            "bh03",
        ),
        Stronghold(
            49, "sh_bh_docks", "Brotherhood Stronghold: Poseidon Alley Docks", "bh04"
        ),
        Stronghold(
            50,
            "sh_bh_airport",
            "Brotherhood Stronghold: Wardill Airport Hangars",
            "bh05",
        ),
    ],
}


ULTOR_EPILOGUE_CHAIN: MissionChain = {
    "missions": [
        Mission(
            51,
            "ep01",
            "Ultor Epilogue Mission #01: Picking a Fight",
            creates_unlock_item=True,
        ),
        Mission(52, "ep02", "Ultor Epilogue Mission #02: Pyramid Scheme"),
        Mission(53, "ep03", "Ultor Epilogue Mission #03: Salting the Earth... Again"),
        Mission(
            54,
            "ep04",
            "Ultor Epilogue Final Mission: ...and a Better Life",
            creates_unlock_item=True,
        ),
    ],
    "strongholds": [
        Stronghold(
            55,
            "sh_tss_ugmall",
            "Ultor Epilogue Stronghold: Rounds Square Shopping Center",
            "ep01",
        )
    ],
}

ULTOR_SECRET_MISSION = Mission(
    56, "em01", "Ultor Secret Mission: Revelation", creates_unlock_item=True
)

MISSION_CHAINS = [
    TSS_INTRO_CHAIN,
    RONIN_CHAIN,
    BROTHERHOOD_CHAIN,
    SAMEDI_CHAIN,
    ULTOR_EPILOGUE_CHAIN,
]


def get_mission_by_id(id: int) -> Mission | Stronghold:
    for chain in MISSION_CHAINS:
        for entry in (*chain["missions"], *chain["strongholds"]):
            if entry.id == id:
                return entry

    raise LookupError(f"Mission or stronghold with id {id} not found")


def get_mission_by_key(key: str) -> Mission | Stronghold:
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
        # Need to add the Saints stronghold here, as it's needed to complete the game but not needed to complete any of the gang arcs
        missions_to_complete += len(TSS_INTRO_CHAIN["strongholds"])

        missions_to_complete += sum(
            mission.required_respect for mission in ULTOR_EPILOGUE_CHAIN["missions"]
        ) + len(ULTOR_EPILOGUE_CHAIN["strongholds"])

    if world.options.include_secret_mission.value == 1:
        missions_to_complete += 1

    return missions_to_complete


def get_mission_complete_event_name(mission: Mission | Stronghold) -> str:
    return f"Event: {mission.name} Complete"


def get_mission_complete_item_name(mission: Mission | Stronghold) -> str:
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

    # The intro is special: Stilwater Caverns is not required to
    # finish Three Kings, so don't include it in tss04's requirement.
    add_chain(
        TSS_INTRO_CHAIN,
        strongholds_required_for_finale=False,
    )

    # The three gang arcs are independent of one another.
    ronin_total = add_chain(RONIN_CHAIN)
    brotherhood_total = add_chain(BROTHERHOOD_CHAIN)
    samedi_total = add_chain(SAMEDI_CHAIN)

    # The epilogue cannot begin until the previous story arcs are done.
    pre_epilogue_respect = (
        sum(m.required_respect for m in TSS_INTRO_CHAIN["missions"])
        + len(TSS_INTRO_CHAIN["strongholds"])
        + ronin_total
        + brotherhood_total
        + samedi_total
    )

    add_chain(
        ULTOR_EPILOGUE_CHAIN,
        base_respect=pre_epilogue_respect,
    )

    # special entry for Revelation
    table[ULTOR_SECRET_MISSION.key] = 3

    return table
