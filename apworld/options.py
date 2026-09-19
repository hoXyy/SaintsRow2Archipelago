from dataclasses import dataclass

from Options import OptionGroup, PerGameCommonOptions, Toggle, OptionSet, Range, Choice

RONIN_ARC_NAME = "Ronin Arc"
SAMEDI_ARC_NAME = "Sons of Samedi Arc"
BROTHERHOOD_ARC_NAME = "Brotherhood Arc"
ULTOR_EPILOGUE_ARC_NAME = "Ultor Epilogue"


class RequiredGangArcs(OptionSet):
    """
    Select which mission chain(s) you need to complete to finish your game.

    Available options: Ronin Arc, Sons of Samedi Arc, Brotherhood Arc, Ultor Epilogue
    """

    display_name = "Required Gang Arcs"

    valid_keys = {
        RONIN_ARC_NAME,
        SAMEDI_ARC_NAME,
        BROTHERHOOD_ARC_NAME,
        ULTOR_EPILOGUE_ARC_NAME,
    }

    default = {RONIN_ARC_NAME}


class StartingActivity(Choice):
    """
    Select which activity/collectible will be unlocked initially when you finish the mission Appointed Defender
    """

    display_name = "Starting Activity/Collectible"

    option_crowd_control = 0
    option_demo_derby = 1
    option_drug_trafficking = 2
    option_escort = 3
    option_fight_club = 4
    option_insurance_fraud = 5
    option_fuzz = 6
    option_heli_assault = 7
    option_mayhem = 8
    option_septic_avenger = 9
    option_snatch = 10
    option_trail_blazing = 11

    random_sentinel = -1

    default = option_crowd_control

    def __init__(self, value: int, randomized: bool = False):
        super().__init__(value)
        self.randomized = randomized

    @classmethod
    def from_text(cls, text: str):
        if text.lower() == "random":
            return cls(cls.default, randomized=True)

        return super().from_text(text)


class IncludeSecretMission(Toggle):
    """
    Whether to include the Ultor mission 'Revelation' in the locations list.
    """

    display_name = "Include the Ultor mission 'Revelation'"
    default = 0


class IncludeSecretMissionAsGoal(Toggle):
    """
    Whether to include the Ultor mission 'Revelation' as part of the completion goal.
    """

    display_name = "Include completing the Ultor mission 'Revelation' as a goal"
    default = 0


class IncludeCrowdControl(Toggle):
    """
    Whether to include both Crowd Control instances with each level as an individual location check.
    """

    display_name = "Include Crowd Control"
    default = 1


class IncludeDemoDerby(Toggle):
    """
    Whether to include Demolition Derby with each level as an individual location check.
    """

    display_name = "Include Demolition Derby"
    default = 1


class IncludeDrugTrafficking(Toggle):
    """
    Whether to include both Drug Trafficking instances with each level as an individual location check.
    """

    display_name = "Include Drug Trafficking"
    default = 1


class IncludeEscort(Toggle):
    """
    Whether to include both Escort instances with each level as an individual location check.
    """

    display_name = "Include Escort"
    default = 1


class IncludeFightClub(Toggle):
    """
    Whether to include both Fight Club instances with each level as an individual location check.
    """

    display_name = "Include Fight Club"
    default = 1


class IncludeFraud(Toggle):
    """
    Whether to include both Insurance Fraud instances with each level as an individual location check.
    """

    display_name = "Include Insurance Fraud"
    default = 1


class IncludeFuzz(Toggle):
    """
    Whether to include both FUZZ instances with each level as an individual location check.
    """

    display_name = "Include FUZZ"
    default = 1


class IncludeHeliAssault(Toggle):
    """
    Whether to include both Heli Assault instances with each level as an individual location check.
    """

    display_name = "Include Heli Assault"
    default = 1


class IncludeMayhem(Toggle):
    """
    Whether to include both Mayhem instances with each level as an individual location check.
    """

    display_name = "Include Mayhem"
    default = 1


class IncludeSewage(Toggle):
    """
    Whether to include both Septic Avenger instances with each level as an individual location check.
    """

    display_name = "Include Septic Avenger"
    default = 1


class IncludeSnatch(Toggle):
    """
    Whether to include both Snatch instances with each level as an individual location check.
    """

    display_name = "Include Snatch"
    default = 1


class IncludeTorch(Toggle):
    """
    Whether to include both Trail Blazing instances with each level as an individual location check.
    """

    display_name = "Include Trail Blazing"
    default = 1


class IncludeChopShop(Toggle):
    """
    Whether to include all Chop Shop lists with each vehicle as an individual location check.
    """

    display_name = "Include Chop Shop Lists"
    default = 1


class IncludeHitman(Toggle):
    """
    Whether to include all Hitman lists with each target as an individual location check.
    """

    display_name = "Include Hitman Lists"
    default = 1


class IncludeCDs(Toggle):
    """
    Whether to include all 50 CDs as individual location checks.
    """

    display_name = "Include CDs"
    default = 1


class IncludeRaces(Toggle):
    """
    Whether to include all races as individual location checks.

    Each race will include a location for the Bronze, Silver and Gold medal.
    """

    display_name = "Includes Races"
    default = 1


class TrapChance(Range):
    """
    Percentage chance that you'll drop a trap item.
    """

    display_name = "Trap Chance"

    range_start = 0
    range_end = 100

    default = 10


class BonusRespectPercentage(Range):
    """
    Percentage of bonus respect points to add to the item pool. Very useful to make it easier to find more respect to progress.
    """

    display_name = "Bonus Respect Percentage"

    range_start = 0
    range_end = 100

    default = 15


class StyleLevelLocationCount(Range):
    """
    Amount of player style level locations to include.
    """

    display_name = "Style Level Location Count"
    range_start = 0
    range_end = 10

    default = 10


@dataclass
class SR2Options(PerGameCommonOptions):
    required_gang_arcs: RequiredGangArcs
    starting_activity: StartingActivity
    include_secret_mission: IncludeSecretMission
    include_secret_mission_as_goal: IncludeSecretMissionAsGoal
    include_crowd_control: IncludeCrowdControl
    include_chop_shop: IncludeChopShop
    include_demo_derby: IncludeDemoDerby
    include_drug_trafficking: IncludeDrugTrafficking
    include_escort: IncludeEscort
    include_fight_club: IncludeFightClub
    include_fuzz: IncludeFuzz
    include_fraud: IncludeFraud
    include_heli_assault: IncludeHeliAssault
    include_hitman: IncludeHitman
    include_mayhem: IncludeMayhem
    include_sewage: IncludeSewage
    include_snatch: IncludeSnatch
    include_torch: IncludeTorch
    include_cds: IncludeCDs
    include_races: IncludeRaces
    trap_chance: TrapChance
    bonus_respect_percentage: BonusRespectPercentage
    style_level_location_count: StyleLevelLocationCount


option_groups = [
    OptionGroup("Goal Options", [RequiredGangArcs, IncludeSecretMissionAsGoal]),
    OptionGroup(
        "Activity & Collectibles Location Options",
        [
            StartingActivity,
            IncludeCDs,
            IncludeChopShop,
            IncludeCrowdControl,
            IncludeDemoDerby,
            IncludeDrugTrafficking,
            IncludeEscort,
            IncludeFightClub,
            IncludeFuzz,
            IncludeFraud,
            IncludeHeliAssault,
            IncludeHitman,
            IncludeMayhem,
            IncludeRaces,
            IncludeSewage,
            IncludeSnatch,
            IncludeTorch,
        ],
    ),
    OptionGroup(
        "Other Location Options",
        [
            StyleLevelLocationCount,
            IncludeSecretMission,
        ],
    ),
    OptionGroup("Item Options", [TrapChance, BonusRespectPercentage]),
]

ACTIVITY_STARTING_ACTIVITY_OPTION_MAPPING = {
    StartingActivity.option_crowd_control: "Crowd Control",
    StartingActivity.option_trail_blazing: "Trail Blazing",
    StartingActivity.option_snatch: "Snatch",
    StartingActivity.option_septic_avenger: "Septic Avenger",
    StartingActivity.option_mayhem: "Mayhem",
    StartingActivity.option_insurance_fraud: "Insurance Fraud",
    StartingActivity.option_heli_assault: "Heli Assault",
    StartingActivity.option_fuzz: "FUZZ",
    StartingActivity.option_fight_club: "Fight Club",
    StartingActivity.option_escort: "Escort",
    StartingActivity.option_drug_trafficking: "Drug Trafficking",
    StartingActivity.option_demo_derby: "Demolition Derby",
}
