-- mission_globals.lua
-- SR2 global mission script
-- 4/11/07

----------------------
-- Global Variables --
----------------------

--Strongold Globals
LAST_DOOR_OPENED = ""

-- How often the pre-mission phonecall function checks
-- to see if a phonecall should be made to the player
PRE_MISSION_PHONECALL_UPDATE_DELAY_SECONDS	= 10
RECEIVED_PHONECALL_FUNCTION_SUFFIX				= "_phonecall"

-- Cell phone calls received identifiers. Note that these are tracked as bits and 
-- stored as a uint16, so we're currently limited to 16 unique calls.
CELLPHONE_CALL_TSS03							= 1
CELLPHONE_CALL_BH09                    = 2
CELLPHONE_CALL_BH11                    = 3
CELLPHONE_CALL_BH01							= 4
CELLPHONE_CALL_MID_MISSION             = 5

-- TSS02 checks the route taken in TSS01, by default we will say the tutorial route was chosen
Tss01_tutorial_route_chosen				= true

-- Special checkpoint data for BH07
Bh07_checkpoint_firebombers_complete	= false
Bh07_checkpoint_swat_complete				= false
Bh07_checkpoint_store_complete			= false
Bh07_checkpoint_coordinator_complete	= false

-- Special checkpoint data for RN07
Rn07_gat_health								= 0

-- Each defense objective has an associated trigger. If a player completes an objective while they are standing
-- inside the trigger of another incomplete objective, then they should immediately start or join that objective.
-- This structure keeps track of which defense objective triggers each player occupies.
Rn10_player_defense_objectives_occupied_at_checkpoint =
	{
		[LOCAL_PLAYER]		= {},
		[REMOTE_PLAYER]	= {},
	}

-- Ep01 requires the player(s) to save Pierce and Shaundi from the Ultor. In
-- coop, these objectives can be worked on simultaneously. In single player,
-- we have a checkpoint after one ltnt is saved. We can't do this in coop
-- because the other player might be in the middle of saving the other ltnt,
-- and our checkpointing system isn't robust enough to restart in the middle
-- of a sequence. Instead, we merely save the name of the ltnt. that has been
-- saved.
Ep01_lieutenant_saved						= ""

-- Checks to see if all the missions in the passed-in table are
-- completed. Returns true if they are, false otherwise.
--
-- missions: Names of the missions to check for completion.
--
function missions_completed( missions )

   local all_completed = true
   for index, mission_name in pairs( missions ) do
      if ( mission_is_complete( mission_name ) == false ) then
         all_completed = false
      end
   end

   return all_completed
end

-- Have the player take a mid-mission phone call
--
function mid_mission_phonecall(pickup_callback)

	-- Reset the mid-mission phone call
	cellphone_reset_call( CELLPHONE_CALL_MID_MISSION )
	
	-- Do the cellphone pre-mission phone call
	audio_play_for_cellphone( CELLPHONE_INCOMING, 4, 1.5, 0.25, "RON2_INTRO_L1", "", true, pickup_callback, "", true)

end

function mid_mission_phonecall_reset()
	cellphone_remove("RON2_INTRO_L1")
   cellphone_clear_between_call_delay()
end

function mid_mission_phonecall_received()
	cellphone_receive_call(CELLPHONE_CALL_MID_MISSION)
end


-- Do the actual pre-mission phone call
--
function mission_globals_pre_mission_phonecall_do(mission_name, preq_missions, min_delay_s, max_delay_s, which_call, what_person, audio_tag, required_respect)
	-- If the mission is already considered complete...exit thread
	if ( mission_is_complete( mission_name ) ) then
		return true
	end

   -- If there's a prereq or prereqs
   if ( preq_missions ~= nil ) then
      -- Turn it into a table if it's not
      if ( type( preq_missions ) ~= "table" ) then
         preq_missions = { preq_missions }
      end

      -- If the prerequisite mission(s) are not complete delay
      while ( missions_completed( preq_missions ) == false ) do
         delay( PRE_MISSION_PHONECALL_UPDATE_DELAY_SECONDS )
      end
   end
	
	-- If we do not have enough props to do this mission then delay
	if (required_respect ~= nil) then
		while ( get_player_respect( ) < required_respect ) do
			delay( PRE_MISSION_PHONECALL_UPDATE_DELAY_SECONDS )
		end
	end
	
	-- Delay a little while before we post the phone call (2 minutes)
	delay( rand_int(min_delay_s, max_delay_s) )
	
	-- If the cell call has already been received then do not do it
	if ( cellphone_has_received_call( which_call ) ) then
		return true
	end
	
	-- Mission is not complete and the cell phone call has not been done yet so do the phone call
	if ( not mission_is_complete( mission_name ) ) then
		-- Do the cellphone pre-mission phone call
		audio_play_for_cellphone( CELLPHONE_INCOMING, 4, 1.5, 0.75, audio_tag, what_person, false, "",
                                mission_name..RECEIVED_PHONECALL_FUNCTION_SUFFIX )

		-- If the mission is in progress then we need to kill the phone call...
		--
		-- NOTE: The phone call is pre-mission only!!!
		--
		-- Wait until the phone call is considered to be received
		while (not cellphone_has_received_call(which_call)) do
			-- While the phone call has not be received, look to see if the mission it is suppose to play before is active
			if (mission_is_active(mission_name)) then
				-- We are playing the mission the phone call is suppose to happen before so kill it
				cellphone_remove(audio_tag)

				-- Wait until the mission is no longer active
				while (mission_is_active(mission_name)) do
					delay(1.0)
				end

				-- Return false so the phone call will be posted again if possible
				return false
			end

			delay(1.0)
		end
	end

	return true
end

-- Automatically handles a pre-mission phonecall.
-- IMPORTANT NOTE: Make sure that the mission_name.."_phonecall" function is defined and that it
--                 calls cellphone_receive_call() with the appropriate unique call ID, listed in
--                 the list with "CELLPHONE_CALL_"s
--
--						Also, make sure this function is spawned as a thread...if not then it may not 
--						work as intended
--
-- mission_name: Name of the mission this phonecall is being made for.
-- preq_missions: Name of the prerequisite mission or missions that must be completed before this
--                mission can be played. Set to nil if this mission doesn't have prereqs.
-- min_delay_s: The minimum delay before the call is received once all requirements are met.
-- max_delay_s: The max delay before the call is received once all requirements are met.
-- which_call: The unique ID for the call. See the "CELLPHONE_CALL_" list.
-- what_person: Who's making the call.
-- audio_tag: The audio to play for the call.
-- min_respect: The minimum amount of respect(props) the player needs to have before receiving the call.
-- 
function mission_globals_pre_mission_phonecall(mission_name, preq_missions, min_delay_s, max_delay_s, which_call, what_person, audio_tag, min_respect)
	-- Until the phone call has been received or "mission_name" has been completed, post the phone call
   while (not mission_globals_pre_mission_phonecall_do(mission_name, preq_missions, min_delay_s, max_delay_s, which_call, what_person, audio_tag, min_respect)) do
		thread_yield()
	end
end

function mission_globals_init()
	-- Init any data that is needed when the game is loaded...vary rarely should there be anything here
	-- In SR1, this was mission_add(...). For SR2, this is done through a table file
end

----------------------------------
-- Third Street Saints Prologue --
----------------------------------

function tss01_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function tss01_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function tss01_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news events
	radio_post_event("JANE_NEWS_TSSP01", true)

end

function tss02_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function tss02_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end


function tss02_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Unlock all activities and diversions except heli. Both heli instances
	-- have cutscenes w/ Shaundi and Pierce who are introduced in TSS04 so
	-- heli is unlocked after sccessfully completeing that mission.
	mission_unlock("crowd_ht")
	mission_unlock("crowd_su")
	mission_unlock("demoderby_un")
	mission_unlock("drug_ai")
	mission_unlock("drug_ht")
	mission_unlock("escort_rl")
	mission_unlock("escort_un")
	mission_unlock("fight_ar")
	mission_unlock("fight_pr")
	mission_unlock("fraud_fc")
	mission_unlock("fraud_mu")
	mission_unlock("fuzz_pj")
	mission_unlock("fuzz_sx")
	mission_unlock("mayhem_nu")
	mission_unlock("mayhem_st")
	mission_unlock("sewage_rl")
	mission_unlock("sewage_sx")
	mission_unlock("snatch_ct")
	mission_unlock("snatch_dt")
	mission_unlock("torch_ap")
	mission_unlock("torch_dt")
	mission_unlock("zombie")

end

function tss03_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function tss03_phonecall()
	cellphone_receive_call(CELLPHONE_CALL_TSS03)
end

function tss03_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
	--

	-- The player is required to have one bar of respect to start this mission
	local min_respect = 8000

	-- Spawn a thread to watch the pre-mission phone call
   thread_new("mission_globals_pre_mission_phonecall", "tss03", "tss02", 10, 20, CELLPHONE_CALL_TSS03, "Gat", "GAT_TSSP03_PHONECALL_1", min_respect)
end

function tss03_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news event
	radio_post_event("JANE_NEWS_TSSP03", true)

end

function tss04_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function tss04_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function tss04_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news event
	radio_post_event("JANE_NEWS_TSSP04", true)

	-- Unlock heli activities. Both instances include cutscenes w/ Shaundi and Pierce who
	-- aren't introduced until this mission. Most activities are unlocked after tss02.
	mission_unlock("heli_br")
	mission_unlock("heli_tp")

end


-----------------
-- Brotherhood --
-----------------

function bh01_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh01_phonecall()
	cellphone_receive_call(CELLPHONE_CALL_BH01)
end
function bh01_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
   thread_new("mission_globals_pre_mission_phonecall", "bh01", "tss04", 10, 60, CELLPHONE_CALL_BH01, "Carlos", "CARLOS_BR01_PHONECALL_2")
end

function bh02_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh02_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function bh03_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh03_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function bh04_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh04_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function bh05_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh05_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function bh06_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh06_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function bh07_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh07_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function bh08_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh08_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function bh08_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news event
	radio_post_event("JANE_NEWS_BR08", true)

end

function bh09_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh09_phonecall()
	cellphone_receive_call(CELLPHONE_CALL_BH09)
end

function bh09_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
	--
	
	-- Spawn a thread to watch the pre-mission phone call
   thread_new("mission_globals_pre_mission_phonecall", "bh09", "bh08", 10, 60, CELLPHONE_CALL_BH09, "Shaundi", "BR09_SHAUNDI_PHONECALL_1")
end

function bh10_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh10_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function bh11_phonecall()
	cellphone_receive_call(CELLPHONE_CALL_BH11)
end

function bh11_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function bh11_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
   local prereqs = { "bh10", "sh_bh_airport", "sh_bh_apartments", "sh_bh_chinatown", "sh_bh_docks" }
   
	-- Spawn a thread to watch the pre-mission phone call
   thread_new("mission_globals_pre_mission_phonecall", "bh11", prereqs, 10, 60, CELLPHONE_CALL_BH11, "Maero", "MAERO_BR11_PHONECALL_1")
end

-----------
-- Ronin --
-----------

-- Players locations are tracked during rn01. We need to declare those
-- variables here so that the locations are available when restarting
-- from checkpoints.
Rn01_local_player_location		= ""
Rn01_remote_player_location	= ""

Rn04_vehicle_homie				= ""

function rn01_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn01_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn01_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news event
	radio_post_event("JANE_NEWS_RON01", true)

end

function rn02_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn02_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn03_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn03_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn04_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn04_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn05_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn05_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn06_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn06_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn07_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn07_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn08_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn08_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn09_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn09_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn10_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn10_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn11_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function rn11_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function rn11_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news event
	radio_post_event("JANE_NEWS_RON11", true)

end

--------------------
-- Sons of Samedi --
--------------------

function ss01_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss01_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss02_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss02_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss03_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss03_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss04_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss04_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss05_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss05_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss05_on_coop_skip()
   local CHUNK_SWAP_FIRES = { "Chunk_Fire_1", "Chunk_Fire_2", "Lab_Fire_1", "Lab_Fire_2", "Lab_Fire_3" }

   for index, name in pairs( CHUNK_SWAP_FIRES ) do
      chunk_swap_sequence_start( name, false )
		
		chunk_swap_sequence_make_permanent( name )
   end

	-- Post the news event
	radio_post_event("JANE_NEWS_SOS05", true)

end

function ss06_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss06_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss07_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss07_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss08_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss08_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss09_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss09_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss10_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss10_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ss11_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ss11_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

----------------------------------
-- Third Street Saints Epilogue --
----------------------------------

function ep01_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ep01_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ep02_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ep02_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ep02_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news event
	radio_post_event("JANE_NEWS_TSSE02", true)
end

function ep03_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ep03_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ep03_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news event
	radio_post_event("JANE_NEWS_TSSE03", true)
end

function ep04_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function ep04_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end

function ep04_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news event
	radio_post_event("JANE_NEWS_TSSE04", true)
end

----------------------------------
-- Third Street Saints Emergent --
----------------------------------

function em01_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function em01_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call

	-- If this mission has been completed then just ignore
	if (mission_is_complete("em01") or mission_is_unlocked("em01")) then
		return
	end
	
	-- Make sure the prologue is finished
	while (not mission_is_complete("tss04")) do
		thread_yield()
	end

	-- Setup our trigger callback to unlock the mission
	--trigger_enable("mission_start_sr2_city_$em01_unlock", true)
	--ingame_effect_add_trigger("mission_start_sr2_city_$em01_unlock", INGAME_EFFECT_LOCATION, SYNC_ALL)
end

function em01_skip_coop()
	-- Called when the player skips a mission previously completed as a co-op client.

	-- Post the news event
	radio_post_event("JANE_NEWS_BONUS", true)

end

function em02_setup()
	-- Setup/restore mission data after being unlock/cleanup respectively.
end

function em02_main()
	-- Special process that needs to happen during or after the mission is completed...such as a cellphone call
end
