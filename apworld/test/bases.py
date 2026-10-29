from BaseClasses import CollectionState
from test.bases import WorldTestBase

from ..options import StartingActivity
from ..world import SR2World

# The include option that has to be on for each starting activity choice.
STARTING_ACTIVITY_INCLUDE_OPTIONS = {
    StartingActivity.option_crowd_control: "include_crowd_control_levels",
    StartingActivity.option_demo_derby: "include_demo_derby_levels",
    StartingActivity.option_drug_trafficking: "include_drug_trafficking_levels",
    StartingActivity.option_escort: "include_escort_levels",
    StartingActivity.option_fight_club: "include_fight_club_levels",
    StartingActivity.option_insurance_fraud: "include_fraud_levels",
    StartingActivity.option_fuzz: "include_fuzz_levels",
    StartingActivity.option_heli_assault: "include_heli_assault_levels",
    StartingActivity.option_mayhem: "include_mayhem_levels",
    StartingActivity.option_septic_avenger: "include_sewage_levels",
    StartingActivity.option_snatch: "include_snatch_levels",
    StartingActivity.option_trail_blazing: "include_torch_levels",
    StartingActivity.option_chop_shop: "include_chop_shop",
    StartingActivity.option_hitman: "include_hitman",
    StartingActivity.option_cd: "include_cds",
    StartingActivity.option_tags: "include_tags",
    StartingActivity.option_stunt_jumps: "include_stunt_jumps",
    StartingActivity.option_car_races: "include_car_races",
    StartingActivity.option_bike_races: "include_bike_races",
    StartingActivity.option_plane_races: "include_plane_races",
    StartingActivity.option_helicopter_races: "include_heli_races",
    StartingActivity.option_boat_races: "include_boat_races",
    StartingActivity.option_taxi_diversion: "include_taxi_diversion_levels",
    StartingActivity.option_paramedic_diversion: "include_paramedic_diversion_levels",
    StartingActivity.option_firefighter_diversion: "include_firefighter_diversion_levels",
    StartingActivity.option_towing_diversion: "include_towing_diversion_levels",
    StartingActivity.option_hoing_diversion: "include_hoing_diversion_levels",
}


class SR2TestBase(WorldTestBase):
    game = "Saints Row 2"
    world: SR2World

    def get_fresh_state(self) -> CollectionState:
        return CollectionState(self.world.multiworld)
