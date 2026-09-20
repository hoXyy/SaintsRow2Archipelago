-- tss04.lua
-- SR2 mission script
-- 3/27/08

-- Global Variables --

-- This factor is applied to player damage in single player. 
SINGLE_PLAYER_DAMAGE_MULTIPLIER = 0.50

-- This factor is applied to player damage in co-op.
COOP_DAMAGE_MULTIPLIER = 0.75

CHARACTER_SHAUNDI					= "tss04_$Shaundi"
CHARACTER_CARLOS					= "tss04_$Carlos"
CHARACTER_PIERCE					= "tss04_$Pierce"

-- Index/Trigger --> Character, Checkpoing Name, Completed
Tss04_3_phase_table				= {{CHARACTER_SHAUNDI,	"Shaundi",	false, INVALID_THREAD_HANDLE, INVALID_THREAD_HANDLE}, 
											{CHARACTER_CARLOS,	"Carlos",	false, INVALID_THREAD_HANDLE, INVALID_THREAD_HANDLE}, 
											{CHARACTER_PIERCE,	"Pierce",	false, INVALID_THREAD_HANDLE, INVALID_THREAD_HANDLE}}

Tss04_recruit_table_index		= {[CHARACTER_SHAUNDI] = 1, [CHARACTER_CARLOS] = 2, [CHARACTER_PIERCE] = 3}

Tss04_3_phase_table_size		= 3

-- This needs to come after Shaundi, Carlos, and Pierce being defined...
Tss04_carlos_reacquaint			= {{"TSSP4_REACQUAINT_L1",				CHARACTER_CARLOS,	0.2},
											{"PLAYER_TSSP4_REACQUAINT_L2",	LOCAL_PLAYER,		0.2},
											{"TSSP4_REACQUAINT_L3",				CHARACTER_CARLOS,	0.2},
											{"PLAYER_TSSP4_REACQUAINT_L4",	LOCAL_PLAYER,		0.2},
											{"TSSP4_REACQUAINT_L5",				CHARACTER_CARLOS,	0.2},
											{"PLAYER_TSSP4_REACQUAINT_L6",	LOCAL_PLAYER,		0.2}}

Tss04_carlos_phone_call			= {{"CARLOS_TSSP4_RANDOM_CALL_01",	CELLPHONE_CHARACTER,	0},
											{"CARLOS_TSSP4_RANDOM_CALL_02",	CELLPHONE_CHARACTER,	1},
											{"CARLOS_TSSP4_RANDOM_CALL_03",	CELLPHONE_CHARACTER,	2}}

Tss04_shaundi_in_exchange		= {{"TSSP04_SHAUNDI_FINISHED_1",	CHARACTER_SHAUNDI, 0.25, "talk with respect" }}

Tss04_carlos_in_exchange		= {{"TSSP4_CARLOS_IN_L1",			CHARACTER_CARLOS,		0.25, "talk submissive" },
											{"PLAYER_TSSP4_CARLOS_IN_L2",	LOCAL_PLAYER,			rand_int(0.1, 0.25)},
											{"TSSP4_CARLOS_IN_L3",			CHARACTER_CARLOS,		rand_int(0.1, 0.25) }}

Tss04_pierce_in_exchange		= {{"TSSP4_PIERCE_IN_L1",			CHARACTER_PIERCE,		0.25, "talk yes" },
											{"PLAYER_TSSP4_PIERCE_IN_L2",	LOCAL_PLAYER,			rand_int(0.1, 0.25)},
											{"TSSP4_PIERCE_IN_L3",			CHARACTER_PIERCE,		rand_int(0.1, 0.25)}}

Tss04_in_saints_exchange		= {[CHARACTER_SHAUNDI]	= Tss04_shaundi_in_exchange, 
											[CHARACTER_CARLOS]	= Tss04_carlos_in_exchange, 
											[CHARACTER_PIERCE]	= Tss04_pierce_in_exchange}

Tss04_shaundi_back_exchange	= {{"PLAYER_TSSP4_SHAUNDI_BACK_L1",	LOCAL_PLAYER,			0.25},
											{"TSSP4_SHAUNDI_BACK_L2",			CHARACTER_SHAUNDI,	0.25, "talk yes" }}

Tss04_carlos_back_exchange		= {{"PLAYER_TSSP4_CARLOS_BACK_L1",	LOCAL_PLAYER,			0.25},
											{"TSSP4_CARLOS_BACK_L2",			CHARACTER_CARLOS,		0.25, "talk yes" }}

Tss04_pierce_back_exchange		= {{"PLAYER_TSSP4_PIERCE_BACK_L1",	LOCAL_PLAYER,			0.25},
											{"TSSP4_PIERCE_BACK_L2",			CHARACTER_PIERCE,		0.25, "talk yes" }}

Tss04_back_to_hq_exchange		= {[CHARACTER_SHAUNDI]	= Tss04_shaundi_back_exchange, 
											[CHARACTER_CARLOS]	= Tss04_carlos_back_exchange, 
											[CHARACTER_PIERCE]	= Tss04_pierce_back_exchange}

SHAUNDI_PERSONA_OVERRIDES = { { POT_SITUATIONS[POT_ATTACK], "SHAUNDI_TSSP4_ATTACK" },
										{ POT_SITUATIONS[POT_TAKE_DAMAGE], "SHAUNDI_TSSP4_TAKDAM" },
										{ POT_SITUATIONS[POT_PRAISED_BY_PC], "TSSP04_SHAUNDI_PRAISED" },
										{ POT_SITUATIONS[POT_TAUNTED_BY_PC], "TSSP04_SHAUNDI_TAUNT" },
										{ POT_SITUATIONS[POT_BARTER], "TSSP04_SHAUNDI_BARTER" },
										{ POT_SITUATIONS[POT_HIT_CAR], "TSSP04_SHAUNDI_HITCAR" },
										{ POT_SITUATIONS[POT_HIT_OBJ], "TSSP04_SHAUNDI_HITOBJ" },
										{ POT_SITUATIONS[POT_HIT_PED], "TSSP04_SHAUNDI_HITPED" } }

CARLOS_PERSONA_OVERRIDES = { { POT_SITUATIONS[POT_ATTACK], "CARLOS_TSSP4_ATTACK" },
									  { POT_SITUATIONS[POT_TAKE_DAMAGE], "CARLOS_TSSP4_TAKDAM" },
									  { POT_SITUATIONS[POT_PRAISED_BY_PC], "CARLOS_TSSP4_PRAISE" },
									  { POT_SITUATIONS[POT_TAUNTED_BY_PC], "CARLOS_TSSP4_TAUNT" },
									  { POT_SITUATIONS[POT_BARTER], "CARLOS_TSSP4_BARTER" },
									  { POT_SITUATIONS[POT_HIT_CAR], "CARLOS_TSSP4_HITCAR" },
									  { POT_SITUATIONS[POT_HIT_OBJ], "CARLOS_TSSP4_HITOBJ" },
									  { POT_SITUATIONS[POT_HIT_PED], "CARLOS_TSSP4_HITPED" } }

PIERCE_PERSONA_OVERRIDES = { { POT_SITUATIONS[POT_ATTACK], "PIERCE_TSSP4_ATTACK" },
									  { POT_SITUATIONS[POT_TAKE_DAMAGE], "PIERCE_TSSP4_TAKDAM" },
									  { POT_SITUATIONS[POT_PRAISED_BY_PC], "PIERCE_TSSP4_PRAISE" },
									  { POT_SITUATIONS[POT_TAUNTED_BY_PC], "PIERCE_TSSP4_TAUNT" },
									  { POT_SITUATIONS[POT_BARTER], "PIERCE_TSSP4_BARTER" },
									  { POT_SITUATIONS[POT_HIT_CAR], "PIERCE_TSSP4_HITCAR" },
									  { POT_SITUATIONS[POT_HIT_OBJ], "PIERCE_TSSP4_HITOBJ" },
									  { POT_SITUATIONS[POT_HIT_PED], "PIERCE_TSSP4_HITPED" },
									  { POT_SITUATIONS[POT_GRATS_PC], "PIERCE_TSSP4_GRATS" } }

Tss04_lieutenant_persona_overrides = { [CHARACTER_SHAUNDI] = SHAUNDI_PERSONA_OVERRIDES, 
													[CHARACTER_CARLOS] = CARLOS_PERSONA_OVERRIDES, 
													[CHARACTER_PIERCE] = PIERCE_PERSONA_OVERRIDES }

Tss04_mourners						= {"tss04_$c000", "tss04_$c000 (0)", "tss04_$c001", "tss04_$c001 (0)", "tss04_$c002"}
Tss04_trash							= {"tss04_$c003", "tss04_$c003 (0)", "tss04_$c003 (1)", "tss04_$c004", "tss04_$c004 (0)"}

Num_tss04_mourners				= sizeof_table(Tss04_mourners)
Num_tss04_trash					= sizeof_table(Tss04_trash)

Tss04_ronin							= {"tss04_$ronin_sl1", "tss04_$ronin_sl2", "tss04_$ronin_sl3", 
											"tss04_$ronin_lt1", "tss04_$ronin_sl2 (0)", "tss04_$ronin_sl3 (0)", 
											"tss04_$ronin_lt1 (0)", "tss04_$ronin_sl3 (1)", "tss04_$ronin_lt1 (1)"}
Num_tss04_ronin					= sizeof_table(Tss04_ronin)

Tss04_ronin_group1				= {"tss04_$ronin_lt1 (1)", "tss04_$ronin_sl3 (1)"} 
Tss04_ronin_group2				= {"tss04_$ronin_lt1 (0)", "tss04_$ronin_sl2 (0)", "tss04_$ronin_sl3 (0)"} 
Tss04_ronin_group3				= {"tss04_$ronin_lt1", "tss04_$ronin_sl1", "tss04_$ronin_sl2", "tss04_$ronin_sl3"} 

Num_tss04_ronin_group1			= sizeof_table(Tss04_ronin_group1)
Num_tss04_ronin_group2			= sizeof_table(Tss04_ronin_group2)
Num_tss04_ronin_group3			= sizeof_table(Tss04_ronin_group3)

Tss04_tow_trucks					= {"tss04_$towtruck1", "tss04_$towtruck2"}
Num_tss04_tow_trucks				= sizeof_table(Tss04_tow_trucks)

Tss01_tow_truck_entered			= {["tss04_$towtruck1"] = false, ["tss04_$towtruck2"] = false}
Tss04_carlos_has_phoned_buddy = false

Tss04_player1_kills				= 0
Tss04_player2_kills				= 0

Tss04_ronin_kills             = 0

Tss04_stunt_jumps					= {"tss04_$t002", "tss04_$t003", "tss04_$t004"}
Tss04_stunt_jumps_idx			= {["tss04_$t002"] = 1, ["tss04_$t003"] = 2, ["tss04_$t004"] = 3}
Num_tss04_stunt_jumps			= sizeof_table(Tss04_stunt_jumps)

Tss04_player1_jump				= 0
Tss04_player2_jump				= 0

Tss04_coop_winner					= ""

STUNT_JUMP_EFFECT					= "icon_race_empty"

LOCATION_START						= "mission_start_sr2_city_$tss04"
LOCATION_START_PLAYER_2			= "tss04_$remote_player_start"

IN_COOP								= false
PHASE_ACTIVE						= false
PLAYER_IN_STUNT_AREA				= false

GROUP_START_VEHICLES				= "tss04_$start_vehicles"
GROUP_STUNT_VEHICLES				= "tss04_$stunt_vehicles"
GROUP_TOWTRUCK_1					= "tss04_$towtruck1"
GROUP_TOWTRUCK_2					= "tss04_$towtruck2"
GROUP_TOWED_VEHICLE				= "tss04_$towed_vehicle"
GROUP_TOWED_VEHICLE_COOP		= "tss04_$towed_vehicle_coop"
GROUP_RONIN							= "tss04_$ronin"

STUNTS_INDEX						= 1
TOWTRUCK_INDEX						= 2
KILLING_INDEX						= 3
LT_ACTIVATION_DISTANCE			= 4

CHARACTER_INDEX					= 1
CHECKPOINT_INDEX					= 2
COMPLETED_INDEX					= 3
LOAD_THREAD_INDEX					= 4
MOVE_THREAD_INDEX					= 5

LOCAL_TOWED_VEHICLE				= "tss04_$v005"
REMOTE_TOWED_VEHICLE				= "tss04_$v008"
TOWED_VEHICLE_TRIGGER			= "tss04_$in_garage"
GARAGE_OPEN_TRIGGER				= "tss04_$open_garage"
TOWED_COMPLETE_TRIGGER			= "tss04_$tow_finished"
SUBURBS_TRIGGER					= "tss04_$suburbs"

GARAGE_DOOR_MOVER					= "garage_sr2_city_SU_mech_door"

TRIGGER_DISABLED_1				= "garage_sr2_city_$SU_mech_ext"
TRIGGER_DISABLED_2				= "garage_sr2_city_$SU_mech_store"

MISSION_FINISHED_TRIGGER		= "tss04_$finish"

START_PHASE_THREAD				= INVALID_THREAD_HANDLE
TOWTRUCK_REGENERATE				= INVALID_THREAD_HANDLE

CT_INTRO = "tssp04-01"
CT_OUTRO = "tssp04-02"

LIEUTENANT_LEAVE_VEHICLE_MAX_SPEED_MPH = 5

Correct_vehicle_hitched = { [LOCAL_PLAYER] = false, [REMOTE_PLAYER] = false }

function tss04_start(checkpoint, is_restart)
	-- Start trigger is hit...the activate button was hit 
	set_mission_author("Ryan Spencer")

	-- Check for coop being active
	if (coop_is_active()) then
		IN_COOP	= true
	end

	-- Reduce the amount of damage applied to the players
	if (IN_COOP) then
		character_set_damage_multiplier(REMOTE_PLAYER, COOP_DAMAGE_MULTIPLIER)
		character_set_damage_multiplier(LOCAL_PLAYER, COOP_DAMAGE_MULTIPLIER)
	else
		character_set_damage_multiplier(LOCAL_PLAYER, SINGLE_PLAYER_DAMAGE_MULTIPLIER)
	end

	notoriety_force_no_spawn("Ronin", true)
	ambient_gang_spawn_enable(false)
	notoriety_set_max("Police", 2)
	notoriety_set_max("Ronin", 2)

   mission_start_fade_out()
   if ( checkpoint == MISSION_START_CHECKPOINT ) then
	if (not is_restart) then
		cutscene_play( CT_INTRO, "", "", true )
	end
   end

	-- Check to see if we are starting from the beginning
	if (checkpoint == MISSION_START_CHECKPOINT) then
		-- Teleport the player(s) to where they will need to be
		teleport_coop(LOCATION_START, LOCATION_START_PLAYER_2)

		-- We need to create both of these groups since the mission is starting from the beginning
		group_create(GROUP_START_VEHICLES, true)
		group_create(GROUP_STUNT_VEHICLES, false)

		-- Always be looking to regenerate the tow trucks
		TOWTRUCK_REGENERATE = thread_new("tss04_regenerate_towtrucks")

		-- Add a minimap icon
		minimap_icon_add_navpoint(SUBURBS_TRIGGER, MINIMAP_ICON_LOCATION, "", SYNC_ALL)
		trigger_enable(SUBURBS_TRIGGER, true)
		on_trigger("tss04_recruit_lieutenants", SUBURBS_TRIGGER)

		-- Add a waypoint
		waypoint_add(SUBURBS_TRIGGER, SYNC_ALL)
	else
      -- There is at least one phase that still needs to be completed
      for i = 1, Tss04_3_phase_table_size, 1 do
         if (strstr(checkpoint, Tss04_3_phase_table[i][CHECKPOINT_INDEX])) then
            -- Mark the checkpoint as complete
            Tss04_3_phase_table[i][COMPLETED_INDEX] = true
         else
            -- Add a minimap icon
            --marker_add_npc( Tss04_3_phase_table[i][CHARACTER_INDEX], MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_PROTECT_ACQUIRE, SYNC_ALL )
            minimap_icon_add_navpoint(Tss04_3_phase_table[i][CHARACTER_INDEX], MINIMAP_ICON_PROTECT_ACQUIRE, "", SYNC_ALL)
            --mission_debug( "Start: added minimap icon for "..Tss04_3_phase_table[i][CHARACTER_INDEX], 20 )
            -- Create the load thread
            Tss04_3_phase_table[i][LOAD_THREAD_INDEX] = thread_new("tss04_load_lieutenant", Tss04_3_phase_table[i][CHARACTER_INDEX], 270.0)

            -- Check to see if we need to create any of the vehicle groups
            if (i == STUNTS_INDEX) then
               -- We need these vehicles for stunts
               group_create(GROUP_STUNT_VEHICLES, true)
            elseif (i == TOWTRUCK_INDEX) then
               -- Always be looking to regenerate the tow trucks
               TOWTRUCK_REGENERATE = thread_new("tss04_regenerate_towtrucks")
            end
         end
      end
	end

	-- Reset the garage door
	mesh_mover_reset(GARAGE_DOOR_MOVER)

	-- Disable the Rim Job triggers for Carlos' activity
	trigger_enable(TRIGGER_DISABLED_1, false)
	trigger_enable(TRIGGER_DISABLED_2, false)

   mission_start_fade_in()

	START_PHASE_THREAD = thread_new("tss04_start_phase_try")

	-- Tell the player(s) what to do
	if (checkpoint == MISSION_START_CHECKPOINT) then
		delay(2.0)
		-- Tell the player(s) to go to the suburbs
		mission_help_table("tss04_suburbs")
	elseif (checkpoint == "ShaundiCarlosPierce") then
		delay(2.0)
		-- Tell the player(s) to go back to the headquarters
		mission_help_table("tss04_back_to_hq")
	end
end

function tss04_cleanup()
	-- Cleanup mission here

	ambient_gang_spawn_enable(true)

	-- Reset IN_COOP in case mission end was triggered by a disconnect
	IN_COOP = coop_is_active()

	-- Reset the amount of damage applied to the players
	character_set_damage_multiplier(LOCAL_PLAYER, 1.0)
	if (IN_COOP) then
		character_set_damage_multiplier(REMOTE_PLAYER, 1.0)
	end

	-- Kill the start phase thread
	thread_kill(START_PHASE_THREAD)
	thread_kill(TOWTRUCK_REGENERATE)

	-- Remove the waypoint if it still exists
	waypoint_remove(SYNC_ALL)

	-- Cleanup the phase table
	for i = 1, Tss04_3_phase_table_size, 1 do
		local lieutenant_name = Tss04_3_phase_table[i][CHARACTER_INDEX]

      --marker_remove_npc( Tss04_3_phase_table[i][CHARACTER_INDEX], SYNC_ALL )
		minimap_icon_remove_navpoint( lieutenant_name, SYNC_ALL )
		-- Remove the on death
		on_death("", lieutenant_name )

		if (character_exists(lieutenant_name)) then
			for index, override in pairs( Tss04_lieutenant_persona_overrides[lieutenant_name] ) do
				persona_override_character_stop( lieutenant_name, override[1] )
			end
		end
	end

	-- Reset notoriety
	notoriety_reset("Police")
	notoriety_reset("Ronin")
	notoriety_force_no_spawn("Ronin", false)


	-- Remove the markers
	minimap_icon_remove_navpoint(SUBURBS_TRIGGER)

	marker_remove_group(GROUP_RONIN, SYNC_ALL)
	marker_remove_vehicle(LOCAL_TOWED_VEHICLE, SYNC_LOCAL)
 
	for index, truck_name in pairs( Tss04_tow_trucks ) do
		marker_remove_vehicle( truck_name, SYNC_ALL )
	end

	if (IN_COOP) then
		marker_remove_vehicle(REMOTE_TOWED_VEHICLE, SYNC_REMOTE)
	end
	marker_remove_trigger(TOWED_VEHICLE_TRIGGER, SYNC_ALL)

	-- Remove any callbacks
	for i = 1, Num_tss04_ronin, 1 do
		on_death("", Tss04_ronin[i])
	end

	if (IN_COOP) then
		on_random_human_killed("", "tss04")
	end

	on_vehicle_destroyed("", LOCAL_TOWED_VEHICLE)
	if (IN_COOP) then
		on_vehicle_destroyed("", REMOTE_TOWED_VEHICLE)
	end

	-- Enable the Rim Job triggers that were disabled for Carlos' activity
	trigger_enable(TRIGGER_DISABLED_1, true)
	trigger_enable(TRIGGER_DISABLED_2, true)

	-- Disable any triggers that may have been enabled
	trigger_enable(MISSION_FINISHED_TRIGGER, false)
	trigger_enable(TOWED_VEHICLE_TRIGGER, false)
	trigger_enable(GARAGE_OPEN_TRIGGER, false)
   trigger_type_enable( "car mechanic", true )

	-- Release the groups to the world		
	release_to_world(GROUP_START_VEHICLES)
	release_to_world(GROUP_STUNT_VEHICLES)
	release_to_world(GROUP_TOWTRUCK_1)
	release_to_world(GROUP_TOWTRUCK_2)
	release_to_world(GROUP_TOWED_VEHICLE)
	release_to_world(GROUP_TOWED_VEHICLE_COOP)
	release_to_world(GROUP_RONIN)

   hud_timer_stop( 0 )

   mission_cleanup_maybe_reenable_player_controls()
end

function tss04_success()
	-- Called when the mission has ended with success

	-- Post the news event
	radio_post_event("JANE_NEWS_TSSP04", true)

	-- Unlock heli activities. Both instances include cutscenes w/ Shaundi and Pierce who
	-- aren't introduced until this mission. Most activities are unlocked after tss02.
	--mission_unlock("heli_br")
	--mission_unlock("heli_tp")

	-- Let players recruit again
	party_set_recruitable(true)
end

function tss04_recruit_lieutenants()
	-- Disable this trigger
	trigger_enable(SUBURBS_TRIGGER, false)

	-- Remove a minimap icon
	minimap_icon_remove_navpoint(SUBURBS_TRIGGER, SYNC_ALL)
	-- Remove the waypoint
	waypoint_remove(SYNC_ALL)

	-- Tell the player(s) to recruit
	mission_help_table("tss04_recruit")

	-- Show the lieutenants
	for i = 1, Tss04_3_phase_table_size, 1 do
		-- Add a minimap icon
      --marker_add_npc( Tss04_3_phase_table[i][CHARACTER_INDEX], MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_PROTECT_ACQUIRE, SYNC_ALL )
		minimap_icon_add_navpoint(Tss04_3_phase_table[i][CHARACTER_INDEX], MINIMAP_ICON_PROTECT_ACQUIRE, "", SYNC_ALL)
      --mission_debug( "Recruit: added minimap icon for "..Tss04_3_phase_table[i][CHARACTER_INDEX], 20 )
		-- Create the load thread
		Tss04_3_phase_table[i][LOAD_THREAD_INDEX] = thread_new("tss04_load_lieutenant", Tss04_3_phase_table[i][CHARACTER_INDEX], 270.0)
	end

	-- We do not need these vehicles anymore =P
	release_to_world(GROUP_START_VEHICLES)
end

function tss04_load_lieutenant(human, dist)
	-- Now continuously look to load this human if we are close enough
	while (1) do
		thread_yield()

		-- Only process this if the player is looking for another phase to participate in
		if (not PHASE_ACTIVE) then
			local		load_human = false

			-- We need to make sure one of the players is close enough
			if (get_dist_char_to_nav(LOCAL_PLAYER, human) < dist) then
				load_human = true
			elseif (IN_COOP) then
				if (get_dist_char_to_nav(REMOTE_PLAYER, human) < dist) then
					load_human = true
				end
			end

			-- Is the human already loaded?
			local		human_loaded = group_is_loaded(human)

			-- Create the human or destory him
			if (load_human and not human_loaded) then
				group_create(human, true)
            ingame_effect_add_npc( human, INGAME_EFFECT_PROTECT_ACQUIRE, SYNC_ALL )
				-- Do not let the player recruit the lieutenants
				set_unrecruitable_flag(human, true)
			elseif (not load_human and human_loaded) then
				-- Since we are going to unload this character, remove his death function
				on_death("", human)
            ingame_effect_remove_npc( human, SYNC_ALL )
				group_destroy(human)
			end

			-- The lieutenants cannot die at anytime
			if ( human_loaded and character_is_dead( human ) == false ) then
				on_death("tss04_phase_lieutenant_died", human)
			end
		end
	end
end

function tss04_start_phase_try()
	-- Now continuously look for one of the lieutenants to recriut
	while (1) do
		thread_yield()

		-- Loop through each character
		for i = 1, Tss04_3_phase_table_size, 1 do
			local		human_is_loaded = character_check_resource_loaded(Tss04_3_phase_table[i][CHARACTER_INDEX])
			local		phase_is_complete = Tss04_3_phase_table[i][COMPLETED_INDEX]

			-- Make the phase can be activiated
			if (not PHASE_ACTIVE and not phase_is_complete and human_is_loaded) then
				local		activating_player = ""

				-- We are so we need to make sure the two players are close enough...
				if (get_dist_char_to_char(LOCAL_PLAYER, Tss04_3_phase_table[i][CHARACTER_INDEX]) < LT_ACTIVATION_DISTANCE) then
					activating_player = LOCAL_PLAYER
				elseif (IN_COOP) then
					if (get_dist_char_to_char(REMOTE_PLAYER, Tss04_3_phase_table[i][CHARACTER_INDEX]) < LT_ACTIVATION_DISTANCE) then
						activating_player = REMOTE_PLAYER
					end
				end

				if (not (activating_player == "")) then
					-- Try to start the phase associated with this character
					PHASE_ACTIVE = tss04_start_phase(activating_player, Tss04_3_phase_table[i][CHARACTER_INDEX])
				               
					if (not PHASE_ACTIVE) then
						-- Starting the phase failed so delay a little while
						delay(5.0)
					end
				end
			end
		end
	end
end

function tss04_start_phase(human, lieutenant)
	-- Check to see if we are in coop...
	if (IN_COOP) then
		-- We are so we need to make sure the two players are close enough...
		if (get_dist_char_to_char(LOCAL_PLAYER, REMOTE_PLAYER) > 8.0) then
			-- The players are too far apart, output a message to the users
			mission_help_table_nag("tss04_too_far_apart")
			-- The start phase failed
			return false
		end
	end

	-- Disable the other active triggers and remove their icons
	for i = 1, Tss04_3_phase_table_size, 1 do
		-- If the phase is complete then do nothing
		if (not Tss04_3_phase_table[i][COMPLETED_INDEX]) then
			-- Remove the minimap icon
         --marker_remove_npc( Tss04_3_phase_table[i][CHARACTER_INDEX], SYNC_ALL )
			minimap_icon_remove_navpoint(Tss04_3_phase_table[i][CHARACTER_INDEX], SYNC_ALL)

         --mission_debug( "Start phase: Removed marker from "..Tss04_3_phase_table[i][CHARACTER_INDEX], 20 )
		end
	end

   ingame_effect_remove_npc( lieutenant, SYNC_ALL )

	-- The lieutenant is now recruitable
	set_unrecruitable_flag(lieutenant, false)
	-- Add the character to the player's party
   npc_go_idle( lieutenant )

	-- The mission's already over. Don't bother trying to start the phase
	if ( character_is_dead( lieutenant ) ) then
		return
	end
	party_add(lieutenant, human)
   npc_leash_remove( lieutenant )

	-- Kill this parties load thread
	thread_kill(Tss04_3_phase_table[Tss04_recruit_table_index[lieutenant]][LOAD_THREAD_INDEX])

	-- Figure out what we need to do now
	--
	-- "tss04_$Shaundi" 
	-- "tss04_$Carlos"
	-- "tss04_$Pierce"
	--

	-- Override personas for this lieutenant
	for index, override in pairs( Tss04_lieutenant_persona_overrides[lieutenant] ) do
		persona_override_character_start( lieutenant, override[1], override[2] )
	end

	if (lieutenant == Tss04_3_phase_table[STUNTS_INDEX][CHARACTER_INDEX]) then
		-- Stunts
		thread_new("tss04_intro_line_and_objective", "TSSP04_SHAUNDI_SEEPLAYER_1", lieutenant, "tss04_shaundi_start", "tss04_stunts_phase")
	elseif (lieutenant == Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX]) then
		-- Towing
		thread_new("tss04_intro_line_and_objective", "CARLOS_TSSP4_IMPRESS_01", lieutenant, "tss04_carlos_start", "tss04_towtruck_phase")
	else
		-- Killing
		thread_new("tss04_intro_line_and_objective", "PIERCE_TSSP4_INTRO_01", lieutenant, "tss04_pierce_start", "tss04_killing_phase")
	end

	-- The start phase was setup correctly
	return true
end

function tss04_intro_line_and_objective(line, lieutenant, objective, setup)
	-- Play the audio line
	audio_play_for_character(line, lieutenant, "voice", false, true)
	-- Show the objective
	mission_help_table(objective)
	-- Spawn a thread to do the setup
	thread_new(setup)
end

function tss04_phase_lieutenant_died(human)
	if (Tss04_3_phase_table[STUNTS_INDEX][CHARACTER_INDEX] == human) then
		-- Shaundi
		mission_end_failure("tss04", "tss04_shaundi_died")
	elseif (Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX] == human) then
		-- Carlos
		mission_end_failure("tss04", "tss04_carlos_died")
	else
		-- Pierce
		mission_end_failure("tss04", "tss04_pierce_died")
	end
end

function tss04_phase_lieutenant_abandoned(human)
	if (Tss04_3_phase_table[STUNTS_INDEX][CHARACTER_INDEX] == human) then
		-- Shaundi
		mission_end_failure("tss04", "tss04_shaundi_abandoned")
	elseif (Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX] == human) then
		-- Carlos
		mission_end_failure("tss04", "tss04_carlos_abandoned")
	else
		-- Pierce
		mission_end_failure("tss04", "tss04_pierce_abandoned")
	end
end

function tss04_phase_completed(player, lieutenant)
	-- Clear the HUD of the current objectives
	hud_timer_stop(0)
	objective_text_clear(0)
	objective_text_clear(1)

   --mission_debug( lieutenant.." phase completed.", 20 )

	local		all_phases_complete = true
	-- Mark this part as finished
	for i = 1, Tss04_3_phase_table_size, 1 do
		-- Look for the matching human
		if (Tss04_3_phase_table[i][CHARACTER_INDEX] == lieutenant) then
			-- Mark completed
			Tss04_3_phase_table[i][COMPLETED_INDEX] = true
		end

		-- Are all phases complete
		if (not Tss04_3_phase_table[i][COMPLETED_INDEX]) then
			all_phases_complete = false
		end
   end

	-- If we are not finished then handle the other phases and dismiss the current party member
	if (not all_phases_complete) then
		-- Enable the other active triggers and remove their icons
		for i = 1, Tss04_3_phase_table_size, 1 do
			-- If the phase is complete then do nothing
			if (not Tss04_3_phase_table[i][COMPLETED_INDEX]) then
				-- Add a minimap icon
            --marker_add_npc( Tss04_3_phase_table[i][CHARACTER_INDEX], MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_PROTECT_ACQUIRE, SYNC_ALL )
				minimap_icon_add_navpoint(Tss04_3_phase_table[i][CHARACTER_INDEX], MINIMAP_ICON_PROTECT_ACQUIRE, "", SYNC_ALL)
            --mission_debug( "Phase completed: added minimap icon for "..Tss04_3_phase_table[i][CHARACTER_INDEX], 20 )
			end
		end

		-- Revive the follower if they are incapacitated
		if (follower_is_unconscious(lieutenant)) then
			npc_revive(lieutenant)
		end

		-- Remove the on death
		on_death("", lieutenant)
		-- Remove the on abandon
		on_dismiss("", lieutenant)
		-- Remove from the party
		party_dismiss(lieutenant)
		-- Don't allow this character to be recruitable
		set_unrecruitable_flag(lieutenant, true)
		-- Make them invulnerable so they can't be killed
		turn_invulnerable(lieutenant, false)

		-- Setup the checkpoint...
		local		checkpoint_string = ""

		for i = 1, Tss04_3_phase_table_size, 1 do
			-- If the phase is complete then add its signature
			if (Tss04_3_phase_table[i][COMPLETED_INDEX]) then
				checkpoint_string = checkpoint_string .. Tss04_3_phase_table[i][CHECKPOINT_INDEX]
			end
		end

		-- Instruct the player(s) what to do next
		if (lieutenant == Tss04_3_phase_table[STUNTS_INDEX][CHARACTER_INDEX]) then
			mission_help_table("tss04_shaundi_complete")
		elseif (lieutenant == Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX]) then
			mission_help_table("tss04_carlos_complete")
		else
			mission_help_table("tss04_pierce_complete")
		end

		-- Play the in Saints conversation
		audio_play_conversation(Tss04_in_saints_exchange[lieutenant])
		-- Have them pathfind back to their original positions...
		Tss04_3_phase_table[Tss04_recruit_table_index[lieutenant]][MOVE_THREAD_INDEX] = thread_new("tss04_lieutenant_done", lieutenant)
		-- Look for an opportunity to release the lieutenant
		thread_new("tss04_release_human_to_world", lieutenant, 100.0, true)

      notoriety_set( "brotherhood", 0 )
      notoriety_set( "ronin", 0 )
      notoriety_set( "samedi", 0 )
      notoriety_set( "police", 0 )

      thread_yield()

      -- Set the checkpoint string data
		mission_set_checkpoint(checkpoint_string)
	else
		-- Make them invulnerable so they can't be killed
		turn_invulnerable(lieutenant )
      turn_invulnerable( LOCAL_PLAYER )
      if ( coop_is_active() ) then
         turn_invulnerable( REMOTE_PLAYER )
      end
		-- Play the back to HQ conversation
		audio_play_conversation(Tss04_back_to_hq_exchange[lieutenant])

      -- End the mission with success
      mission_end_success( "tss04", CT_OUTRO, { LOCATION_START, LOCATION_START_PLAYER_2 } )
	end

	-- Handle coop reward stuff here based upon the phase completed... it is by lieutenant
	if (IN_COOP) then
		-- Give out the bonus...
		if (lieutenant == Tss04_3_phase_table[STUNTS_INDEX][CHARACTER_INDEX]) then
			-- Stunts reward
			cash_add(300, player)
		elseif (lieutenant == Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX]) then
			-- Tow truck reward
			cash_add(300, player)
		else
			-- Killing reward
			cash_add(300, player)
		end
	end

	-- The phase is no longer active
	PHASE_ACTIVE = false
end

function tss04_lieutenant_done(human)
   while ( character_is_in_vehicle( human ) ) do
      local cur_speed = get_vehicle_speed( human )
      if ( get_char_vehicle_is_in_air( human ) == false and cur_speed <= LIEUTENANT_LEAVE_VEHICLE_MAX_SPEED_MPH ) then
         delay( 2.0 )
         cur_speed = get_vehicle_speed( human )
         if ( get_char_vehicle_is_in_air( human ) == false and cur_speed <= LIEUTENANT_LEAVE_VEHICLE_MAX_SPEED_MPH ) then
            vehicle_exit( human )
         end
         delay( 2.0 )
      end
      thread_yield()
   end

	-- Have a human run back to their spawning position
	move_to_safe(human, human, 3, true, true)
	turn_to_orient(human, human)
end

function tss04_release_human_to_world(human, dist, kill_move_thread)
	-- Now continuously look to release the human to the world
	while (1) do
		thread_yield()

		local		human_in_range = false

		-- We need to make sure one of the players is close enough
		if (get_dist_char_to_nav(LOCAL_PLAYER, human) < dist) then
			human_in_range = true
		elseif (IN_COOP) then
			if (get_dist_char_to_nav(REMOTE_PLAYER, human) < dist) then
				human_in_range = true
			end
		end

		-- The human is no longer in range so release it...
		if (not human_in_range) then
			if (kill_move_thread) then
				-- Kill the move and orient thread
				thread_kill(Tss04_3_phase_table[Tss04_recruit_table_index[human]][MOVE_THREAD_INDEX])
			end
         mission_debug( "releasing "..human.." to world.", 3600 )
			-- Release to the world
         if ( character_is_released( human ) == false ) then
   			release_to_world(human)
         end
			return
		end
	end
end

function tss04_stunts_phase()
	-- Shaundi cannot be abandoned
	on_dismiss("tss04_phase_lieutenant_abandoned", Tss04_3_phase_table[STUNTS_INDEX][CHARACTER_INDEX])

	-- Add the stunts callbacks
	if (not IN_COOP) then
		objective_text(0, "tss04_stunt", Tss04_player1_jump, Num_tss04_stunt_jumps)

		-- Enable all of the stunt jump triggers and their callbacks
		for i = 1, Num_tss04_stunt_jumps, 1 do
			trigger_enable(Tss04_stunt_jumps[i], true)
			on_trigger("tss04_stunt_jump", Tss04_stunt_jumps[i])
		end
	else		
		-- In coop there is a timer
		hud_timer_set(0, 1000 * 60 * 3, "tss04_stunt_timer_expired")

		objective_text(0, "tss04_stunt", Tss04_player1_jump, Num_tss04_stunt_jumps, SYNC_LOCAL)
		objective_text(1, "tss04_stunt", Tss04_player2_jump, Num_tss04_stunt_jumps, SYNC_REMOTE)

		-- Enable all of the stunt jump triggers and their callbacks
		for i = 1, Num_tss04_stunt_jumps, 1 do
			trigger_enable(Tss04_stunt_jumps[i], true)
			on_trigger("tss04_stunt_jump_coop", Tss04_stunt_jumps[i])
		end
	end

	marker_add_trigger(Tss04_stunt_jumps[1], MINIMAP_ICON_LOCATION, STUNT_JUMP_EFFECT)

	-- We have a driving stunts tutorial
	if ( tutorial_completed( "driving_stunts" ) == false ) then
		tutorial_start("driving_stunts", 3000, true)
	end
end

function tss04_stunt_jump(human, trigger)
   -- If Shaundi is not in a vehicle...this will not count
   if (not character_is_in_vehicle(CHARACTER_SHAUNDI)) then
      return
   end

	-- Needs to be the local player...
	if (not (human == LOCAL_PLAYER)) then
		return
	end

	-- Get the trigger index
	local		index = Tss04_stunt_jumps_idx[trigger]
	
	-- Check to see if we went through a trigger the we are not suppose to
	if (index > (Tss04_player1_jump + 1)) then
		-- We cannot go through this one yet
		return
	end

	-- Increment the count
	Tss04_player1_jump = Tss04_player1_jump + 1

   if ( Tss04_player1_jump == 1 ) then
      if ( tutorial_completed( "hud_text" ) == false ) then
		   tutorial_start("hud_text", 15000, true)
      end
   end

	-- Check for player completion (removal of effect)
	marker_remove_trigger(Tss04_stunt_jumps[index], SYNC_LOCAL)
	if (Tss04_player1_jump < Num_tss04_stunt_jumps) then
		marker_add_trigger(Tss04_stunt_jumps[index + 1], MINIMAP_ICON_LOCATION, STUNT_JUMP_EFFECT)
	end

	-- Disable the trigger
	trigger_enable(Tss04_stunt_jumps[index], false)
	-- for_cutscene = false, blocking = false
	audio_play_for_character( "SYS_RACE_CHECKPOINT", human, "foley", false, false )


	-- Update the objective status
	objective_text(0, "tss04_stunt", Tss04_player1_jump, Num_tss04_stunt_jumps)

	-- Has player 1 or 2 reached the required cash amount?
	if (Tss04_player1_jump == Num_tss04_stunt_jumps) then
		-- We do not need these vehicles anymore =P
		release_to_world(GROUP_STUNT_VEHICLES)

		tss04_phase_completed(human, Tss04_3_phase_table[STUNTS_INDEX][CHARACTER_INDEX])
	end
end

function tss04_stunt_jump_coop(human, trigger)
	-- Needs to be the local or remote player...
	if (not (human == LOCAL_PLAYER) and not (human == REMOTE_PLAYER)) then
		return
	end

	-- Get the trigger index
	local		index = Tss04_stunt_jumps_idx[trigger]
	
	if (human == LOCAL_PLAYER) then
		-- Check to see if we went through a trigger the we are not suppose to
		if (index <= Tss04_player1_jump) then
			-- We already went through these
			return
		elseif (index > (Tss04_player1_jump + 1)) then
			-- We cannot go through this one yet
			return
		end

		-- Increment the count
		Tss04_player1_jump = Tss04_player1_jump + 1

		-- Check for player completion (removal of effect)
		marker_remove_trigger(Tss04_stunt_jumps[index], SYNC_LOCAL)
		if (Tss04_player1_jump < Num_tss04_stunt_jumps) then
			marker_add_trigger(Tss04_stunt_jumps[index + 1], MINIMAP_ICON_LOCATION, STUNT_JUMP_EFFECT, SYNC_LOCAL)
		end
	else
		-- Check to see if we went through a trigger the we are not suppose to
		if (index <= Tss04_player2_jump) then
			-- We already went through these
			return
		elseif (index > (Tss04_player2_jump + 1)) then
			-- We cannot go through this one yet
			return
		end

		-- Increment the count
		Tss04_player2_jump = Tss04_player2_jump + 1

		-- Check for player completion (removal of effect)
		marker_remove_trigger(Tss04_stunt_jumps[index], SYNC_REMOTE)
		if (Tss04_player2_jump < Num_tss04_stunt_jumps) then
			marker_add_trigger(Tss04_stunt_jumps[index + 1], MINIMAP_ICON_LOCATION, STUNT_JUMP_EFFECT, SYNC_REMOTE)
		end
	end
	-- for_cutscene = false, blocking = false
	audio_play_for_character( "SYS_RACE_CHECKPOINT", human, "foley", false, false )

	-- Check for trigger disable
	if ((Tss04_player1_jump >= index) and (Tss04_player2_jump >= index)) then
		-- Both players have passed through this trigger so disable it
		trigger_enable(Tss04_stunt_jumps[index], false)
	end

	-- Update the objective status
	objective_text(0, "tss04_stunt", Tss04_player1_jump, Num_tss04_stunt_jumps, SYNC_LOCAL)
	objective_text(1, "tss04_stunt", Tss04_player2_jump, Num_tss04_stunt_jumps, SYNC_REMOTE)

	-- Check to see if we need to set who the winner is
	if (Tss04_coop_winner == "") then
		if (Tss04_player1_jump == Num_tss04_stunt_jumps) then
			Tss04_coop_winner = LOCAL_PLAYER
		elseif (Tss04_player2_jump == Num_tss04_stunt_jumps) then
			Tss04_coop_winner = REMOTE_PLAYER
		end
	end

	-- Has player 1 or 2 reached the required cash amount?
	if ((Tss04_player1_jump == Num_tss04_stunt_jumps) and (Tss04_player2_jump == Num_tss04_stunt_jumps)) then
		-- We do not need these vehicles anymore =P
		release_to_world(GROUP_STUNT_VEHICLES)

		tss04_phase_completed(Tss04_coop_winner, Tss04_3_phase_table[STUNTS_INDEX][CHARACTER_INDEX])
		-- Reset the coop winner
		Tss04_coop_winner = ""
	end
end

function tss04_stunt_timer_expired()
	-- The stunt timer elapsed
	mission_end_failure("tss04", "tss04_stunt_timer")
end

function tss04_mark_tow_target( player_name )
   if ( player_name == LOCAL_PLAYER ) then
   	marker_add_vehicle(LOCAL_TOWED_VEHICLE, MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_VEHICLE_PROTECT_ACQUIRE, SYNC_LOCAL)
		waypoint_add(LOCAL_TOWED_VEHICLE, SYNC_LOCAL)
   elseif ( player_name == REMOTE_PLAYER ) then
		marker_add_vehicle(REMOTE_TOWED_VEHICLE, MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_VEHICLE_PROTECT_ACQUIRE, SYNC_REMOTE)
		waypoint_add(REMOTE_TOWED_VEHICLE, SYNC_REMOTE)
   end
end

function tss04_unmark_tow_target( player_name )
   if ( player_name == LOCAL_PLAYER ) then
   	marker_remove_vehicle(LOCAL_TOWED_VEHICLE, SYNC_LOCAL)
		waypoint_remove(SYNC_LOCAL)
   elseif ( player_name == REMOTE_PLAYER ) then
		marker_remove_vehicle(REMOTE_TOWED_VEHICLE, SYNC_REMOTE)
		waypoint_remove(SYNC_REMOTE)
   end
end

function tss04_towtruck_phase()
	-- Create the towed vehicle
	group_create(GROUP_TOWED_VEHICLE)
	if (IN_COOP) then
		group_create(GROUP_TOWED_VEHICLE_COOP)
	end

	-- Carlos cannot be abandoned
	on_dismiss("tss04_phase_lieutenant_abandoned", Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX])
   trigger_type_enable( "car mechanic", false )

	-- The towed vehicle cannot die
	on_vehicle_destroyed("tss04_towed_vehicle_destroyed", LOCAL_TOWED_VEHICLE)
	-- Set the hit points...the base hitpoints are weak
	set_max_hit_points(LOCAL_TOWED_VEHICLE, (get_max_hit_points(LOCAL_TOWED_VEHICLE) * 2.5))
	-- Add an on vehicle hitched callback
	on_vehicle_hitched("tss04_towed_vehicle_hitched", LOCAL_TOWED_VEHICLE)
	-- Add an on vehicle unhitched callback
	on_vehicle_unhitched("tss04_towed_vehicle_unhitched", LOCAL_TOWED_VEHICLE)
	-- Lock the vehicle towed vehicle
	set_unjackable_flag(LOCAL_TOWED_VEHICLE, true)

	if (IN_COOP) then
		-- Carlos needs to be independent
		follower_make_independent(Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX], true)
		-- We need to move Carlos out of the way
		thread_new("tss04_move_carlos", Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX])

		-- The towed vehicle cannot die
		on_vehicle_destroyed("tss04_towed_vehicle_destroyed", REMOTE_TOWED_VEHICLE)
		-- Set the hit points...the base hitpoints are weak
		set_max_hit_points(REMOTE_TOWED_VEHICLE, (get_max_hit_points(REMOTE_TOWED_VEHICLE) * 2.5))
		-- Add an on vehicle hitched callback
		on_vehicle_hitched("tss04_towed_vehicle_hitched", REMOTE_TOWED_VEHICLE)
		-- Add an on vehicle unhitched callback
		on_vehicle_unhitched("tss04_towed_vehicle_unhitched", REMOTE_TOWED_VEHICLE)
		-- Lock the vehicle towed vehicle
		set_unjackable_flag(REMOTE_TOWED_VEHICLE, true)
	end

	-- Show the towed vehicle
	group_show(GROUP_TOWED_VEHICLE)
	if (IN_COOP) then
		group_show(GROUP_TOWED_VEHICLE_COOP)
	end

	-- Enable the drop off trigger...
	on_trigger("tss04_towtruck_parked", TOWED_VEHICLE_TRIGGER)
	trigger_enable(TOWED_VEHICLE_TRIGGER, true)

   -- Start a new thread to watch for showing the tow message
	thread_new("tss04_tow_truck_message")

	-- Kill the regenerate thread...it is no longer needed
	thread_kill(TOWTRUCK_REGENERATE)

	for i = 1, Num_tss04_tow_trucks, 1 do
		if (vehicle_exists(Tss04_tow_trucks[i]) and not vehicle_hidden(Tss04_tow_trucks[i])) then
			-- Setup the on vehicle enter
			on_vehicle_enter("tss02_towtruck_entered", Tss04_tow_trucks[i])

         vehicle_infinite_mass( Tss04_tow_trucks[i], false )
         vehicle_prevent_explosion_fling( Tss04_tow_trucks[i], false )
         turn_vulnerable( Tss04_tow_trucks[i] )
			on_vehicle_destroyed("tss04_towtruck_destroyed_failure", Tss04_tow_trucks[i])
         set_unjackable_flag( Tss04_tow_trucks[i], false )

			if (not Tss01_tow_truck_entered[Tss04_tow_trucks[i]]) then
				-- Add a marker to the tow truck
            marker_add_vehicle( Tss04_tow_trucks[i], MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_INTERACT, SYNC_ALL )
			else 
				-- Setup the on vehicle exit
				on_vehicle_exit("tss02_towtruck_exited", Tss04_tow_trucks[i])
			end
		end
	end

	-- In coop, Carlos does not ride in a vehicle
	if (not IN_COOP) then
		-- Play the Carlos conversation
		thread_new("tss04_carlos_conversation")
	end
end

function tss04_carlos_conversation()
	delay(2.0)
	-- Play the reacquaintance conversation...in the tow truck
	audio_play_conversation_in_vehicle(Tss04_carlos_reacquaint)
end

function tss04_move_carlos(human)
	-- Move Carlos to the navpoint
	move_to_safe(human, "tss04_$n000", 3, true, false)
	-- Orient Carlos
	turn_to_orient(human, "tss04_$n000")
end

function tss04_tow_truck_message()
	local		local_player = false
	local		remote_player = false

	if (not IN_COOP) then
		remote_player = true
	end

	-- Check to see if we need to show the player the tow truck message
	while (not local_player or not remote_player) do
		thread_yield()

		if (not local_player and get_dist_char_to_vehicle(LOCAL_PLAYER, LOCAL_TOWED_VEHICLE) < 20.0) then
			mission_help_table_nag("tss04_tow_truck", nil, nil, SYNC_LOCAL)
			waypoint_remove(SYNC_LOCAL)
			local_player = true
		end

		if (not remote_player and get_dist_char_to_vehicle(REMOTE_PLAYER, REMOTE_TOWED_VEHICLE) < 20.0) then
			mission_help_table_nag("tss04_tow_truck", nil, nil, SYNC_REMOTE)
			waypoint_remove(SYNC_REMOTE)
			remote_player = true
		end
	end
end

function tss04_towed_vehicle_destroyed()
	-- The towed vehicle was destroyed
	mission_end_failure("tss04", "tss04_towed_destroyed")
end

function tss04_towed_vehicle_hitched(hitched, towtruck)
	local		is_local_player = character_is_in_vehicle(LOCAL_PLAYER, towtruck)
	local		is_remote_player = character_is_in_vehicle(REMOTE_PLAYER, towtruck)

	-- As a safety check verify that one of these guys is in the towtruck
	if (not is_local_player and not is_remote_player) then
		return
	end

	local		towed_vehicle = REMOTE_TOWED_VEHICLE
	local		sync_type = SYNC_REMOTE
   local player_name = REMOTE_PLAYER

	-- Are we looking for the remote towed vehicle?
	if (is_local_player) then
		towed_vehicle = LOCAL_TOWED_VEHICLE
		sync_type = SYNC_LOCAL
      player_name = LOCAL_PLAYER
	end 

	-- It must be the correct towtruck and towed vehicle
	if (not (hitched == towed_vehicle)) then
		return
	end

   Correct_vehicle_hitched[player_name] = true

	-- Remove the towed vehicle marker
   tss04_unmark_tow_target( player_name )
	
	-- Add the dropoff trigger as the objective
   marker_add_trigger(TOWED_VEHICLE_TRIGGER, MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_LOCATION, sync_type)

	-- Add callback if it is a scripted tow truck
	if (not (towtruck == nil)) then
		-- We might add something here in the future...just don't want to remove it for now
	end

	-- Add a waypoint back to the garage
	waypoint_add(TOWED_VEHICLE_TRIGGER, sync_type)

	-- Have the appropriate group attack
	if (is_local_player) then
		for i = 1, Num_tss04_mourners, 1 do
			if (not character_is_dead(Tss04_mourners[i])) then
				set_blitz_flag(Tss04_mourners[i], true)
				set_cant_flee_flag(Tss04_mourners[i], true) 
				attack(Tss04_mourners[i], LOCAL_PLAYER)

				-- Look for an opportunity to release the mourners
				thread_new("tss04_release_human_to_world", Tss04_mourners[i], 60.0, false)
			end
		end

		-- In coop, Carlos does not ride in a vehicle
		if (not IN_COOP and not Tss04_carlos_has_phoned_buddy) then
			-- Carlos will now phone his buddy
			thread_new("tss04_carlos_phones_buddy")
			-- Mark it as having been played...
			Tss04_carlos_has_phoned_buddy = true
		end
	elseif (is_remote_player) then
		for i = 1, Num_tss04_trash, 1 do
			if (not character_is_dead(Tss04_trash[i])) then
				set_blitz_flag(Tss04_trash[i], true)
				set_cant_flee_flag(Tss04_trash[i], true) 
				attack(Tss04_trash[i], REMOTE_PLAYER)

				-- Look for an opportunity to release the trash
				thread_new("tss04_release_human_to_world", Tss04_trash[i], 60.0, false)
			end
		end
	end

	-- Delay a moment so that police notoriety doesn't happen right away
	delay(5.0)
	-- Once we are considered hitched, set police notoriety
	notoriety_set_max("Police", 1)
	notoriety_set_min("Police", 1)
end

function tss04_carlos_phones_buddy()
	-- Carlos has a one man conversation with his buddy...
	audio_play_conversation(Tss04_carlos_phone_call, OUTGOING_CALL)
end

function tss04_towed_vehicle_unhitched(unhitched, towtruck)
	local		is_local_player = character_is_in_vehicle(LOCAL_PLAYER, towtruck)
	local		is_remote_player = character_is_in_vehicle(REMOTE_PLAYER, towtruck)

	-- As a safety check verify that one of these guys is in the towtruck
	if (not is_local_player and not is_remote_player) then
		return
	end

	local		towed_vehicle = REMOTE_TOWED_VEHICLE
	local		sync_type = SYNC_REMOTE
   local player_name = REMOTE_PLAYER

	-- Are we looking for the local towed vehicle?
	if (is_local_player) then
		towed_vehicle = LOCAL_TOWED_VEHICLE
		sync_type = SYNC_LOCAL
      player_name = LOCAL_PLAYER
	end 

	-- It must be the correct towtruck and towed vehicle
	if (not (unhitched == towed_vehicle)) then
		return
	end

   Correct_vehicle_hitched[player_name] = false

	-- Add the marker
	waypoint_remove( sync_type )
   tss04_mark_tow_target( player_name )
   
	-- Remove the garage open trigger as the objective
   marker_remove_trigger( TOWED_VEHICLE_TRIGGER, sync_type )

	-- Add callback if it is a scripted tow truck
	if (not (towtruck == nil)) then
		-- We might add something here in the future...just don't want to remove it for now
	end

	-- Add callback if it is a scripted tow truck
	if (not (towtruck == nil)) then
		-- We might add something here in the future...just don't want to remove it for now
	end
end

function tss04_towtruck_destroyed(vehicle)
	-- Release this vehicle (group) to the world so it can regenerate
	release_to_world(vehicle)
end

function tss02_towtruck_entered(human, vehicle)
	-- Needs to be the local or remote player...
	if (not (human == LOCAL_PLAYER) and not (human == REMOTE_PLAYER)) then
		return
	end

	local		sync_type = SYNC_REMOTE

	-- Are we looking for the local towed vehicle?
	if (human == LOCAL_PLAYER) then
		sync_type = SYNC_LOCAL
	end 

   -- Guide the player to the objective now that he's in the truck
	for index, truck_name in pairs( Tss04_tow_trucks ) do
		marker_remove_vehicle( truck_name, sync_type )
	end
   if ( Correct_vehicle_hitched[human] ) then
      marker_add_trigger(TOWED_VEHICLE_TRIGGER, MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_LOCATION, sync_type)
   else
      tss04_mark_tow_target( human )
   end

	on_vehicle_exit("tss02_towtruck_exited", vehicle)
end

function tss02_towtruck_exited(human, vehicle)
	-- Needs to be the local or remote player...
	if (not (human == LOCAL_PLAYER) and not (human == REMOTE_PLAYER)) then
		return
	end

	local		sync_type = SYNC_REMOTE

	-- Are we looking for the local towed vehicle?
	if (human == LOCAL_PLAYER) then
		sync_type = SYNC_LOCAL
	end 

	-- Remove the objective for now and add a marker for the tow truck
   if ( Correct_vehicle_hitched[human] ) then
      marker_remove_trigger(TOWED_VEHICLE_TRIGGER, sync_type)
      waypoint_remove( sync_type )
   else
      tss04_unmark_tow_target( human )
   end
	marker_add_vehicle(vehicle, MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_VEHICLE_PROTECT_ACQUIRE, sync_type)
end

function tss04_towtruck_destroyed_failure()
	-- The mission ends in failure since the tow truck is required
	mission_end_failure("tss04", "tss04_tow_truck_destroyed")
end

function tss04_regenerate_towtrucks()
	-- Only use one tow truck unless we are in coop
	local	num_tow_truck = 1

	if (IN_COOP) then
		num_tow_truck = Num_tss04_tow_trucks
	end

	-- While in this mission
	while (1) do
		-- Loop through the towtrucks and check for revival
		for i = 1, num_tow_truck, 1 do
			-- When a group is no longer loaded and is dead, recreate the group
			if (not group_is_loaded(Tss04_tow_trucks[i])) then
				-- Create the group, hidden
				group_create_hidden(Tss04_tow_trucks[i])
			end

			-- If the current towtruck is hidden then see if we can reveal it
			if (vehicle_hidden(Tss04_tow_trucks[i])) then
				local local_can_show		= not navpoint_in_player_fov(Tss04_tow_trucks[i], LOCAL_PLAYER, 3.0)
				local remote_can_show	= true

				if (IN_COOP) then
					remote_can_show = not navpoint_in_player_fov(Tss04_tow_trucks[i], REMOTE_PLAYER, 3.0)
				end

				if (local_can_show and remote_can_show) then
					-- Show the towtruck
					group_show(Tss04_tow_trucks[i])
					-- Setup the death callback
					on_vehicle_destroyed("tss04_towtruck_destroyed", Tss04_tow_trucks[i])
					-- Setup our enter callback
					on_vehicle_enter("tss02_towtruck_enter", Tss04_tow_trucks[i])
					-- Adjust the hitpoints
					set_max_hit_points(Tss04_tow_trucks[i], get_max_hit_points(Tss04_tow_trucks[i]) * 1.5)

               vehicle_infinite_mass( Tss04_tow_trucks[i], true )
               vehicle_prevent_explosion_fling( Tss04_tow_trucks[i], false )
               turn_invulnerable( Tss04_tow_trucks[i] )
               set_unjackable_flag( Tss04_tow_trucks[i], true )
				end
			end

			-- Yield and check the next tow truck in the next frame
			thread_yield()
		end
	end
end

function tss04_towtruck_parked(human)
	local		is_local_player = (human == LOCAL_PLAYER)
	local		is_remote_player = (human == REMOTE_PLAYER)

   --mission_debug( "tow truck parked called" )

	-- Must be one of the players and must be in the tow truck with the hitched vehicle
	if (not is_local_player and not is_remote_player) then
      --mission_debug( "fell through because was not player" )
		return
	elseif (not (get_char_vehicle_special_type(human) == VST_TOW_TRUCK)) then
		-- Must be in a tow truck
      --mission_debug( "fell through because player was not in tow truck" )
		return
	else
		local		towed_vehicle = REMOTE_TOWED_VEHICLE
		local		sync_type = SYNC_REMOTE

		-- Are we looking for the local towed vehicle?
		if (is_local_player) then
			towed_vehicle = LOCAL_TOWED_VEHICLE
			sync_type = SYNC_LOCAL
		end

		-- Must be towing the correct vehicle
		if (not (get_char_vehicle_towed_vehicle(human) == towed_vehicle)) then
			return
		end

		-- Reset the police min notoriety
		notoriety_reset("Police")
		notoriety_set_max("Police", 2)

      local cur_tow_truck_name = get_char_vehicle_name( human )
      for index, tow_truck_name in pairs( Tss04_tow_trucks ) do
         if ( cur_tow_truck_name == tow_truck_name ) then
            on_vehicle_enter( "", cur_tow_truck_name )
            on_vehicle_exit( "", cur_tow_truck_name )
            marker_remove_vehicle( cur_tow_truck_name )
		      set_unjackable_flag( cur_tow_truck_name, true )
         end
      end

		-- Force the vehicle to stop
		vehicle_stop(human)
		-- Force the player out of the vehicle...don't allow the vehicle to be enterable
		vehicle_exit(human, true)
		-- Lock the vehicle towed vehicle
		set_unjackable_flag(towed_vehicle, true)

		-- Remove the trigger effect from the person who has made it here
		marker_remove_trigger(TOWED_VEHICLE_TRIGGER, sync_type)
		-- Direct the player to his next goal of being outside of the garage
		minimap_icon_add_trigger(TOWED_COMPLETE_TRIGGER, MINIMAP_ICON_LOCATION, "", sync_type)

      -- Release the mourners and trash to the world so it can be populated
      for i = 1, Num_tss04_mourners, 1 do
         if (not character_is_released(Tss04_mourners[i])) then
            release_to_world(Tss04_mourners[i])
         end
      end

      if (IN_COOP) then
         -- Only do these when in coop
         for i = 1, Num_tss04_trash, 1 do
            if (not character_is_released(Tss04_trash[i])) then
               release_to_world(Tss04_trash[i])
            end
         end
      end

      tss04_towtruck_complete( human )
	end
end

function tss04_towtruck_complete(human)
	local		is_local_player = (human == LOCAL_PLAYER)
	local		is_remote_player = (human == REMOTE_PLAYER)

	-- Must be one of the players and must be in the tow truck with the hitched vehicle
	if (not is_local_player and not is_remote_player) then
		return
	elseif (character_is_in_vehicle(human)) then
		return
	end

	-- Kill the regenerate thread...it is no longer needed
	thread_kill(TOWTRUCK_REGENERATE)

	-- Remove the callback functions
	on_vehicle_destroyed("", LOCAL_TOWED_VEHICLE)
	-- Remove the on vehicle hitched callback
	on_vehicle_hitched("", LOCAL_TOWED_VEHICLE)
	-- Remove the on vehicle unhitched callback
	on_vehicle_unhitched("", LOCAL_TOWED_VEHICLE)
	-- Remove the towed vehicle marker
	marker_remove_vehicle(LOCAL_TOWED_VEHICLE, SYNC_LOCAL)

   for index, tow_truck_name in pairs( Tss04_tow_trucks ) do
      marker_remove_vehicle( tow_truck_name, SYNC_ALL )
      on_vehicle_exit( "", tow_truck_name )
      on_vehicle_enter( "", tow_truck_name )
      set_unjackable_flag( tow_truck_name, false )
   end

	-- We do not need these vehicles anymore
	release_to_world(GROUP_TOWTRUCK_1)
	release_to_world(GROUP_TOWTRUCK_2)
	release_to_world(GROUP_TOWED_VEHICLE)
	tss04_unmark_tow_target( LOCAL_PLAYER )

	if (IN_COOP) then
		tss04_unmark_tow_target( REMOTE_PLAYER )
		-- Remove the callback functions
		on_vehicle_destroyed("", REMOTE_TOWED_VEHICLE)
		-- Remove the on vehicle hitched callback
		on_vehicle_hitched("", REMOTE_TOWED_VEHICLE)
		-- Remove the on vehicle unhitched callback
		on_vehicle_unhitched("", REMOTE_TOWED_VEHICLE)
		-- Remove the towed vehicle marker
		marker_remove_vehicle(REMOTE_TOWED_VEHICLE, SYNC_REMOTE)

		-- We do not need these vehicles anymore
		release_to_world(GROUP_TOWED_VEHICLE_COOP)

		-- Carlos needs to be dependent
		follower_make_independent(Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX], false)
	end

	-- Disable all of the towed to trigger stuff
	on_trigger("", TOWED_VEHICLE_TRIGGER)
	trigger_enable(TOWED_VEHICLE_TRIGGER, false)
	marker_remove_trigger(TOWED_VEHICLE_TRIGGER, SYNC_ALL)

	-- Disable all of the towed to trigger stuff
	on_trigger("", TOWED_COMPLETE_TRIGGER)
	trigger_enable(TOWED_COMPLETE_TRIGGER, false)
	minimap_icon_remove_trigger(TOWED_COMPLETE_TRIGGER, SYNC_ALL)

	-- Remove the waypoints that were added
	waypoint_remove(SYNC_ALL)
   trigger_type_enable( "car mechanic", true )

   -- Clear notoriety here to fix a TDL
   notoriety_set( "brotherhood", 0 )
   notoriety_set( "ronin", 0 )
   notoriety_set( "samedi", 0 )
   notoriety_set( "police", 0 )

	tss04_phase_completed(human, Tss04_3_phase_table[TOWTRUCK_INDEX][CHARACTER_INDEX])
end

function tss04_killing_phase()
	-- Create the three groups of 2, 3, and 4 Ronin
	group_create(GROUP_RONIN, false)

	-- Pierce cannot be abandoned
	on_dismiss("tss04_phase_lieutenant_abandoned", Tss04_3_phase_table[KILLING_INDEX][CHARACTER_INDEX])

	if (IN_COOP) then
		-- Set max notoriety
		notoriety_set_max("Ronin", 3)

		-- In coop there is a timer
		hud_timer_set(0, 1000 * 60 * 2, "tss04_killing_timer_expired")
   end

   -- Add on_death function for the scripted characters...
   for i = 1, Num_tss04_ronin, 1 do
      on_death("tss04_ronin_killed", Tss04_ronin[i])
   end

   -- Mark the group of npcs
   marker_add_group(GROUP_RONIN, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)

   -- Soldiers follow the lieutenants
   for i = 2, Num_tss04_ronin_group1, 1 do
      npc_follow_npc(Tss04_ronin_group1[i], Tss04_ronin_group1[1])
   end

   for i = 2, Num_tss04_ronin_group2, 1 do
      npc_follow_npc(Tss04_ronin_group2[i], Tss04_ronin_group2[1])
   end

   for i = 2, Num_tss04_ronin_group3, 1 do
      npc_follow_npc(Tss04_ronin_group3[i], Tss04_ronin_group3[1])
   end

   -- Have the lieutenants wander
   wander_start(Tss04_ronin_group1[1], Tss04_ronin_group1[1], 10.0)
   wander_start(Tss04_ronin_group2[1], Tss04_ronin_group2[1], 10.0)
   wander_start(Tss04_ronin_group3[1], Tss04_ronin_group3[1], 30.0)

   -- Set max notoriety
   notoriety_set_max("Ronin", 2)
   -- Disable notoriety spawning
   notoriety_force_no_spawn("Ronin", true)

   objective_text(0, "tss04_killing", Tss04_ronin_kills, Num_tss04_ronin)

	-- Make sure the group is visible
	group_show(GROUP_RONIN)

	-- Show the shooting stunts tutorial
   if ( tutorial_completed( "shooting_stunts" ) == false ) then
   	tutorial_start("shooting_stunts", 5000, true)
   end
end

function tss04_ronin_killed(victim, attacker)
	on_death("", victim)
	marker_remove_npc(victim, SYNC_ALL)

	Tss04_ronin_kills = Tss04_ronin_kills + 1

	objective_text(0, "tss04_killing", Tss04_ronin_kills, Num_tss04_ronin)

	-- Have all of the scripted enemies died?
	if (Tss04_ronin_kills >= Num_tss04_ronin) then
		-- Release the ronin group to the world
		release_to_world(GROUP_RONIN)
		-- We need notoriety spawning to continue
		notoriety_force_no_spawn("Ronin", false)

		tss04_phase_completed(attacker, Tss04_3_phase_table[KILLING_INDEX][CHARACTER_INDEX])
	end
end

function tss04_killing_timer_expired()
	-- The stunt timer elapsed
	mission_end_failure("tss04", "tss04_killing_timer")
end