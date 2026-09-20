-- tss02.lua
-- SR2 mission script
-- 03/19/08

-- Global Variables --

-- This factor is applied to player damage in single player. 
SINGLE_PLAYER_DAMAGE_MULTIPLIER = 0.50

-- This factor is applied to player damage in co-op.
COOP_DAMAGE_MULTIPLIER = 0.75

Tss02_bh_brawl							= {"tss02_$c000", "tss02_$c001", "tss02_$c006", "tss02_$c007"}
Num_tss02_bh_brawl					= sizeof_table(Tss02_bh_brawl)

Tss02_bh_brawl_coop					= {"tss02_$c005", "tss02_$c005 (0)", "tss02_$c005 (1)"}
Num_tss02_bh_brawl_coop				= sizeof_table(Tss02_bh_brawl_coop)

Tss02_police_group_01				= {"tss02_$cop00", "tss02_$cop01", "tss02_$cop03", "tss02_$cop05", "tss02_$cop06", "tss02_$cop08"}
Num_tss02_police_group_01			= sizeof_table(Tss02_police_group_01)

Tss02_police_group_03				= {"tss02_$cop26", "tss02_$cop27", "tss02_$cop28", "tss02_$cop29", "tss02_$cop30"}
Num_tss02_police_group_03			= sizeof_table(Tss02_police_group_03)

Tss02_police_group_03_coop			= {"tss02_$coopcop11", "tss02_$coopcop12", "tss02_$coopcop13", "tss02_$coopcop14"}
Num_tss02_police_group_03_coop	= sizeof_table(Tss02_police_group_03_coop)

Tss02_police_need_pistols			= {"tss02_$cop17", "tss02_$cop15", "tss02_$cop16", "tss02_$cop14", "tss02_$cop31", "tss02_$cop32"}
Num_tss02_police_need_pistols		= sizeof_table(Tss02_police_need_pistols)

Alarm_audio_inst						= {} -- Courthouse alarm audio instances

Tss02_clothes_thread					= {[LOCAL_PLAYER] = INVALID_THREAD_HANDLE, [REMOTE_PLAYER] = INVALID_THREAD_HANDLE}
Tss02_clothes_nag_thread			= {[LOCAL_PLAYER] = INVALID_THREAD_HANDLE, [REMOTE_PLAYER] = INVALID_THREAD_HANDLE}
Tss02_clothes_purchased				= {[LOCAL_PLAYER] = false, [REMOTE_PLAYER] = false}

IN_COOP									= false

BROTHERHOOD_BRAWL						= false
Tss02_clothes_obtained				= false

LOCATION_START_LOCAL					= "tss02_$local_player_start"
LOCATION_START_REMOTE				= "tss02_$remote_player_start"
LOCATION_CHECKPOINT_LOCAL			= "tss02_$checkpoint_local"
LOCATION_CHECKPOINT_REMOTE			= "tss02_$checkpoint_remote"

GROUP_START								= "tss02_$start"
GROUP_START_VEHICLES					= "tss02_$start_vehicles"
GROUP_COURTROOM						= "tss02_$courtroom"
GROUP_POLICE_01						= "tss02_$police01"
GROUP_POLICE_02						= "tss02_$police02"
GROUP_POLICE_03						= "tss02_$police03"
GROUP_NEWSCOPTER						= "tss02_$MediaChopper"
GROUP_MEDIA								= "tss02_$Gmedia"
GROUP_AMBUSH							= "tss02_$ambush"

GROUP_START_COOP						= "tss02_$start_coop"
GROUP_POLICE_02_COOP					= "tss02_$police02_coop"
GROUP_POLICE_03_COOP					= "tss02_$police03_coop"

TRIGGER_COURTHOUSE_ENTER			= "tss02_$courthouse_enter"
TRIGGER_COURTROOM						= "tss02_$courtroom"
TRIGGER_COURTHOUSE_EXIT				= "tss02_$courthouse_exit"
TRIGGER_FORGIVE_FORGET				= "tss02_$forgive_forget"
TRIGGER_MISSION_COMPLETE			= "tss02_$finish"
TRIGGER_TURN01							= "tss02_$trigger_turn01"
TRIGGER_TURN02							= "tss02_$trigger_turn02"
TRIGGER_GPSFIX							= "tss02_$trigger_gpsfix"

TURNCOP01_01							= "tss02_$cop06"
TURNCOP01_02							= "tss02_$cop08"
TURNCOP02_01							= "tss02_$cop14"
TURNCOP02_02							= "tss02_$cop15"
TURNCOP02_03							= "tss02_$cop16"
TURNCOP03_01							= "tss02_$cop00"
TURNCOP03_02							= "tss02_$cop01"
TURNCOP03_03							= "tss02_$cop03"
TURNCOP03_04							= "tss02_$cop05"

TRIGGER_TIMER_START					= "tss02_$timer"
TRIGGER_GO_COMMANDO					= "tss02_$commando"
TRIGGER_CREATE_POLICE_03			= "tss02_$police03"
TRIGGER_SHOW_POLICE_03				= "tss02_$climax"

PLANTED_VEHICLE_1						= "tss02_$v000"
PLANTED_VEHICLE_2						= "tss02_$v001"

CHARACTER_GAT							= "tss02_$gat"
CHARACTER_LEGAL_LEE					= "tss02_$legal_lee"

CHARACTER_REPORTERS					= {	"tss02_$Creporter_1", "tss02_$Creporter_2", 
													"tss02_$Creporter_3", "tss02_$Creporter_4"
												}
CHARACTER_CAMERAMEN					= {	"tss02_$Ccameraman_1", "tss02_$Ccameraman_2",
													"tss02_$Ccameraman_3", "tss02_$Ccameraman_4"
												}

BROTHERHOOD_VEHICLE					= "tss02_$v002"

BROTHERHOOD_THREAD					= INVALID_THREAD_HANDLE

SECURITY_DOOR							= "tss02_SecurityDoor"

AMBUSH_NAVPOINT01						= "tss02_$n004"
AMBUSH_NAVPOINT02						= "tss02_$n005"
AMBUSH_COP01							= "tss02_$c009"
AMBUSH_COP02							= "tss02_$c008"
AMBUSH_THREAD_1						= INVALID_THREAD_HANDLE
AMBUSH_THREAD_2						= INVALID_THREAD_HANDLE

ALARM_NAVPOINTS						= {"tss02_$Nalarm_1"}

-- If the players restart the mission from the beginning, they're warped to these navpoints near
-- the mission trigger.
NAVPOINT_RESTART_LOCAL				= "tss02_$local_restart"
NAVPOINT_RESTART_REMOTE				= "tss02_$remote_restart"

NAVPOINT_OUTSIDE_SLOPPY_SECONDS	= "mission_start_sr2_city_$tss02_shop_entrance"
NAVPOINT_CASHIER_SLOPPY_SECONDS	= "shops_sr2_city_$RL_Slop_store"
NAVPOINT_OUTSIDE_TEENAY				= "tss02_$n006"
NAVPOINT_TEENAY_PARKING				= "tss02_$n007"

CHECKPOINT_TEENAY						= "CHECKPOINT_TEENAY"
CHECKPOINT_COURTROOM					= "CHECKPOINT_GOT_GAT"
-- MISSION_START_CHECKPOINT (This checkpoint is defined in ug_lib)

-- News Helicopter Stuffage
VEH_NEWS_HELI							= "tss02_$v005"
NEWS_HELI_PILOT						= "tss02_$c004"
NEWS_HELI_NAVPOINT					= "tss02_$media_flyaway"

TSS02_BROTHERHOOD_TAUNT_PERSONAS = {
	["HM_Bro1"]	=	"HMBRO1",
	["HM_Bro2"]	=	"HMBRO2",
	["HM_Bro3"]	=	"HMBRO3",

	["HF_Bro2"]	=	"HFBRO2",

	["WM_Bro3"]	=	"WMBRO3",

	["WF_Bro1"]	=	"WFBRO1",
	["WF_Bro2"]	=	"WFBRO2",
}

OTHER_PLAYER = {[LOCAL_PLAYER] = REMOTE_PLAYER, [REMOTE_PLAYER] = LOCAL_PLAYER}
PLAYER_SYNC = {[LOCAL_PLAYER] =SYNC_LOCAL, [REMOTE_PLAYER] = SYNC_REMOTE}

-- This needs to come after Gat being defined...
Tss02_gat_player_drive_conversation = {{"TSSP2_EXCHANGE_L1",			CHARACTER_GAT,	.25},
													{"PLAYER_TSSP2_EXCHANGE_L2",	LOCAL_PLAYER,	.25},
													{"TSSP2_EXCHANGE_L3",			CHARACTER_GAT,	.25},
													{"PLAYER_TSSP2_EXCHANGE_L4",	LOCAL_PLAYER,	.25},
													{"TSSP2_EXCHANGE_L5",			CHARACTER_GAT,	.25},
													{"PLAYER_TSSP2_EXCHANGE_L6",	LOCAL_PLAYER,	.1},
													{"TSSP2_EXCHANGE_L7",			CHARACTER_GAT,	.25},
													{"PLAYER_TSSP2_EXCHANGE_L8",	LOCAL_PLAYER,	.25},
													{"TSSP2_EXCHANGE_L9",			CHARACTER_GAT,	.25},
													{"PLAYER_TSSP2_EXCHANGE_L10",	LOCAL_PLAYER,	.25}}


function tss02_start(checkpoint, is_restart)

	-- Initialize the mission
	tss02_initialize(checkpoint, is_restart)

	-- If we're replaying the mission through the newspaper clippings and the player selects to restart
	-- from the beginning, then just restart from the Teenay checkpoint.
	if ((checkpoint == MISSION_START_CHECKPOINT) and mission_is_complete("tss02") and is_restart) then
		checkpoint = CHECKPOINT_TEENAY
	end

	-- If we're starting from the very begining, make the players change their clothes
	if (checkpoint == MISSION_START_CHECKPOINT) then

		-- When replaying the mission from the newspaper clippings, don't do the clothing
		-- purchase sequence
		local is_replay = mission_is_complete("tss02")
		notoriety_set_max("police", 2)
		notoriety_set_max("brotherhood", 2)
		notoriety_set_max("samedi", 0)
		notoriety_set_max("ronin", 0)
		character_slots_cap_for_team("ronin", 0)
		character_slots_cap_for_team("brotherhood", 2)
		character_slots_cap_for_team("samedi", 0)
		if (not is_replay) then
			tss02_get_new_clothes()
		end

		mission_start_fade_out()
		character_evacuate_from_all_vehicles(LOCAL_PLAYER)
		if (IN_COOP) then
			character_evacuate_from_all_vehicles(REMOTE_PLAYER)
		end

		-- Play the cutscene
		local group_list = {GROUP_START, GROUP_START_VEHICLES}
		if (IN_COOP) then
			group_list = {GROUP_START, GROUP_START_VEHICLES, GROUP_START_COOP}
		end
		cutscene_play("TSSP02-01", group_list, {LOCATION_START_LOCAL, LOCATION_START_REMOTE}, false)
		notoriety_set("police", 0)
		tss02_setup_teenay()
		mission_start_fade_in()
		mission_set_checkpoint(CHECKPOINT_TEENAY)
		checkpoint = CHECKPOINT_TEENAY
	end

	-- Let the brawl begin if we are starting from Teenay
	if (checkpoint == CHECKPOINT_TEENAY) then
		BROTHERHOOD_BRAWL = true
	end

	-- Starting from the courtroom	
	if (checkpoint == CHECKPOINT_COURTROOM) then

		local		human = LOCAL_PLAYER

		if (IN_COOP) then
			local		local_dist = get_dist_char_to_char(LOCAL_PLAYER, CHARACTER_GAT)
			local		remote_dist = get_dist_char_to_char(REMOTE_PLAYER, CHARACTER_GAT)

			if (remote_dist < local_dist) then
				human = REMOTE_PLAYER
			end
		end

		-- Get away.
		tss02_escape_the_courthouse(human)

		-- Bring the news helicopter back
		tss02_news_helicopter()

	end

end

function tss02_initialize(checkpoint, is_restart)

	-- We don't need to prep anything for the start of this mission so Scott
	-- asked me to get rid of the initial fade-in/fade-out.

	local do_fades = ( (checkpoint ~= MISSION_START_CHECKPOINT) or ( is_restart) )

	if (do_fades) then
		mission_start_fade_out(0.0)
	end

	tss02_initialize_common()

	tss02_initialize_checkpoint(checkpoint, is_restart)

	if (do_fades) then
		mission_start_fade_in()
	end

end

function	tss02_initialize_common()
	-- Start trigger is hit...the activate button was hit
	set_mission_author("Ryan Spencer and Phillip Alexander")

	-- Reduce the amount of damage applied to the players
	if (IN_COOP) then
		character_set_damage_multiplier(REMOTE_PLAYER, COOP_DAMAGE_MULTIPLIER)
		character_set_damage_multiplier(LOCAL_PLAYER, COOP_DAMAGE_MULTIPLIER)
	else
		character_set_damage_multiplier(LOCAL_PLAYER, SINGLE_PLAYER_DAMAGE_MULTIPLIER)
	end

	-- Check for coop being active
	if (coop_is_active()) then
		IN_COOP	= true
	end
end

function tss02_initialize_checkpoint(checkpoint, is_restart)

	if (checkpoint == MISSION_START_CHECKPOINT) then

		-- If we're replaying the mission through the newspaper clippings and the player selects to restart
		-- from the beginning, then just restart from the Teenay checkpoint.
		if (mission_is_complete("tss02") and is_restart) then
			teleport_coop(LOCATION_START_LOCAL, LOCATION_START_REMOTE, true)
			checkpoint = CHECKPOINT_TEENAY
		elseif (is_restart) then
			teleport_coop(NAVPOINT_RESTART_LOCAL, NAVPOINT_RESTART_REMOTE, true)
		end

	end
	
	if (checkpoint == CHECKPOINT_TEENAY) then

		-- Create groups hidden to match cutscene_play behavior
		group_create_hidden(GROUP_START, true)
		if (IN_COOP) then
			group_create_hidden(GROUP_START_COOP, true)
		end
		group_create_hidden(GROUP_START_VEHICLES)

		tss02_setup_teenay()
	end

	if (checkpoint == CHECKPOINT_COURTROOM) then

		-- Create the courtroom group
		group_create(GROUP_COURTROOM, true)
		-- Create the police group
		group_create(GROUP_POLICE_02, true)
		group_create(GROUP_AMBUSH, true)

		if (IN_COOP) then
			group_create(GROUP_POLICE_02_COOP, true)
		end
		
		if not (IN_COOP) then
			character_hide(CHARACTER_LEGAL_LEE)
		end


	end

end

function tss02_setup_teenay()

	-- Disable Police ambient spawns. This keeps extra security guards from spawning in the Courthouse
	-- beyond those which we've scripted to do so.
	spawning_allow_cop_ambient_spawns(false)

	-- Show the starting groups
	group_show(GROUP_START)
	group_show(GROUP_START_VEHICLES)
	if (IN_COOP) then
		group_show(GROUP_START_COOP)
	end

	-- Hide the brotherhood chase members for now
	character_hide("tss02_$c002")
	character_hide("tss02_$c003")
	vehicle_hide(BROTHERHOOD_VEHICLE)

	character_set_combat_ready("tss02_$c000", true, 5000)
	character_set_combat_ready("tss02_$c001", true, 5000)

	-- Enable timer trigger
	trigger_enable(TRIGGER_TIMER_START, true)
	on_trigger("tss02_timer_start", TRIGGER_TIMER_START)

	-- Enable commando trigger
	trigger_enable(TRIGGER_GO_COMMANDO, true)
	on_trigger("tss02_go_commando", TRIGGER_GO_COMMANDO)

	-- Enable courthouse trigger
	trigger_enable(TRIGGER_COURTHOUSE_ENTER, true)
	on_trigger("tss02_go_to_the_courtroom", TRIGGER_COURTHOUSE_ENTER)

	-- Enable the GPSFix Trigger
	trigger_enable(TRIGGER_GPSFIX, true)
	on_trigger("tss02_fix_the_gps", TRIGGER_GPSFIX)

	-- Remove the inventory from the bar brawlers...we want a fist fight for now
	for i = 1, Num_tss02_bh_brawl, 1 do
		inv_item_remove_all(Tss02_bh_brawl[i])
	end

	if (IN_COOP) then
		for i = 1, Num_tss02_bh_brawl_coop, 1 do
			inv_item_remove_all(Tss02_bh_brawl_coop[i])
		end
	end

	-- Override the Brotherhood personas
	persona_override_group_start(TSS02_BROTHERHOOD_TAUNT_PERSONAS, POT_SITUATIONS[POT_ATTACK], "TSSPROLOGUE02_TAUNT")
	
	-- Give the brotherhood in the doorway knives...
	inv_item_add("baseball_bat", 1, "tss02_$c006")
	inv_item_add("tire_iron", 1, "tss02_$c007")

	-- Prompt player(s) to get to the Courthouse
	mission_help_table("tss02_courthouse")

	-- Up the notoriety a couple seconds after the mission starts
	thread_new("tss02_bump_notoriety")

	-- Spawn off a thread to create our police group when we are close to the courthouse
	thread_new("tss02_load_police_group")

	-- Spawn the media group when we're close to the courthouse
	thread_new("tss02_load_media_group")

	-- Display the gang notoriety tutorial as soon as Brotherhood notoriety ~= 0
	thread_new("tss02_notoriety_tutorial")

	-- Spawn a group of brotherhood to chase when the players get so far away.
	thread_new("tss02_go_to_the_courthouse")

end


Tss02_message_delay_s = 0
function tss02_get_new_clothes()

	-- Spawn threads directing players to change clothes
	Tss02_clothes_thread[LOCAL_PLAYER] = thread_new("tss02_change_clothes", LOCAL_PLAYER)
	if (coop_is_active()) then
		Tss02_clothes_thread[REMOTE_PLAYER] = thread_new("tss02_change_clothes", REMOTE_PLAYER)
	end

	while (not Tss02_clothes_obtained) do
		thread_yield()
	end

end

function tss02_change_clothes(player)

	local sync = PLAYER_SYNC[player]

	-- Tell the player to head to Sloppy Seconds
	Tss02_clothes_nag_thread[player] = thread_new("tss02_help_nag", "MSN_TSS02_INTRO_A", sync, 40)

	-- Add indicators leading to the shop's parking lot
	marker_add_navpoint(NAVPOINT_OUTSIDE_SLOPPY_SECONDS, MINIMAP_ICON_LOCATION, INGAME_EFFECT_LOCATION, sync)
	waypoint_add(NAVPOINT_OUTSIDE_SLOPPY_SECONDS, sync)

	-- Add an on_item_purchased callback
	on_purchase("tss02_item_purchased")

	-- Wait for the player to purchase clothing or approach the indicated store, then remove indicators
	while ( (get_dist(player, NAVPOINT_OUTSIDE_SLOPPY_SECONDS) > 10.0) and (not Tss02_clothes_purchased[player]) ) do
		thread_yield()
	end
	marker_remove_navpoint(NAVPOINT_OUTSIDE_SLOPPY_SECONDS, sync)
	waypoint_remove(sync)

	-- Redisplay the help message if it hasn't played recently.
	if (not Tss02_clothes_purchased[player]) then

		if (Tss02_message_delay_s > 8) then 
			Tss02_message_delay_s = 0
		end

	end

	-- wait for player to approach the indicated cashier or purchase clothing, then remove indicator
	minimap_icon_add_navpoint(NAVPOINT_CASHIER_SLOPPY_SECONDS, MINIMAP_ICON_LOCATION, "", sync)
	while( (get_dist(player, NAVPOINT_CASHIER_SLOPPY_SECONDS) > 1.5) and (not Tss02_clothes_purchased[player]) ) do
		thread_yield()
	end
	minimap_icon_remove_navpoint(NAVPOINT_CASHIER_SLOPPY_SECONDS, sync)
	thread_kill(Tss02_clothes_nag_thread[player])

	-- Wait for the player to leave the store
	while( get_dist(player, NAVPOINT_CASHIER_SLOPPY_SECONDS) < 4.0)  do
		thread_yield()
	end

	-- Make sure that oustanding threads and indicators are removed for this player
	thread_kill(Tss02_clothes_nag_thread[player])	

	-- Send to teenay
	tss02_send_to_teenay(player)

end

function tss02_send_to_teenay(player)

	local sync = PLAYER_SYNC[player]

	-- Make sure that there is a delay after old messages are cleared
	delay(1.0)

	-- Send the player to the start of tss02
	Tss02_clothes_nag_thread[player] = thread_new("tss02_help_nag", "MSN_TSS02_INTRO_B", sync, 40)

	-- Send the player to the Teenay parking lot
	waypoint_add(NAVPOINT_TEENAY_PARKING, sync)
	marker_add_navpoint(NAVPOINT_TEENAY_PARKING, MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_LOCATION, sync)
	while( get_dist(player, NAVPOINT_TEENAY_PARKING) > 15 ) do
		thread_yield()
	end
	marker_remove_navpoint(NAVPOINT_TEENAY_PARKING, sync)
	waypoint_remove(sync)

	-- Send the player to the Teenay front door
	marker_add_navpoint(NAVPOINT_OUTSIDE_TEENAY, MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_LOCATION, sync)
	while( get_dist(player, NAVPOINT_OUTSIDE_TEENAY) > 5 ) do
		thread_yield()
	end
	marker_remove_navpoint(NAVPOINT_OUTSIDE_TEENAY, sync)	

	-- Kill outstanding threads and cleanup indicators
	thread_kill(Tss02_clothes_nag_thread[player])	
	thread_kill(Tss02_clothes_thread[OTHER_PLAYER[player]])
	thread_kill(Tss02_clothes_nag_thread[OTHER_PLAYER[player]])

	-- Remove the item purchased callback
	tss02_cleanup_clothes_sequence()
	delay(1.0)
	Tss02_clothes_obtained = true

end

function tss02_item_purchased(store_type, item_purchased, player)

	if(store_type == "clothing") then

		Tss02_clothes_purchased[player] = true

	end

end

function tss02_help_nag(tag, sync, delay_s)
	Tss02_message_delay_s = 0
	while(true) do
		Tss02_message_delay_s = Tss02_message_delay_s - get_frame_time()
		if (Tss02_message_delay_s <= 0.0) then
			mission_help_table(tag, "", "", sync)
			Tss02_message_delay_s = delay_s
		end
		thread_yield()
	end
end

function tss02_cleanup_clothes_sequence()

	on_purchase("")

	marker_remove_navpoint(NAVPOINT_OUTSIDE_SLOPPY_SECONDS, SYNC_LOCAL)
	marker_remove_navpoint(NAVPOINT_CASHIER_SLOPPY_SECONDS, SYNC_LOCAL)
	marker_remove_navpoint(NAVPOINT_TEENAY_PARKING, SYNC_LOCAL)	
	marker_remove_navpoint(NAVPOINT_OUTSIDE_TEENAY, SYNC_LOCAL)	
	waypoint_remove(SYNC_LOCAL)
	mission_help_clear(SYNC_LOCAL)

	if( coop_is_active() ) then
		marker_remove_navpoint(NAVPOINT_OUTSIDE_SLOPPY_SECONDS, SYNC_REMOTE)
		marker_remove_navpoint(NAVPOINT_CASHIER_SLOPPY_SECONDS, SYNC_REMOTE)
		marker_remove_navpoint(NAVPOINT_TEENAY_PARKING, SYNC_LOCAL)	
		marker_remove_navpoint(NAVPOINT_OUTSIDE_TEENAY, SYNC_REMOTE)	
		waypoint_remove(SYNC_REMOTE)
		mission_help_clear(SYNC_REMOTE)	
	end
end

function tss02_cleanup()
	-- Cleanup mission here

	IN_COOP = coop_is_active()

	-- Reset the amount of damage applied to the players
	character_set_damage_multiplier(LOCAL_PLAYER, 1.0)
	if (IN_COOP) then
		character_set_damage_multiplier(REMOTE_PLAYER, 1.0)
	end

	-- Reenable the action nodes
	action_nodes_restrict_spawning(false)

	tss02_cleanup_clothes_sequence()

	-- Kill the courthouse alarms
	for i,alarm_audio_inst in pairs(Alarm_audio_inst) do
		audio_stop_emitting_id(alarm_audio_inst)
	end

	-- Make sure that the hud timer is stopped
	hud_timer_stop(0)

	-- Reenable Police ambient spawns
	spawning_allow_cop_ambient_spawns(true)

	-- Just reset all of the persona overrides...
	if (character_exists(CHARACTER_GAT)) then
		persona_override_character_stop_all(CHARACTER_GAT)
		on_death("", CHARACTER_GAT)
		on_dismiss("", CHARACTER_GAT)
		party_dismiss(CHARACTER_GAT)
	end

	if (character_exists(CHARACTER_LEGAL_LEE)) then
		persona_override_character_stop_all(CHARACTER_LEGAL_LEE)
		on_death("", CHARACTER_LEGAL_LEE)
		on_dismiss("", CHARACTER_LEGAL_LEE)
		party_dismiss(CHARACTER_LEGAL_LEE)
	end

	persona_override_group_stop(BROTHERHOOD_PERSONAS, POT_SITUATIONS[POT_ATTACK])

	-- Reset notoriety levels
	notoriety_reset("brotherhood")
	notoriety_reset("police")
	notoriety_reset("samedi")
	notoriety_reset("ronin")

	-- Remove the enter callbacks
	on_vehicle_enter("", LOCAL_PLAYER)
	if (IN_COOP) then
		on_vehicle_enter("", REMOTE_PLAYER)
	end
	on_vehicle_enter("", PLANTED_VEHICLE_1)
	on_vehicle_enter("", PLANTED_VEHICLE_2)

	-- Reset the door
	mesh_mover_reset(SECURITY_DOOR)
	mesh_mover_reset("tss02_door_1")
	mesh_mover_reset("tss02_door_2")

end

function tss02_success()
	-- Called when the mission has ended with success

	-- Post the news events
	radio_post_event("JANE_NEWS_TSSP02_01", true)
	radio_post_event("JANE_NEWS_TSSP02_02", true)

	-- Unlock all activities and diversions except heli. Both heli instances
	-- have cutscenes w/ Shaundi and Pierce who are introduced in TSS04 so
	-- heli is unlocked after sccessfully completeing that mission.

--	mission_unlock("angel_ct")
--	mission_unlock("angel_he")
--	mission_unlock("crowd_ht")
--	mission_unlock("crowd_su")
--	mission_unlock("demoderby_un")
--	mission_unlock("drug_ai")
--	mission_unlock("drug_ht")
--	mission_unlock("escort_rl")
--	mission_unlock("escort_un")
--	mission_unlock("fight_ar")
--	mission_unlock("fight_pr")
--	mission_unlock("fraud_fc")
--	mission_unlock("fraud_mu")
--	mission_unlock("fuzz_pj")
--	mission_unlock("fuzz_sx")
--	mission_unlock("mayhem_nu")
--	mission_unlock("mayhem_st")
--	mission_unlock("piracy_dk")
--	mission_unlock("piracy_sr")
--	mission_unlock("sewage_rl")
--	mission_unlock("sewage_sx")
--	mission_unlock("snatch_ct")
--	mission_unlock("snatch_dt")
--	mission_unlock("torch_ap")
--	mission_unlock("torch_dt")
	mission_unlock("zombie")

	-- Unlock the crib purchasing
	crib_purchasing_unlock()

	-- Unlock Red Light Apartment Crib

	-- Display the tutorials
	tutorial_start("respect_meter", 0, true)
	tutorial_start("waypoint", 2000, true)
end

function tss02_complete()
	-- End the mission with success
	mission_end_success("tss02")
end

function tss02_time_ran_out()
	-- End the mission since Gat was killed
	mission_end_failure("tss02", "tss02_time_ran_out")
end

function tss02_gat_died()
	-- End the mission since Gat was killed
	mission_end_failure("tss02", "tss02_gat_died")
end

function tss02_gat_abandoned()
	-- End the mission since Gat was abandoned
	mission_end_failure("tss02", "tss02_gat_abandoned")
end

function tss02_lee_died()
	-- End the mission since Legal Lee was killed
	mission_end_failure("tss02", "tss02_lee_died")
end

function tss02_lee_abandoned()
	-- End the mission since Legal Lee was abandoned
	mission_end_failure("tss02", "tss02_lee_abandoned")
end

function tss02_bump_notoriety()
	-- Wait until we can brawl
	while (not BROTHERHOOD_BRAWL) do
		thread_yield()
	end
	
	-- Delay for a second or two
	delay(1.5)

	-- Set the notoriety level for the Brotherhood
	notoriety_set_min("brotherhood", 1)

	-- Have the first Brotherhood character attack
	attack("tss02_$c000")

end

Tss02_notoriety_tutorial_displayed = false
function tss02_notoriety_tutorial()

	while( notoriety_get("Brotherhood") == 0) do
		thread_yield()
	end
		
	delay(1.0)
	tutorial_start("notoriety_gang")

end

function tss02_timer_start()
	-- Disable the trigger
	trigger_enable(TRIGGER_TIMER_START, false)

	-- Tell the player(s) to get to the Courthouse before the trial ends...
	mission_help_table("tss02_trial")

	-- Add a timer
	hud_timer_set(0, 4 * 60 * 1000, "tss02_time_ran_out")

	-- This function returns true if the players should receive the courthouse prompts
	local function ready_for_courthouse_prompts()
		local in_vehicle =	(	character_is_in_vehicle(LOCAL_PLAYER) 
										or (IN_COOP and (character_is_in_vehicle(REMOTE_PLAYER)))
									)
		local far_enough =	(	(get_dist(LOCAL_PLAYER, TRIGGER_TIMER_START) > 100)
										or (IN_COOP and (get_dist(REMOTE_PLAYER, TRIGGER_TIMER_START) > 100))
									)
		return (in_vehicle or far_enough)
	end

	-- Wait for the previous helptext to go away, then prompt to grab the car
	delay(7.0)
	if (not ready_for_courthouse_prompts()) then
		mission_help_table_nag("ss08_get_a_car")
		marker_add_vehicle(PLANTED_VEHICLE_2, MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_INTERACT, SYNC_ALL)
		while(not ready_for_courthouse_prompts() ) do
			thread_yield()
		end
		marker_remove_vehicle(PLANTED_VEHICLE_2)
	end

	-- If the player is in a car, display the cruise contol tutorial
	if ( character_is_in_vehicle(LOCAL_PLAYER) or (IN_COOP and (character_is_in_vehicle(REMOTE_PLAYER))) ) then
		tutorial_start("cruise_control_completed", 0, true)
	end

	-- Mark the Courthouse, remind the player to go there
	mission_help_table_nag("tss02_courthouse")
	waypoint_add(TRIGGER_GPSFIX, SYNC_ALL)
	marker_add_trigger(TRIGGER_GPSFIX, MINIMAP_ICON_LOCATION, INGAME_EFFECT_LOCATION, SYNC_ALL)
end

function tss02_fix_the_gps()
	marker_remove_trigger(TRIGGER_GPSFIX, SYNC_ALL)
	waypoint_remove(SYNC_ALL)
	waypoint_add(TRIGGER_COURTHOUSE_ENTER, SYNC_ALL)
	marker_add_trigger(TRIGGER_COURTHOUSE_ENTER, MINIMAP_ICON_LOCATION, INGAME_EFFECT_LOCATION, SYNC_ALL)
end

function tss02_news_helicopter()
	group_create(GROUP_NEWSCOPTER, true)
	delay(1)
	vehicle_enter_teleport(NEWS_HELI_PILOT, VEH_NEWS_HELI, 0)
	vehicle_set_dont_scare_peds(VEH_NEWS_HELI, true)
	delay(1)
	vehicle_chase(VEH_NEWS_HELI, LOCAL_PLAYER)
end

function tss02_go_commando()
	-- Disable the trigger
	trigger_enable(TRIGGER_GO_COMMANDO, false)

	-- Turn the human toward the player
	turn_to("tss02_$c006", "tss02_door_1")

	-- Play the kick animation
	action_play("tss02_$c006", "kick door", nil, true, 0.45, true)
	door_open("tss02_door_1")

	-- Attack the player
	npc_leash_remove("tss02_$c006")
	attack("tss02_$c006")

	-- Turn the human toward the player
	turn_to("tss02_$c007", "tss02_door_2")

	-- Play the kick animation
	action_play("tss02_$c007", "kick door", nil, true, 0.45, true)
	door_open("tss02_door_2", true)

	-- Attack the player
	npc_leash_remove("tss02_$c007")
	attack("tss02_$c007")
end

function tss02_load_police_group()
	local		dist = 50.0

	-- Wait until either the local or remote player are close enough
	while (1) do
		thread_yield()

		-- Is the local player close enough?
		if (get_dist_char_to_nav(LOCAL_PLAYER, TRIGGER_COURTHOUSE_ENTER) < dist) then
			break
		end

		if (IN_COOP) then
			-- Is the remote player close enough?
			if (get_dist_char_to_nav(REMOTE_PLAYER, TRIGGER_COURTHOUSE_ENTER) < dist) then
				break
			end
		end

	end

	-- Show the group
	group_create(GROUP_POLICE_01, true)
	group_show(GROUP_POLICE_01)

	-- Have the police wander
	-- for i = 1, Num_tss02_police_group_01, 1 do
	-- 	wander_start(Tss02_police_group_01[i], Tss02_police_group_01[i], 10.0)
	-- end

	-- since we are here anyhow, spawn the news chopper.
	tss02_news_helicopter()

end

function tss02_load_media_group()

	local		dist = 200.0

	-- Wait until either the local or remote player are close enough
	while (get_dist_closest_player_to_object(TRIGGER_COURTHOUSE_ENTER) > dist) do
		thread_yield()
	end

	-- Create the group
	group_create(GROUP_MEDIA, true)

	-- Wait until a player gets closer before using the action node
	for i=1, sizeof_table(CHARACTER_REPORTERS), 1 do
		thread_new("tss02_media_use_action_nodes", CHARACTER_REPORTERS[i], CHARACTER_CAMERAMEN[i])
	end

end

function tss02_media_use_action_nodes(reporter, cameraman)

	while( (get_dist_closest_player_to_object(reporter)>75) and (get_dist_closest_player_to_object(cameraman)>75) ) do
		thread_yield()
	end

	if(	character_exists(reporter) and (not character_is_dead(reporter)) and
			character_exists(cameraman) and (not character_is_dead(cameraman))
		) then
		npc_use_closest_action_node_of_type(reporter, "Reporter")
		npc_use_closest_action_node_of_type(cameraman, "CameraMan")
	end

end

function tss02_brotherhood_chase()
	local		dist = 60.0
	local		local_player = false
	local		remote_player = true

	-- Put the Characters in the vehicle
	vehicle_enter_teleport("tss02_$c002", BROTHERHOOD_VEHICLE, 0)
	vehicle_enter_teleport("tss02_$c003", BROTHERHOOD_VEHICLE, 1)

	-- Wait until both the local and remote player are far enough away
	while (not local_player or not remote_player) do
		thread_yield()

		-- We need to make sure both of the players are far enough away
		local_player = (get_dist_char_to_nav(LOCAL_PLAYER, BROTHERHOOD_VEHICLE) > dist)
		if (IN_COOP) then
			remote_player = (get_dist_char_to_nav(REMOTE_PLAYER, BROTHERHOOD_VEHICLE) > dist)
		end
	end

	-- Show the chase members
	character_show("tss02_$c002")
	character_show("tss02_$c003")
	vehicle_show(BROTHERHOOD_VEHICLE)

	-- Perform the chase
	vehicle_chase(BROTHERHOOD_VEHICLE, LOCAL_PLAYER, false, true, true)
end

function tss02_go_to_the_courthouse()

	-- Spawn off another thread to have the Brotherhood chase the player(s)
	BROTHERHOOD_THREAD = thread_new("tss02_brotherhood_chase")
end

function tss02_call_off_brotherhood()
	-- Have them go to the forgive and forget trigger...since it isn't so far away
	vehicle_pathfind_to(BROTHERHOOD_VEHICLE, TRIGGER_FORGIVE_FORGET, true)

	-- Release this group to the world
	release_to_world("tss02_$c002")
	release_to_world("tss02_$c003")
	release_to_world(BROTHERHOOD_VEHICLE)
end

function tss02_go_to_the_courtroom()

	-- Disable this trigger
	trigger_enable(TRIGGER_COURTHOUSE_ENTER, false)
	trigger_enable(TRIGGER_GPSFIX, false)
	-- Remove the marker
	marker_remove_trigger(TRIGGER_COURTHOUSE_ENTER, SYNC_ALL)
	marker_remove_trigger(TRIGGER_GPSFIX, SYNC_ALL)
	-- Remove the waypoint
	waypoint_remove(SYNC_ALL)

	-- Kill the hud timer
	hud_timer_stop(0)

	-- Kill the Brotherhood thread if it still is active
	thread_kill(BROTHERHOOD_THREAD)
	-- Reset the Brotherhood notoriety so they flee the situation
	notoriety_reset("brotherhood")
	notoriety_set("brotherhood", 0)
	-- Force the scripted Brotherhood to retreat
	if (not vehicle_is_destroyed(BROTHERHOOD_VEHICLE)) then
		thread_new("tss02_call_off_brotherhood")
	end

	-- Mark the Courtroom
	marker_add_trigger(TRIGGER_COURTROOM, MINIMAP_ICON_LOCATION, INGAME_EFFECT_LOCATION, SYNC_ALL)

	-- Enable the Courtroom trigger
	trigger_enable(TRIGGER_COURTROOM, true)
	on_trigger("tss02_checkpoint", TRIGGER_COURTROOM)

	-- Make sure the door is open
	door_open(SECURITY_DOOR)

	-- Tell the player(s) what to do
	mission_help_table("tss02_find_gat")
end

function tss02_checkpoint(human)

   -- Disable this trigger
	trigger_enable(TRIGGER_COURTROOM, false)
	marker_remove_trigger(TRIGGER_COURTROOM, SYNC_ALL)

	-- Release the media group. Otherwise we'll have too many critical ambients.
	release_to_world(GROUP_MEDIA)

	-- Play the mid-mission Story cutscene...block load the 2nd police group
	mission_start_fade_out()

	-- Remove players from all vehicles
	character_evacuate_from_all_vehicles(LOCAL_PLAYER)
	if (IN_COOP) then
		character_evacuate_from_all_vehicles(REMOTE_PLAYER)
	end

	-- Determine which groups the cutscene will need to load
	local groups = {GROUP_COURTROOM, GROUP_POLICE_02, GROUP_AMBUSH}
	if (IN_COOP) then
		groups = {GROUP_COURTROOM, GROUP_POLICE_02, GROUP_AMBUSH, GROUP_POLICE_02_COOP}
	end

	-- Determine the locations to which the players should be teleported by the cutscene
	local teleport_locations = {LOCATION_CHECKPOINT_LOCAL, LOCATION_CHECKPOINT_REMOTE}
	if (human == REMOTE_PLAYER) then
		teleport_locations = {LOCATION_CHECKPOINT_REMOTE, LOCATION_CHECKPOINT_LOCAL}
	end

	-- Playe the cutscene and show the groups
	cutscene_play("tssp02-02", groups, teleport_locations, false)
	for i, group in pairs(groups) do
		group_show(group)
	end

	-- Hide Legal Lee if this is a single player game.
	if (not IN_COOP) then
		character_hide(CHARACTER_LEGAL_LEE)
	end

	-- Force police to always be combat ready
	set_force_combat_ready_team("police")

	-- Fade back in and set a checkpoint.
	mission_start_fade_in()
	mission_set_checkpoint(CHECKPOINT_COURTROOM)

	-- Setup the escape
	tss02_escape_the_courthouse(human)
end

function tss02_escape_the_courthouse(human)

	-- Start the courthouse alarms
	thread_new("tss02_courthouse_alarms")

	-- Lets do some persona override here...
	persona_override_character_stop_all(CHARACTER_GAT)
	persona_override_character_start(CHARACTER_GAT, POT_SITUATIONS[POT_ATTACK],		"GAT_TSSP02_ATTACK")
	persona_override_character_start(CHARACTER_GAT, POT_SITUATIONS[POT_TAKE_DAMAGE], "GAT_TSSP02_TAKEDAM")
	-- most of Gat's "chatter" lines refer to Lee who isn't present in single player
	if (IN_COOP) then
		persona_override_character_start(CHARACTER_GAT, POT_SITUATIONS[POT_GRATS_SELF],	"GAT_TSSP2_CHATTER")
	end

	persona_override_character_stop_all(CHARACTER_LEGAL_LEE)
	persona_override_character_start(CHARACTER_LEGAL_LEE, POT_SITUATIONS[POT_TAKE_DAMAGE],		"LEE_TSSP2_TAKEDAM")
	persona_override_character_start(CHARACTER_LEGAL_LEE, POT_SITUATIONS[POT_BARTER],			"LEE_TSSP2_BARTER")
	persona_override_character_start(CHARACTER_LEGAL_LEE, POT_SITUATIONS[POT_TAUNTED_BY_PC],	"LEE_TSSP2_TAUNT")
	persona_override_character_start(CHARACTER_LEGAL_LEE, POT_SITUATIONS[POT_PRAISED_BY_PC],	"LEE_TSSP2_PRAISED")
	persona_override_character_start(CHARACTER_LEGAL_LEE, POT_SITUATIONS[POT_GRATS_PC],			"LEE_TSSP2_CHATTER")

	-- Mark the Courthouse exit
	marker_add_trigger(TRIGGER_COURTHOUSE_EXIT, MINIMAP_ICON_LOCATION, INGAME_EFFECT_LOCATION, SYNC_ALL)

	-- Enable turn to face triggers
	trigger_enable(TRIGGER_TURN01, true)
	on_trigger("tss02_trigger_turntoface_01", TRIGGER_TURN01)
	trigger_enable(TRIGGER_TURN02, true)
	on_trigger("tss02_trigger_turntoface_02", TRIGGER_TURN02)

	-- Enable the Courthouse exit
	trigger_enable(TRIGGER_COURTHOUSE_EXIT, true)
	on_trigger("tss02_visit_forgive_and_forget", TRIGGER_COURTHOUSE_EXIT)

	-- Figure out who is added to what party
	if (not IN_COOP) then
		-- In single player, Gat is added
		party_add(CHARACTER_GAT, LOCAL_PLAYER)

		-- Setup the callbacks
		on_death("tss02_gat_died", CHARACTER_GAT)
		on_dismiss("tss02_gat_abandoned", CHARACTER_GAT)

	else
		-- Local player gets Gat and the remote gets Legal Lee
		party_add(CHARACTER_GAT, LOCAL_PLAYER)
		party_add(CHARACTER_LEGAL_LEE, REMOTE_PLAYER)

		-- Legal Lee can't do anything...so we need to remove his inventory
		inv_item_remove_all(CHARACTER_LEGAL_LEE)

		-- Setup the callbacks
		on_death("tss02_gat_died", CHARACTER_GAT)
		on_dismiss("tss02_gat_abandoned", CHARACTER_GAT)

		on_death("tss02_lee_died", CHARACTER_LEGAL_LEE)
		on_dismiss("tss02_lee_abandoned", CHARACTER_LEGAL_LEE)
	end

	-- Adjust the difficulty based upon which route the player took in tss01
	if (Tss01_tutorial_route_chosen) then
		-- Set the minimum police notoriety
		notoriety_set_min("police", 2)
		-- Took the tutorial route
		notoriety_set_max("police", 2)
	else
		-- Set the minimum police notoriety
		notoriety_set_min("police", 2)
		-- Took the run and gun route
		notoriety_set_max("police", 2)
	end

	-- Give the player a gun if he doesn't have one...more ammo if he does
	local weapon = inv_item_in_slot(human, "pistol")

	if (weapon == "none") then
		inv_item_add("glock", 2, human)
	else
		inv_item_add_ammo(human, weapon, 50)
	end

	-- We want to ensure these police have pistols...
	for i = 1, Num_tss02_police_need_pistols, 1 do
		inv_item_remove_all(Tss02_police_need_pistols[i])
		inv_item_add("glock", 1, Tss02_police_need_pistols[i])
	end

	-- Tell the player(s) to get Gat out
	if (IN_COOP) then
		mission_help_table("tss02_get_out")
	else
		mission_help_table("tss02_get_out_solo")
	end

	-- Close and lock the security door
	door_close(SECURITY_DOOR)
	door_lock(SECURITY_DOOR, true)

	-- Setup the trigger for creating the third police group
	trigger_enable(TRIGGER_CREATE_POLICE_03, true)
	on_trigger("tss02_create_police_group", TRIGGER_CREATE_POLICE_03)

	-- Disable the action nodes
	action_nodes_restrict_spawning(true)

	-- Start the cortroom ambush cops
	AMBUSH_THREAD_1 = thread_new("tss02_ambush_path", AMBUSH_COP01, AMBUSH_NAVPOINT01)
	delay(1)
	AMBUSH_THREAD_2 = thread_new("tss02_ambush_path", AMBUSH_COP02, AMBUSH_NAVPOINT02)
end

function tss02_courthouse_alarms()

	local	audio_id = audio_get_id("SFX_TSSP02_ALARM", "foley")

	-- Fire off the interior alarms
	for i, navpoint in pairs(ALARM_NAVPOINTS) do
		Alarm_audio_inst[i] = audio_play_emitting_id_for_navpoint(navpoint, audio_id)
	end

end

function tss02_ambush_path(cop, ambush_navp)
	set_ignore_ai_flag(cop, true)
	move_to(cop, ambush_navp, 2, true, true)
	if ( character_exists(cop) and (not character_is_dead(cop)) )then
		set_ignore_ai_flag(cop, false)
		local dist, player = get_dist_closest_player_to_object(cop)
		turn_to_char(cop, player)
	end
end

function tss02_trigger_turntoface_01()

	if( character_exists(TURNCOP01_01) and (not character_is_dead(TURNCOP01_01)) ) then
		turn_to_char(TURNCOP01_01, LOCAL_PLAYER)
	end
	if(character_exists(TURNCOP01_02) and (not character_is_dead(TURNCOP01_02)) ) then		
		-- Ambush cop2 and turncop01_02 are the same guy, kill the other thread manipulating him
		thread_kill(AMBUSH_THREAD_2)
		set_ignore_ai_flag(TURNCOP01_02, false)
		turn_to_char(TURNCOP01_02, LOCAL_PLAYER)
	end
		if( character_exists(TURNCOP03_01) and (not character_is_dead(TURNCOP03_01)) ) then
		turn_to_char(TURNCOP03_01, LOCAL_PLAYER)
	end
		if( character_exists(TURNCOP03_02) and (not character_is_dead(TURNCOP03_02)) ) then
		turn_to_char(TURNCOP03_02, LOCAL_PLAYER)
	end
		if( character_exists(TURNCOP03_03) and (not character_is_dead(TURNCOP03_03)) ) then
		turn_to_char(TURNCOP03_03, LOCAL_PLAYER)
	end
		if( character_exists(TURNCOP03_04) and (not character_is_dead(TURNCOP03_04)) ) then
		turn_to_char(TURNCOP03_04, LOCAL_PLAYER)
	end

end

function tss02_trigger_turntoface_02()
	if not character_is_dead(TURNCOP02_01) then
		turn_to_char(TURNCOP02_01, LOCAL_PLAYER)
	end
	if not character_is_dead(TURNCOP02_02) then
		turn_to_char(TURNCOP02_02, LOCAL_PLAYER)
	end
	if not character_is_dead(TURNCOP02_03) then
		turn_to_char(TURNCOP02_03, LOCAL_PLAYER)
	end
end

function tss02_create_police_group()
	-- Disable the trigger since it is no longer needed
	trigger_enable(TRIGGER_CREATE_POLICE_03, false)

	-- Create the police group when this trigger is hit
	group_create_hidden(GROUP_POLICE_03)
	if (IN_COOP) then
		group_create_hidden(GROUP_POLICE_03_COOP)
	end

	-- Setup the show trigger for the climax
	trigger_enable(TRIGGER_SHOW_POLICE_03, true)
	on_trigger("tss02_show_police_group", TRIGGER_SHOW_POLICE_03)
end

function tss02_show_police_group()
	-- Disable the trigger since it is no longer needed
	trigger_enable(TRIGGER_SHOW_POLICE_03, false)

	-- Show the police group when this trigger is hit
	group_show(GROUP_POLICE_03)
	if (IN_COOP) then
		group_show(GROUP_POLICE_03_COOP)
	end

	-- Check to see if a human can safely throw an item.
	local function npc_can_throw(npc)

		if ( (not character_exists(npc)) or character_is_dead(npc) or character_is_panicking(npc) ) then
			return false
		end

	end

	-- Move a cop into position to throw a smoke grenade
	set_max_hit_points("tss02_$cop27", get_max_hit_points("tss02_$cop27") * 3, true)
	move_to_safe("tss02_$cop27", "tss02_$smoke", 3, true, false)
	if (npc_can_throw("tss02_$cop27")) then
		force_throw_char("tss02_$cop27", CHARACTER_GAT)
	end
	if (npc_can_throw("tss02_$cop27")) then
		local dist, player = get_dist_closest_player_to_object("tss02_$cop27")
		force_throw_char("tss02_$cop27", player)
	end


	-- Move a cop into position to throw a flash bang
	set_max_hit_points("tss02_$cop29", get_max_hit_points("tss02_$cop29") * 3, true)
	move_to_safe("tss02_$cop29", "tss02_$flash", 3, true, false)
	if (npc_can_throw("tss02_$cop29")) then
		force_throw_char("tss02_$cop29", LOCAL_PLAYER)
	end
	if (npc_can_throw("tss02_$cop29")) then
		local dist, player = get_dist_closest_player_to_object("tss02_$cop27")
		force_throw_char("tss02_$cop29", player)
	end

	-- Have the groups attack!
	for i = 1, Num_tss02_police_group_03, 1 do
		attack(Tss02_police_group_03[i])
	end

	if (IN_COOP) then
		for i = 1, Num_tss02_police_group_03_coop, 1 do
			attack(Tss02_police_group_03_coop[i])
		end
	end
end

function tss02_visit_forgive_and_forget()

	-- Reenable police ambient spawns
	spawning_allow_cop_ambient_spawns(true)

	-- Reenable the action nodes
	action_nodes_restrict_spawning(false)

	-- Enable the confessionals and allow a free one...
	confessionals_enable(true)
	confessionals_allow_free_one(true) 

	-- Disable this trigger
	trigger_enable(TRIGGER_COURTHOUSE_EXIT, false)
	
	-- Remove the marker
	marker_remove_trigger(TRIGGER_COURTHOUSE_EXIT, SYNC_ALL)

	-- Mark the forgive and forget
	marker_add_trigger(TRIGGER_FORGIVE_FORGET, MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_LOCATION, SYNC_ALL)
	-- Add a waypoint
	waypoint_add(TRIGGER_FORGIVE_FORGET, SYNC_ALL)

	-- Enable the forgive and forget
	trigger_enable(TRIGGER_FORGIVE_FORGET, true)
	on_trigger("tss02_go_to_aishas_house", TRIGGER_FORGIVE_FORGET)

	-- Tell the player(s) to get to forgive and forget to get the FUZZ off their back
	mission_help_table("tss02_ff")

	-- Play the conversation table
	audio_play_conversation_in_vehicle(Tss02_gat_player_drive_conversation)
end

function tss02_go_to_aishas_house()
	-- Disable this trigger
	trigger_enable(TRIGGER_FORGIVE_FORGET, false)

	-- Remove the marker
	marker_remove_trigger(TRIGGER_FORGIVE_FORGET, SYNC_ALL)
	-- Remove the waypoint
	waypoint_remove(SYNC_ALL)

	-- Mark Aisha's house
	marker_add_trigger(TRIGGER_MISSION_COMPLETE, MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_LOCATION, SYNC_ALL)
	-- Add a waypoint
	waypoint_add(TRIGGER_MISSION_COMPLETE, SYNC_ALL)

	-- Enable the mission complete trigger
	trigger_enable(TRIGGER_MISSION_COMPLETE, true)
	on_trigger("tss02_arrived_at_aishas", TRIGGER_MISSION_COMPLETE)

	-- Tell the player(s) to get to Aishas house
	mission_help_table("tss02_aishas")

	-- Reset the police notoriety
	notoriety_reset("police")

	--Lose the media helicopter
	thread_new("tss02_heli_fly_away")
end

function tss02_heli_fly_away()
	if ( vehicle_exists(VEH_NEWS_HELI) and (not vehicle_is_destroyed(VEH_NEWS_HELI)) ) then
		helicopter_enter_retreat(VEH_NEWS_HELI)
	end
end

function tss02_arrived_at_aishas()
	-- Require both players to be close enough to complete
	local		local_player_close_enough = (get_dist(LOCAL_PLAYER, TRIGGER_MISSION_COMPLETE) < 10.0)
	local		remote_player_close_enough = true

	if (IN_COOP) then
		remote_player_close_enough = (get_dist(REMOTE_PLAYER, TRIGGER_MISSION_COMPLETE) < 10.0)
	end

	-- Both players must be considered close to end this mission
	if (local_player_close_enough and remote_player_close_enough) then
		-- Disable this trigger
		trigger_enable(TRIGGER_MISSION_COMPLETE, false)

		-- Stop the players' vehicles when they reach the HQ trigger
		player_controls_disable( LOCAL_PLAYER )
		vehicle_stop_do( LOCAL_PLAYER )
		if ( coop_is_active() ) then
		   player_controls_disable( REMOTE_PLAYER )
		   vehicle_stop_do( REMOTE_PLAYER )
		end

		-- Remove the marker
		marker_remove_trigger(TRIGGER_MISSION_COMPLETE, SYNC_ALL)
		-- Remove the waypoint
		waypoint_remove(SYNC_ALL)

		-- Call the mission success
		tss02_complete()
	elseif (IN_COOP) then
		-- Show a message
		mission_help_table_nag("tss02_cooperative")
	end
end

