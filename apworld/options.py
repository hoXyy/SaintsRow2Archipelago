from dataclasses import dataclass

from Options import (
    Choice,
    DeathLink,
    OptionGroup,
    OptionSet,
    PerGameCommonOptions,
    Range,
    Toggle,
)

RONIN_ARC_NAME = "Ronin Arc"
SAMEDI_ARC_NAME = "Sons of Samedi Arc"
BROTHERHOOD_ARC_NAME = "Brotherhood Arc"
ULTOR_EPILOGUE_ARC_NAME = "Ultor Arc"


class RequiredGangArcs(OptionSet):
    """
    Select which mission chain(s) you need to complete to finish your game.

    Available options: Ronin Arc, Sons of Samedi Arc, Brotherhood Arc, Ultor Arc
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
    Select which activity/collectible will be unlocked initially when you finish the mission Appointed Defender.
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
    option_chop_shop = 12
    option_hitman = 13
    option_cd = 14
    option_tags = 15
    option_stunt_jumps = 16
    option_car_races = 17
    option_bike_races = 18
    option_plane_races = 19
    option_helicopter_races = 20
    option_boat_races = 21
    option_taxi_diversion = 22
    option_paramedic_diversion = 23
    option_firefighter_diversion = 24
    option_towing_diversion = 25
    option_hoing_diversion = 26

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


class IncludeStilwaterCaverns(Toggle):
    """
    Whether to include the Stilwater Caverns Saints stronghold in the locations list.
    """

    display_name = "Include Stilwater Caverns stronghold"
    default = 0


class IncludeSecretMissionAsGoal(Toggle):
    """
    Whether to include the Ultor mission 'Revelation' as part of the completion goal.

    NOTE: Enabling this also adds the mission as a location.
    """

    display_name = (
        "Include completing the Ultor mission 'Revelation' in the completion goal"
    )
    default = 0


class IncludeStilwaterCavernsAsGoal(Toggle):
    """
    Whether to include the Stilwater Caverns Saints stronghold as part of the completion goal.

    NOTE: Enabling this also adds the stronghold as a location.
    """

    display_name = "Include Stilwater Caverns stronghold in the completion goal"
    default = 0


class IncludeCrowdControlLevels(Range):
    """
    Amount of Crowd Control levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Crowd Control Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeDemoDerbyLevels(Range):
    """
    Amount of Demolition Derby levels to include.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Demolition Derby Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeDrugTraffickingLevels(Range):
    """
    Amount of Drug Trafficking levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Drug Trafficking Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeEscortLevels(Range):
    """
    Amount of Escort levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Escort Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeFightClubLevels(Range):
    """
    Amount of Fight Club levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Fight Club Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeFraudLevels(Range):
    """
    Amount of Insurance Fraud levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Insurance Fraud Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeFuzzLevels(Range):
    """
    Amount of FUZZ levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included FUZZ Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeHeliAssaultLevels(Range):
    """
    Amount of Heli Assault levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Heli Assault Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeMayhemLevels(Range):
    """
    Amount of Mayhem levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Mayhem Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeSewageLevels(Range):
    """
    Amount of Septic Avenger levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Septic Avenger Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeSnatchLevels(Range):
    """
    Amount of Snatch levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Snatch Levels"
    default = 6

    range_start = 0
    range_end = 6


class IncludeTorchLevels(Range):
    """
    Amount of Trail Blazing levels to include. Applies to both locations of the activity individually.

    Set to 0 to disable.
    """

    display_name = "Amount of Included Trail Blazing Levels"
    default = 6

    range_start = 0
    range_end = 6


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


class IncludeCDs(Choice):
    """
    Whether to include CDs as location checks.

    If set to individual locations, each of the 50 CDs is its own location.

    Supported options are: off, individual, progressive
    """

    display_name = "Include CDs"

    option_off = 0
    option_individual = 1
    option_progressive = 2

    default = option_individual


class IncludeBoatRaces(Toggle):
    """
    Whether to include boat and jetski races as individual location checks.

    Each race will include a location for the Bronze, Silver and Gold medal.
    """

    display_name = "Include Boat and Jetski Races"
    default = 1


class IncludeBikeRaces(Toggle):
    """
    Whether to include bike races as individual location checks.

    Each race will include a location for the Bronze, Silver and Gold medal.
    """

    display_name = "Include Bike Races"
    default = 1


class IncludeCarRaces(Toggle):
    """
    Whether to include car races as individual location checks.

    Each race will include a location for the Bronze, Silver and Gold medal.
    """

    display_name = "Include Car Races"
    default = 1


class IncludeHeliRaces(Toggle):
    """
    Whether to include helicopter races as individual location checks.

    Each race will include a location for the Bronze, Silver and Gold medal.
    """

    display_name = "Include Helicopter Races"
    default = 1


class IncludePlaneRaces(Toggle):
    """
    Whether to include plane races as individual location checks.

    Each race will include a location for the Bronze, Silver and Gold medal.
    """

    display_name = "Include Plane Races"
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
    Percentage of bonus respect points to add to the item pool.

    Very useful to make it easier to find more respect to progress.
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

    default = 7


class AllowProgressionItemsOnHighStyleLevel(Toggle):
    """
    Allow progression items on high Style Level locations.

    NOTE: Getting to Style Level 9 and 10 requires A LOT of money! Keep that in mind when enabling this option!
    """

    display_name = "Allow progression items on high Style Level locations"
    default = 0


class IncludeTags(Choice):
    """
    Whether to include tags as location checks.

    If set to individual locations, each of the 50 tags is its own location.

    Supported options are: off, individual, progressive
    """

    display_name = "Include Tags"

    option_off = 0
    option_individual = 1
    option_progressive = 2

    default = option_individual


class MoneyFillerItemWeight(Range):
    """
    The weights of a money filler item to be included in the item pool.
    """

    display_name = "Money Filler Item Weight"

    range_start = 0
    range_end = 100

    default = 25


class WeaponFillerItemWeight(Range):
    """
    The weights of a weapon filler item to be included in the item pool.
    """

    display_name = "Weapon Filler Item Weight"

    range_start = 0
    range_end = 100

    default = 25


class VehicleFillerItemWeight(Range):
    """
    The weights of a vehicle filler item to be included in the item pool.
    """

    display_name = "Vehicle Filler Item Weight"

    range_start = 0
    range_end = 100

    default = 25


class MiscFillerItemWeight(Range):
    """
    The weights of a misc. filler item to be included in the item pool.
    """

    display_name = "Misc. Filler Item Weight"

    range_start = 0
    range_end = 100

    default = 25


class IncludeWeatherTrapItems(Toggle):
    """
    Whether to include permanent weather traps in the item pool.
    """

    display_name = "Include weather trap items"

    default = 1


class IncludeNotorietyTrapItems(Toggle):
    """
    Whether to include notoriety traps in the item pool.
    """

    display_name = "Include notoriety trap items"

    default = 1


class IncludeStuntJumps(Choice):
    """
    Whether to include stunt jumps in the location list.

    If set to individual locations, each stunt jump is its own location.
    One stunt jump is omitted if the Ultor Arc is not enabled due to that jump being designed to be done at the end of Pyramid Scheme.

    Supported options are: off, individual, progressive
    """

    display_name = "Include Stunt Jumps"

    option_off = 0
    option_individual = 1
    option_progressive = 2

    default = option_individual


class ProgressiveCDsLocationCount(Range):
    """
    Amount of progressive CD locations to include in the location list.

    Only applies with progressive CDs enabled.
    """

    display_name = "Progressive CDs Location Count"

    range_start = 1
    range_end = 50

    default = 50


class ProgressiveTagsLocationCount(Range):
    """
    Amount of progressive tags locations to include in the location list.

    Only applies with progressive tags enabled.
    """

    display_name = "Progressive Tags Location Count"

    range_start = 1
    range_end = 50

    default = 50


class ProgressiveStuntJumpsLocationCount(Range):
    """
    Amount of progressive stunt jump locations to include in the location list.

    Only applies with progressive stunt jumps enabled.
    """

    display_name = "Progressive Stunt Jumps Location Count"

    range_start = 1
    range_end = 79

    default = 79


class IncludeTaxiDiversionLevels(Range):
    """
    Amount of levels of the taxi diversion to include.

    Set to 0 to disable.
    """

    display_name = "Taxi Diversion Levels To Include"

    range_start = 0
    range_end = 10

    default = 10


class IncludeParamedicDiversionLevels(Range):
    """
    Amount of levels of the paramedic diversion to include.

    Set to 0 to disable.
    """

    display_name = "Paramedic Diversion Levels To Include"

    range_start = 0
    range_end = 10

    default = 10


class IncludeTowingDiversionLevels(Range):
    """
    Amount of levels of the towing diversion to include.

    Set to 0 to disable.
    """

    display_name = "Towing Diversion Levels To Include"

    range_start = 0
    range_end = 10

    default = 10


class IncludeHoingDiversionLevels(Range):
    """
    Amount of levels of the ho-ing diversion to include.

    Set to 0 to disable.
    """

    display_name = "Ho-ing Diversion Levels To Include"

    range_start = 0
    range_end = 10

    default = 10


class IncludeFirefighterDiversionLevels(Range):
    """
    Amount of levels of the firefighter diversion to include.

    Set to 0 to disable.
    """

    display_name = "Firefighter Diversion Levels To Include"

    range_start = 0
    range_end = 10

    default = 10


@dataclass
class SR2Options(PerGameCommonOptions):
    death_link: DeathLink
    required_gang_arcs: RequiredGangArcs
    starting_activity: StartingActivity
    include_secret_mission: IncludeSecretMission
    include_secret_mission_as_goal: IncludeSecretMissionAsGoal
    include_stilwater_caverns: IncludeStilwaterCaverns
    include_stilwater_caverns_as_goal: IncludeStilwaterCavernsAsGoal
    include_crowd_control_levels: IncludeCrowdControlLevels
    include_chop_shop: IncludeChopShop
    include_demo_derby_levels: IncludeDemoDerbyLevels
    include_drug_trafficking_levels: IncludeDrugTraffickingLevels
    include_escort_levels: IncludeEscortLevels
    include_fight_club_levels: IncludeFightClubLevels
    include_fuzz_levels: IncludeFuzzLevels
    include_fraud_levels: IncludeFraudLevels
    include_heli_assault_levels: IncludeHeliAssaultLevels
    include_hitman: IncludeHitman
    include_mayhem_levels: IncludeMayhemLevels
    include_sewage_levels: IncludeSewageLevels
    include_snatch_levels: IncludeSnatchLevels
    include_torch_levels: IncludeTorchLevels
    include_cds: IncludeCDs
    include_boat_races: IncludeBoatRaces
    include_bike_races: IncludeBikeRaces
    include_car_races: IncludeCarRaces
    include_heli_races: IncludeHeliRaces
    include_plane_races: IncludePlaneRaces
    include_tags: IncludeTags
    trap_chance: TrapChance
    bonus_respect_percentage: BonusRespectPercentage
    style_level_location_count: StyleLevelLocationCount
    allow_progression_items_on_high_style_level: AllowProgressionItemsOnHighStyleLevel
    money_filler_item_weight: MoneyFillerItemWeight
    weapon_filler_item_weight: WeaponFillerItemWeight
    vehicle_filler_item_weight: VehicleFillerItemWeight
    misc_filler_item_weight: MiscFillerItemWeight
    include_notoriety_trap_items: IncludeNotorietyTrapItems
    include_weather_trap_items: IncludeWeatherTrapItems
    include_stunt_jumps: IncludeStuntJumps
    progressive_cd_count: ProgressiveCDsLocationCount
    progressive_tags_count: ProgressiveTagsLocationCount
    progressive_jumps_count: ProgressiveStuntJumpsLocationCount
    include_taxi_diversion_levels: IncludeTaxiDiversionLevels
    include_paramedic_diversion_levels: IncludeParamedicDiversionLevels
    include_towing_diversion_levels: IncludeTowingDiversionLevels
    include_hoing_diversion_levels: IncludeHoingDiversionLevels
    include_firefighter_diversion_levels: IncludeFirefighterDiversionLevels


option_groups = [
    OptionGroup(
        "Game Options",
        [
            StartingActivity,
        ],
    ),
    OptionGroup(
        "Goal Options",
        [RequiredGangArcs, IncludeSecretMissionAsGoal, IncludeStilwaterCavernsAsGoal],
    ),
    OptionGroup(
        "Level-Based Activities Location Options",
        [
            IncludeCrowdControlLevels,
            IncludeDemoDerbyLevels,
            IncludeDrugTraffickingLevels,
            IncludeEscortLevels,
            IncludeFightClubLevels,
            IncludeFuzzLevels,
            IncludeFraudLevels,
            IncludeHeliAssaultLevels,
            IncludeMayhemLevels,
            IncludeSewageLevels,
            IncludeSnatchLevels,
            IncludeTorchLevels,
        ],
    ),
    OptionGroup(
        "Other Activity Location Options",
        [
            IncludeBoatRaces,
            IncludeBikeRaces,
            IncludeCarRaces,
            IncludeChopShop,
            IncludeHeliRaces,
            IncludeHitman,
            IncludePlaneRaces,
        ],
    ),
    OptionGroup(
        "Collectible Location Options",
        [
            IncludeCDs,
            IncludeStuntJumps,
            IncludeTags,
            ProgressiveCDsLocationCount,
            ProgressiveStuntJumpsLocationCount,
            ProgressiveTagsLocationCount,
        ],
    ),
    OptionGroup(
        "Diversion Location Options",
        [
            IncludeFirefighterDiversionLevels,
            IncludeTaxiDiversionLevels,
            IncludeTowingDiversionLevels,
            IncludeHoingDiversionLevels,
            IncludeParamedicDiversionLevels,
        ],
    ),
    OptionGroup(
        "Other Location Options",
        [
            StyleLevelLocationCount,
            AllowProgressionItemsOnHighStyleLevel,
            IncludeSecretMission,
            IncludeStilwaterCaverns,
        ],
    ),
    OptionGroup(
        "Item Include & Weights Options",
        [
            TrapChance,
            BonusRespectPercentage,
            IncludeWeatherTrapItems,
            IncludeNotorietyTrapItems,
            MoneyFillerItemWeight,
            WeaponFillerItemWeight,
            VehicleFillerItemWeight,
            MiscFillerItemWeight,
        ],
    ),
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
    StartingActivity.option_chop_shop: "Chop Shop",
    StartingActivity.option_hitman: "Hitman",
    StartingActivity.option_helicopter_races: "Heli Races",
    StartingActivity.option_boat_races: "Boat Races",
    StartingActivity.option_car_races: "Car Races",
    StartingActivity.option_bike_races: "Bike Races",
    StartingActivity.option_plane_races: "Plane Races",
    StartingActivity.option_cd: "CD",
    StartingActivity.option_tags: "Tags",
    StartingActivity.option_stunt_jumps: "Stunt Jumps",
    StartingActivity.option_taxi_diversion: "Taxi",
    StartingActivity.option_towing_diversion: "Towing",
    StartingActivity.option_firefighter_diversion: "Firefighter",
    StartingActivity.option_hoing_diversion: "Ho-ing",
    StartingActivity.option_paramedic_diversion: "Paramedic",
}
