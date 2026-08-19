MAIN_MENU_FIND_SERVER_CONTROL = 1100

Main_menu_first_menu = "top"
Main_menu_screen_alpha = .8
-- load all assets needed to display everything up to and including the top main menu
-- this includes: loading, press start, and main menu
-- do nothing else, wait for call to main_menu_load_complete() to let you know when to transition
Main_menu_loading_friends_dialog_handle = 0

Main_menu = {}

Main_menu_syslink_server_data = {}

FIND_SERVER_WIDTH = 375
FIND_SERVER_HEIGHT = 393
FIND_SERVER_MAX_ITEMS = 12
FIND_SERVER_SELECT_BAR_WIDTH = 350
Main_menu_press_start_peg = -1

Multi_ranked = 0
loadFromCoop = false
search_type = 0
sort_by = 0
search_filter = ""

Main_menu_searching_LAN_games = false
Main_menu_is_coop_game = false

function Main_menu_LAN_find_game_PC()
	Main_menu_showing_coop_games = false
	Main_menu_syslink_server_data.num_items = 0
	Main_menu_searching_LAN_games = true
	Main_menu_is_coop_game = false
	Multi_ranked = 0
	vint_document_load("mp_server_browser")
	--start_find_syslink_servers()
end

function Main_menu_Online_find_game_PC()
	Main_menu_showing_coop_games = false
	Main_menu_syslink_server_data.num_items = 0
	Main_menu_searching_LAN_games = false
	Main_menu_is_coop_game = false
	Multi_ranked = 0
	vint_document_load("mp_server_browser")
	--start_find_online_servers(0)
end

function Main_menu_system_link_find_game_coop_PC()
	Main_menu_showing_coop_games = true
	Main_menu_syslink_server_data.num_items = 0
	Main_menu_searching_LAN_games = true
	Main_menu_is_coop_game = true
	Multi_ranked = 0
	vint_document_load("mp_server_browser")
	--start_find_syslink_servers()
end

function Main_menu_find_coop_online_menu_PC()
	Main_menu_syslink_server_data.num_items = 0
	Main_menu_showing_coop_games = false	
	Main_menu_searching_LAN_games = false
	Main_menu_is_coop_game = true
	Multi_ranked = 0
	vint_document_load("mp_server_browser")
	--start_coop_matchmaking()
end

function Main_menu_online_find_ranked_game_PC()
	Main_menu_showing_coop_games = false
	Main_menu_syslink_server_data.num_items = 0
	Main_menu_searching_LAN_games = false
	Main_menu_is_coop_game = false
	Multi_ranked = 1
	vint_document_load("mp_server_browser")
	--start_find_online_servers(1)
end

function Main_menu_LAN_find_game_setup_PC()
search_type = 1
Main_menu_server_filter.parent_menu = Menu_active
menu_show(Main_menu_server_filter, MENU_TRANSITION_SWAP)
end

function Main_menu_Online_find_game_setup_PC()
search_type = 2
Main_menu_server_filter.parent_menu = Menu_active
menu_show(Main_menu_server_filter, MENU_TRANSITION_SWAP)
end

function Main_menu_system_link_find_game_coop_setup_PC()
search_type = 3
Main_menu_server_filter.parent_menu = Menu_active
menu_show(Main_menu_server_filter, MENU_TRANSITION_SWAP)
end

function Main_menu_find_coop_online_menu_setup_PC()
search_type = 4
Main_menu_server_filter_short.parent_menu = Menu_active
multi_change_game_type_filter(game_mode_values.cur_value)
menu_show(Main_menu_server_filter_short, MENU_TRANSITION_SWAP)
end

function Main_menu_online_find_ranked_game_setup_PC()
search_type = 5
Main_menu_server_filter_short.parent_menu = Menu_active
game_mode_values.cur_value = 0
multi_change_game_type_filter(game_mode_values.cur_value)
menu_show(Main_menu_server_filter_short, MENU_TRANSITION_SWAP)
end


sort_by_values = { [0] = { label = "MULTI_SORT_BY_NAME", }, [1] = { label = "MULTI_SORT_BY_PING", },  cur_value = 0, num_values = 2 }

function multi_change_sort()
	sort_by = sort_by_values.cur_value
	multi_change_sort_method(sort_by)
end

function on_multi_change_name_filter(menu_label, menu_data)
	menu_open_search_filter_window()
	vint_dataresponder_request( "enter_login_PC_data_responder", "dummy_login_pc_func", 0 )
	if menu_open_enter_name_result() == 1 then
		local entered_text = menu_open_enter_name_result_string(6)
		if entered_text == nil then
			entered_text = ""
		end
		vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", entered_text )
		search_filter = entered_text
	end
	menu_resize()
end

function pc_multi_change_game_type_filter(menu_label, menu_data)
	multi_change_game_type_filter(game_mode_values.cur_value)
end

function main_menu_multi_apply_filter()
	if search_type == 1 then
		Main_menu_LAN_find_game_PC()
	elseif search_type == 2 then
		Main_menu_Online_find_game_PC()
	elseif search_type == 3 then
		Main_menu_system_link_find_game_coop_PC()
	elseif search_type == 4 then
		Main_menu_find_coop_online_menu_PC()
	elseif search_type == 5 then
		Main_menu_online_find_ranked_game_PC()
	end
end

function multi_build_search_filter()
	sort_by = multi_get_sort_method();
	sort_by_values.cur_value = sort_by;
	
	local search_filter_text = menu_open_enter_name_result_string(6)
	if search_filter_text == nil then
			search_filter_text = ""
		end
	Main_menu_server_filter[0].info_text_str = search_filter_text

end

function main_menu_init()
	
	local language = get_language()
	
	peg_load("ui_load_main")
	peg_load("ui_mainmenu")
	peg_load("ui_multiplayer")
	
	Main_menu_press_start_peg  = "ui_mm_start_" .. language 
	peg_load(Main_menu_press_start_peg)
	
	--localize press start btn
	local press_start_bmp = "ui_mainmenu_pres_start_" .. language 
	vint_set_property(vint_object_find("press_start_bmp"), "image", press_start_bmp)
	vint_set_property(vint_object_find("press_start_bmp_highlight"), "image", press_start_bmp)
	
	--Platform filter for various things
	if get_platform() == "PS3" then
		--Hide Cellphone Sales for PS3
		Main_menu_extras[1] = nil
		Main_menu_extras.num_items = 1
		
		--Change Press Start Bitmap for PS3 to text because Sony Rules!
		vint_set_property(vint_object_find("press_start_bmp"), "visible", false)
		vint_set_property(vint_object_find("press_start_bmp_highlight"), "visible", false)
		vint_set_property(vint_object_find("press_start_txt"), "visible", true)
		
		Main_menu_friends_tips = Main_menu_friends_tips_ps3
	else
		vint_set_property(vint_object_find("press_start_txt"), "visible", false)
		--setup the show gamercard ability for 360
		Main_menu_coop_join_player_menu.on_alt_select = main_menu_get_gamercard
		Main_menu_coop_join_player_menu.btn_tips = Main_menu_friends_tips
	end
	
	--Language Filter for wireless screens	
	if language ~= "US" and language ~= "DE" and language ~= "FR" and language ~= "ES" and language ~= "UK" then
		Main_menu_extras[1] = nil
		Main_menu_extras.num_items = 1		
	end
	
	--Initialize state
	Main_menu.handles = {}
	
	Main_menu.handles.loading_grp = vint_object_find("loading_grp")
	Main_menu.handles.main_menu_grp = vint_object_find("main_menu_grp")
	Main_menu.handles.press_start_grp = vint_object_find("press_start_grp")
	Main_menu.handles.ornate = vint_object_find("ornate")
	Main_menu.handles.menu_screen = vint_object_find("main_menu_screen")
	
	--Logos
	Main_menu.handles.logo_grp = vint_object_find("logo_sr2")
	Main_menu.handles.logo_bling = vint_object_find("logo_bling", Main_menu.handles.logo_grp)
	Main_menu.handles.logo_sr2 = vint_object_find("logo", Main_menu.handles.logo_grp)
	Main_menu.handles.logo_jp = vint_object_find("logo_jp", Main_menu.handles.logo_grp)
	Main_menu.handles.logo_sr2_ornate_left = vint_object_find("ornate_left", Main_menu.handles.logo_grp)
	Main_menu.handles.logo_sr2_ornate_right = vint_object_find("ornate_right", Main_menu.handles.logo_grp)
	
	if get_language() == "JP" then
		vint_set_property(Main_menu.handles.logo_jp, "visible", true)
	else
		vint_set_property(Main_menu.handles.logo_jp, "visible", false)
	end
	
	--Menu Items (Where all the items go into
	Main_menu.handles.menu_items = vint_object_find("menu_items")
	
	--Menu item
	Main_menu.handles.menu_item = vint_object_find("menu_item")
	vint_set_property(Main_menu.handles.menu_item, "visible", false)
	
	--Animations
	Main_menu.anims = {}
	
	--Intro Anims
	Main_menu.anims.intro_loading = vint_object_find("intro_loading_anim")							--Intro Loading
	Main_menu.anims.intro_loading_fadeout = vint_object_find("intro_loading_fadeout_anim")		--Loading Fadeout
	
	Main_menu.anims.intro_logo = vint_object_find("intro_logo_anim")									--Intro Logo
	Main_menu.anims.intro_ornate = vint_object_find("intro_ornate_anim")								--ornate
	Main_menu.anims.intro_press_start = vint_object_find("intro_press_start_anim")				--Shows press start btn
	Main_menu.anims.intro_menu  = vint_object_find("intro_menu_anim")									--Fades in the menu
	Main_menu.anims.menu_screen_fade  = vint_object_find("menu_screen_fade_anim")					--Darkens main menu using black bitmap

	main_menu_pause_all_anims()
	
	vint_set_property(Main_menu.anims.intro_press_start, "is_paused", true)
	
	Main_menu.anims.text_item_deselect = vint_object_find("text_item_deselect")
	Main_menu.anims.text_item_select = vint_object_find("text_item_select")
	vint_set_property(Main_menu.anims.text_item_deselect, "is_paused", true)
	vint_set_property(Main_menu.anims.text_item_select, "is_paused", true)

	--Main menu fade anim
	Main_menu.anims.menu_screen_fade = vint_object_find("menu_screen_fade_anim")
	Main_menu.anims.menu_screen_fade_twn = vint_object_find("main_menu_screen_alpha_twn_1", Main_menu.anims.menu_screen_fade)
	
	--Initialize bling
	main_menu_bling_init()

	main_menu_visual_set("init")
	
	menu_set_custom_control_callbacks(Main_menu_controls)
	
	Pause_menu_return_to_main = main_menu_restore
	Pause_menu_main_close_ref = main_menu_force_close
	
	-- prune all objects from pause menu doc to here
	local pause_safe_frame_h = vint_object_find("safe_frame", nil, vint_document_find("pause_menu"))
	local main_safe_frame_h = vint_object_find("safe_frame")
	
	local c = vint_object_first_child(pause_safe_frame_h)
	
	if c ~= nil then
		while c ~= nil do
			local n = vint_object_next_sibling(c)
			vint_object_set_parent(c, main_safe_frame_h)
			c = n
		end
	end
	
	if Main_menu_gameboot_complete == true then
		--Game is already loaded, so we know we are coming from game.(no need to press start)
		vint_set_property(Main_menu.handles.press_start_grp, "visible", false)
		main_menu_load_complete(1)
	else
		main_menu_visual_set("start_load")
	end
	
	if Main_menu_controller_selected == true then
		main_menu_controller_selected()
	end

	--	Initialize Main Menu button tips
	
	--Override button tips for main menu
	BTN_TIPS_DEFAULT.b_button.label = "CONTROL_BACK"
	Pause_save_load_btn_tips.b_button.label = "CONTROL_BACK"
	
	btn_tips_init()
	
	-- Set button tips to visible but hide the actual tips.
	local menu_tips_h = vint_object_find("menu_tips", nil, MENU_BASE_DOC_HANDLE)
	vint_set_property(menu_tips_h, "visible", true)
	local menu_btn_tips_h = vint_object_find("menu_btn_tips", nil, MENU_BASE_DOC_HANDLE)
	vint_set_property(menu_btn_tips_h, "alpha", 0)
	
	local tips_clip_h = vint_object_find("tips_clip", nil, MENU_BASE_DOC_HANDLE)
	vint_set_property(tips_clip_h, "visible", false)
end

function main_menu_cleanup()
	main_menu_close_extra_screens(false)
	peg_unload("ui_load_main")
	peg_unload("ui_mainmenu")
	peg_unload("ui_multiplayer")
	peg_unload(Main_menu_press_start_peg)
end

function main_menu_pause_all_anims()
	vint_set_property(Main_menu.anims.menu_screen_fade, "is_paused", true)
	vint_set_property(Main_menu.anims.intro_loading_fadeout, "is_paused", true)
	vint_set_property(Main_menu.anims.intro_loading, "is_paused", true)
	vint_set_property(Main_menu.anims.intro_logo, "is_paused", true)
	vint_set_property(Main_menu.anims.intro_ornate, "is_paused", true)
	vint_set_property(Main_menu.anims.intro_menu, "is_paused", true)
end

-- if percent < 1 then update the loading progress indicator
-- otherwise transition to "press start..."
function main_menu_load_complete(percent)
	if percent >= 1 then
		main_menu_visual_set("load_to_start")
	else
		--Insert Percent into loading text
		local values = {[0] = "GAME_LOADING", [1] = floor(percent*100)}
		local str = vint_insert_values_in_string("{0} {1}%%%", values)
		
		vint_set_property(vint_object_find("mm_percent_complete"), "text_tag", str)
	end
end

-- called by C when a controller has been selected - you will get no input until after this stage
-- when called, transition to main menu
function main_menu_controller_selected()
	Main_menu_controller_selected = true
	menu_input_block(true)
	main_menu_grab_input()
	menu_init()
	thread_new("main_menu_start")
	menu_input_block(false)
end

-- show main menu
function main_menu_start()
	menu_input_block(true)
	
	pause_save_get_global_info(false)	-- this can block across frames

	Menu_swap_center = true			--Change menubase to use center swapping style
	local active_frame = vint_object_find("main_menu_reg_pnt")		--Find point for centering menus
	Menu_active_anchor_end_x, Menu_active_anchor_end_y = vint_get_property(active_frame, "anchor")
	
	--Build Main menu
	main_menu_build(Main_menu_top)
	
	--Change visual state
	main_menu_visual_set("start_to_main_menu")

	menu_input_block(false)
end

function main_menu_close_extra_screens(staying_in_main_menu)
	stop_mp_tutorial(staying_in_main_menu)
	credits_force_close()
	pause_menu_force_close_brightness_screen()
	vint_document_unload(vint_document_find("mm_wireless"))
	vint_document_unload(vint_document_find("mp_player_info_popup"))
	vint_document_unload(vint_document_find("mp_leaderboards"))
	vint_document_unload(vint_document_find("mp_lobby_players"))
end

function main_menu_force_to_menu(menu_name)

	if Main_menu_controller_selected == false then
		Main_menu_first_menu = menu_name
	else
		main_menu_close_extra_screens(true)
		if menu_name == "top" then
			if Main_menu_controller_selected == true then
				menu_hide_active()
				vint_set_property(vint_object_find("safe_frame"), "visible", true)
				Main_menu_can_has_input = true
				menu_input_block(false)
				main_menu_screen_fade(0)
			end
			
			main_menu_grab_input()
			dialog_box_force_close_all()
		elseif menu_name == "coop_load" then
			credits_force_close()
			Pause_load_is_coop = true
			vint_set_property(vint_object_find("safe_frame"), "visible", true)
			loadFromCoop = false
			main_menu_load_menu_select()
		elseif menu_name == "customize" then
			vint_set_property(vint_object_find("safe_frame"), "visible", true)
			main_menu_customize_menu_select()
		end
	end
end

function main_menu_continue_success(s)
	if s == "success" then
		menu_close(main_menu_close_final)
	elseif s == "new_game" then
		main_menu_new_game_select()
	end
end

function main_menu_continue_select_deferred()
	menu_input_block(true)
	main_menu_release_input()
	vint_dataresponder_request("save_load", "main_menu_continue_success", 0, "continue")
	main_menu_grab_input()
	menu_input_block(false)
end

function main_menu_continue_select()
	
	if get_platform() == "PC" then
		set_single_player(true)
		
		if is_connected_to_network() == true then
			set_name_to_use(1)
		else
			set_name_to_use(0)
		end
	end

	mp_switch_modes(-1)

	thread_new("main_menu_continue_select_deferred")
end

function main_menu_new_game_select()
	
	if get_platform() == "PC" then
		set_single_player(true)
		if is_connected_to_network() == true then
			set_name_to_use(1)
		else
			set_name_to_use(0)
		end
		mp_switch_modes(-1)
	end

	vint_dataresponder_request("main_menu_responder", "", 0, "new_game")
end

function main_menu_close_final()
	vint_document_unload(vint_document_find("main_menu"))
end

function main_menu_show_mp_live()
	vint_set_property(vint_object_find("safe_frame"), "visible", false)
	vint_dataresponder_request("main_menu_responder", "", 0, "show_temp_mp")
end

function main_menu_show_mp_syslink()
	vint_set_property(vint_object_find("safe_frame"), "visible", false)
	vint_dataresponder_request("main_menu_responder", "", 0, "show_temp_syslink")
end

-- called by C to tell you to close
function main_menu_force_close()
	main_menu_btn_tips_show(false)
	menu_close(main_menu_close_final)	
end

function main_menu_customize_menu_select()
	main_menu_release_input()
	
	--Darken main menu
	main_menu_screen_fade(Main_menu_screen_alpha)
	
	menu_show(Main_menu_cuztomize_mp_player, MENU_TRANSITION_SWAP)
	
	--setup menu tree, hacky
	if get_platform() == "PS3" then
		Main_menu_cuztomize_mp_player.parent_menu = Main_menu_ps3_online
		Main_menu_ps3_online.parent_menu = Main_menu_multiplayer_ps3
	else
		Main_menu_cuztomize_mp_player.parent_menu = Main_menu_online_pc
		Main_menu_online_pc.parent_menu = Main_menu_multiplayer_pc
	end
end

function main_menu_load_menu_select()

	if get_platform() == "PC" then
		if loadFromCoop == true then
			set_single_player(false)
		else
			set_single_player(true)
		end
		
		if is_connected_to_network() == true then
			set_name_to_use(1)
		else
			set_name_to_use(0)
		end
		
		mp_switch_modes(-1)
	end

	Pause_load_menu.parent_menu = Menu_active
	if Main_menu_can_has_input == true or Menu_active == Pause_load_menu then
		Pause_load_menu.on_back = main_menu_load_back_restore
	else
		Pause_load_menu.on_back = main_menu_load_back_parent
	end
	main_menu_release_input()

	pause_load_menu_select()
end

function main_menu_load_coop_online()
	Pause_load_from_menu = PAUSE_LOAD_FROM_MENU_COOP_ONLINE
	loadFromCoop = true
	main_menu_load_menu_select()
end

function main_menu_load_coop_syslink()
	Pause_load_from_menu = PAUSE_LOAD_FROM_MENU_COOP_SYSLINK
	loadFromCoop = true
	main_menu_load_menu_select()
end

-- called by system to activate attract mode
function main_menu_attract_mode(active)
	--Hides/shows main menu based on active flag
	vint_set_property(vint_object_find("safe_frame"), "visible", active == false)
	
	--Button Tips
	local menu_tips_h = vint_object_find("menu_tips", nil, MENU_BASE_DOC_HANDLE)
	vint_set_property(menu_tips_h, "visible", active == false)
	menu_input_block(active)
end

function main_menu_tutorial_mode(active)
	--Hides/shows main menu based on active flag
	vint_set_property(vint_object_find("safe_frame"), "visible", active == false)
	
	--Button Tips
	local menu_tips_h = vint_object_find("menu_tips", nil, MENU_BASE_DOC_HANDLE)
	vint_set_property(menu_tips_h, "visible", active == false)
end

function main_menu_roll_credits(menu_label, menu_data)
	credits_roll_credits()
end

function Main_menu_online_logout()
	mp_online_logout()
	main_menu_restore()
end

function main_menu_wireless(menu_label, menu_data)
	menu_input_block(true)
	vint_document_load("mm_wireless")
end

function main_menu_wireless_complete()
	menu_input_block(false)
end

--[[ Main menu building ]]--
Main_menu_styles = {
	highlight_tint = { r = 0.8980, g = 0.7490, b = 0.054},
	normal_tint = { r = 0.6314, g = 0.6314, b = 0.634},
	highlight_scale = 1,
	normal_scale = .8,
	glow_highlight_alpha = .35,
	glow_normal_alpha = 0,
}

function main_menu_build(menu)
	local menu_grp = vint_object_find("menu_items", Main_menu.handles.main_menu_grp)
	local pos_y = -2
	
	local text_width, text_height, width, height, x, y
	local menu_item_h, text_h, glow_h
	local glow_left_h, glow_right_h, glow_mid_h
	
	-- clean the old elements out
	local o = vint_object_first_child(menu_grp)
	local o2
	while o ~= nil do
		o2 = vint_object_next_sibling(o)
		vint_object_destroy(o)
		o = o2
	end
	
	-- populate
	local item_num = 0
	
	--Use continue?
	if Pause_save_global_info.continue_avail == true then
		Main_menu_top.current_selection = 0
	else
		Main_menu_top.current_selection = 1
	end

	-- add continue item
	menu[item_num] = Main_menu_master[0]
	item_num = item_num + 1
	
	--Populate rest of items
	for i = 1, Main_menu_master.num_items - 1 do
		menu[item_num] = Main_menu_master[i]
		item_num = item_num + 1
	end
	
	menu.num_items = item_num
	
	--Build Elements
	for i = 0, menu.num_items - 1 do

		--Clone menu items
		menu_item_h = vint_object_clone(Main_menu.handles.menu_item)
		vint_object_set_parent(menu_item_h, Main_menu.handles.menu_items)
		
		--Find elements within item to manipulate
		text_h = vint_object_find("item_text", menu_item_h)
		glow_h = vint_object_find("item_glow", menu_item_h)
		
		--Set Text
		vint_set_property(text_h , "text_tag", menu[i].label)
		
		--Rescale Glow
		glow_left_h = vint_object_find("glow_left", glow_h)
		glow_right_h = vint_object_find("glow_right", glow_h) 
		glow_mid_h = vint_object_find("glow_mid", glow_h)
		
		text_width, text_height = element_get_actual_size(text_h)
		width, height = element_get_actual_size(glow_mid_h)
		text_width = text_width - 20
		
		element_set_actual_size(glow_mid_h, text_width, height)
		x, y = vint_get_property(glow_mid_h, "anchor")
		vint_set_property(glow_left_h, "anchor", -text_width/2, y)
		vint_set_property(glow_right_h, "anchor", text_width/2, y)
		
		if i == Main_menu_top.current_selection then
			--Highlight first item
			vint_set_property(Main_menu.anims.text_item_select, "target_handle", menu_item_h)
			vint_set_property(menu_item_h, "depth", -50)
			lua_play_anim(Main_menu.anims.text_item_select, -2)
		else
			vint_set_property(text_h, "tint", Main_menu_styles.normal_tint.r, Main_menu_styles.normal_tint.g, Main_menu_styles.normal_tint.b)
			vint_set_property(menu_item_h, "scale", Main_menu_styles.normal_scale, Main_menu_styles.normal_scale)
			vint_set_property(glow_h, "alpha", Main_menu_styles.glow_normal_alpha)
		end
		
		vint_set_property(menu_item_h, "anchor", 0, pos_y)
		
		--Set Item to visible
		vint_set_property(menu_item_h, "visible", true)
		
		--Reposition
		pos_y = pos_y + 30
		menu[i].item_h = menu_item_h
		menu[i].text_h = text_h
		menu[i].glow_h = glow_h
	end
end

function main_menu_load_back_parent()
	vint_dataresponder_request("save_load", "", 0, "load_cancelled")
	if Menu_active.parent_menu ~= nil and Menu_active ~= Menu_active.parent_menu then
		menu_show(Menu_active.parent_menu, MENU_TRANSITION_SWEEP_RIGHT)
	else
		main_menu_restore()
	end
end

function main_menu_load_back_restore()
	vint_dataresponder_request("save_load", "", 0, "load_cancelled")
	main_menu_restore()
end

function main_menu_restore()
	main_menu_grab_input()
	
	--Undarken main menu
	main_menu_screen_fade(0)
	menu_hide_active()
end

function main_menu_grab_input()
	Main_menu_can_has_input = true
end

function main_menu_release_input()
	Main_menu_can_has_input = false
end

function main_menu_update_nav_bar(new_sel)
	local cur_item = Main_menu_top[Main_menu_top.current_selection]
	local cur_item_h = cur_item.item_h
	local anim_h = vint_object_clone(Main_menu.anims.text_item_deselect)
	vint_set_property(anim_h , "target_handle", cur_item_h)
	vint_set_property(cur_item_h, "depth", 0)
	
	--Reset tween states for fade out
	local val_r, val_g, val_b = vint_get_property(cur_item.text_h, "tint")
	local twn_h = vint_object_find("menu_item_select", anim_h)
	vint_set_property(twn_h , "start_value", val_r, val_g, val_b)
	local val = vint_get_property(cur_item.glow_h, "alpha")
	twn_h = vint_object_find("item_glow_alpha_twn_1", anim_h)
	vint_set_property(twn_h , "start_value", val )
	vint_set_property(twn_h , "end_event", "main_menu_item_deselect_cleanup")
	
	local new_item = Main_menu_top[new_sel]
	local new_item_h = new_item.item_h
	vint_set_property(Main_menu.anims.text_item_select, "target_handle", new_item_h)
	vint_set_property(new_item_h, "depth", -50)
	
	lua_play_anim(anim_h)
	lua_play_anim(Main_menu.anims.text_item_select)

	Main_menu_top.current_selection = new_sel
end

function main_menu_item_deselect_cleanup(twn_h)
	--Cleansup used animations/tweens
	vint_object_destroy(vint_object_parent(twn_h))
end

function main_menu_nav(target, event)
	if menu_input_is_blocked() == true then
		return
	end

	if event == "nav_up" then
		local cur_selection = Main_menu_top.current_selection - 1
		
		if cur_selection < 0 then
			cur_selection = Main_menu_top.num_items - 1
		end

		main_menu_update_nav_bar(cur_selection)
		audio_play(Menu_sound_item_nav)

	elseif event == "nav_down" then
		local cur_selection = Main_menu_top.current_selection + 1
		
		if cur_selection >= Main_menu_top.num_items then
			cur_selection = 0
		end

		main_menu_update_nav_bar(cur_selection)
		audio_play(Menu_sound_item_nav)

	elseif event == "select" then
		local item = Main_menu_top[Main_menu_top.current_selection]
		if item.on_select ~= nil then
			item.on_select()
			audio_play(Menu_sound_select)
		elseif item.sub_menu ~= nil then
			main_menu_release_input()
			main_menu_screen_fade(Main_menu_screen_alpha)
			item.sub_menu.highlighted_item = 0	--Always select first item in list
			menu_show(item.sub_menu, MENU_TRANSITION_SWAP)
			audio_play(Menu_sound_select)
		end
	end
end

function main_menu_visual_set(state) 
	if state == "init" then
		--Hide everything
		vint_set_property(Main_menu.handles.loading_grp, "alpha", 0)
		vint_set_property(Main_menu.handles.main_menu_grp, "alpha", 0)
		vint_set_property(Main_menu.handles.press_start_grp, "alpha", 0)
		vint_set_property(Main_menu.handles.logo_bling, "alpha", 0)
		vint_set_property(Main_menu.handles.logo_sr2, "alpha", 0)
		vint_set_property(Main_menu.handles.logo_sr2_ornate_left, "alpha", 0)
		vint_set_property(Main_menu.handles.logo_sr2_ornate_right, "alpha", 0)
		vint_set_property(Main_menu.handles.menu_screen, "alpha", 0)
	elseif state == "start_load" then
		--Fade into loading
		lua_play_anim(Main_menu.anims.intro_loading)
	elseif state == "load_to_start" then
		--Fade out loading
		vint_object_destroy(Main_menu.anims.intro_loading)
		lua_play_anim(Main_menu.anims.intro_loading_fadeout)
		
		--Fade in other elements
		lua_play_anim(Main_menu.anims.intro_ornate, 0)
		lua_play_anim(Main_menu.anims.intro_logo, .2)
		lua_play_anim(Main_menu.anims.intro_press_start, .6)
		
		--Start bling
		main_menu_play_bling()	
	elseif state == "start_to_main_menu" then
		vint_set_property(Main_menu.handles.loading_grp, "alpha", 0)
		
		vint_set_property(Main_menu.anims.intro_press_start, "is_paused", true)
		lua_play_anim(Main_menu.anims.intro_menu,.5)
		audio_play(audio_get_audio_id("SYS_MENU_SELECT"))
	end
end

function main_menu_screen_fade(target_alpha)
	--Fades menu to the alpha value
	local cur_alpha = vint_get_property(Main_menu.handles.menu_screen, "alpha")
	vint_set_property(Main_menu.anims.menu_screen_fade_twn, "start_value", cur_alpha)
	vint_set_property(Main_menu.anims.menu_screen_fade_twn, "end_value", target_alpha)
	lua_play_anim(Main_menu.anims.menu_screen_fade, 0)
	
	--if we are faded in show button tips
	if target_alpha > 0 then
		--Show button tips
		main_menu_btn_tips_show(true)
	else
		--Hide button tips
		main_menu_btn_tips_show(false)
	end
end

--coop
function main_menu_maybe_show_coop_menu()
	--see if we're connected to the network
	--if is_connected_to_network() == false then
	--	main_menu_release_input()
	--	main_menu_screen_fade(Main_menu_screen_alpha)
	--	menu_show( Main_menu_multiplayer_sign_in_to_gamespy, MENU_TRANSITION_SWAP )
	--	return
	--end
	--release input
	
	if get_platform() == "PC" then
		set_single_player(false)
	end
	
	main_menu_release_input()
	--Darken main menu
	main_menu_screen_fade(Main_menu_screen_alpha)

	main_manu_switch_controls_coop()

	mp_switch_modes(-1)

	Main_menu_coop_ps3.highlighted_item = 0
	menu_show(Main_menu_coop_pc, MENU_TRANSITION_SWAP)
end

function main_menu_maybe_show_online_pc()
	--see if we're connected to the network
	if is_connected_to_network() == false then
		main_menu_release_input()
		Main_menu_multiplayer_sign_in_to_gamespy.parent_menu = Main_menu_multiplayer_pc
		main_menu_screen_fade(Main_menu_screen_alpha)
		menu_show( Main_menu_multiplayer_sign_in_to_gamespy, MENU_TRANSITION_SWEEP_LEFT )
		return
	end
	if get_platform() == "PC" then
		set_name_to_use(1);
	end
	menu_show(Main_menu_online_pc,  MENU_TRANSITION_SWEEP_LEFT)
end

function main_menu_show_coop_online_menu()
	if user_has_online_privlage() == false then
		if get_platform() ~= "XBOX360" then
			dialog_box_message("MENU_TITLE_WARNING", "MULTI_NO_PRIVILEGE_PS3")
		else
			dialog_box_message("MENU_TITLE_WARNING", "MULTI_NO_PRIVILEGE")
		end
		return
	end
	
	if get_platform() ~= "XBOX360" then
		if get_platform() == "PC" then
			menu_show(Main_menu_coop_online_pc, MENU_TRANSITION_SWAP)
		else
		menu_show(Main_menu_coop_online_ps3,  MENU_TRANSITION_SWEEP_LEFT)
		end
	else
		menu_show(Main_menu_coop_online_xbox,  MENU_TRANSITION_SWEEP_LEFT)
	end
end

function main_menu_maybe_show_coop_online()
	if is_connected_to_service() == false then	
		if get_platform() ~= "XBOX360" then
			if get_platform() == "PC" then
				--see if we're connected to the network
				if is_connected_to_network() == false then
					main_menu_release_input()
					Main_menu_multiplayer_sign_in_to_gamespy.parent_menu = Main_menu_coop_pc
					main_menu_screen_fade(Main_menu_screen_alpha)
					menu_show( Main_menu_multiplayer_sign_in_to_gamespy, MENU_TRANSITION_SWEEP_LEFT )
					return
				end
			end
			login_to_online_network(true)
		elseif user_has_online_privlage() == false then
			dialog_box_message("MENU_TITLE_WARNING", "MULTI_NO_PRIVILEGE")
		else
			dialog_box_message("MENU_TITLE_WARNING", "NOT_LOGGED_IN_XBOX")
		end
		return
	end
	
	--good, open the menu
	
	if get_platform() == "PC" then
		set_name_to_use(1);
	end
	
	main_menu_show_coop_online_menu()
end

function main_menu_maybe_show_coop_quickmatch()
	--see if we're connected to online netowrk and have privlages
	if is_connected_to_service() == false or user_has_online_privlage() == false then
		if get_platform() ~= "XBOX360" then
			dialog_box_message("MENU_TITLE_WARNING", "MULTI_NO_PRIVILEGE_PS3")
		else
			dialog_box_message("MENU_TITLE_WARNING", "MULTI_NO_PRIVILEGE")
		end
		return
	end
	--good, open the menu
	menu_show(Main_menu_empty_menu, MENU_TRANSITION_SWAP)
end

function main_menu_start_public_coop()
	
	set_coop_join_type(0)
	coop_start_new_live()
		
	vint_dataresponder_request("main_menu_responder", "", 0, "new_game")
end

function main_menu_start_friends_coop()
	set_coop_join_type(1)
	coop_start_new_live()
	vint_dataresponder_request("main_menu_responder", "", 0, "new_game")
end

function main_menu_start_private_coop()
	set_coop_join_type(2)
	coop_start_new_live()
	vint_dataresponder_request("main_menu_responder", "", 0, "new_game")
end

function main_menu_start_syslink_coop()
	set_coop_join_type(0)
	coop_start_new_syslink()
	vint_dataresponder_request("main_menu_responder", "", 0, "new_game")
end

function main_menu_start_coop_quickmatch()

end

--coop friends list
function main_menu_coop_join_friend()
	main_menu_join_friend_in_progress(Menu_active.highlighted_item);
end

function  main_menu_populate_friends(display_name)
	local num_items = Main_menu_coop_join_player_menu.num_items
	--add item yay
	Main_menu_coop_join_player_menu[num_items] = {label = display_name, type = MENU_ITEM_TYPE_SELECTABLE, on_select = main_menu_coop_join_friend 	}
	Main_menu_coop_join_player_menu.num_items = num_items + 1

end

function  main_menu_build_join_player_request()
	Main_menu_coop_join_player_menu.num_items = 0
	vint_dataresponder_request("sr2_main_menu_friends", "main_menu_populate_friends", 0) 
	if Main_menu_loading_friends_dialog_handle ~= 0 then
		dialog_box_force_close(Main_menu_loading_friends_dialog_handle)
		Main_menu_loading_friends_dialog_handle = 0
	end
	if Main_menu_coop_join_player_menu.num_items == 0 then
		dialog_box_message("MENU_TITLE_NOTICE", "NO_FRIENDS_ONLINE")
	else
		menu_show(Main_menu_coop_join_player_menu, MENU_TRANSITION_SWEEP_LEFT)
	end
end

function  main_menu_load_friends()
	--open a dialog box
	Main_menu_loading_friends_dialog_handle = dialog_box_message("GAME_LOADING", "MULTI_INVITE_FILTER_FRIENDS")
	--request the friends
	thread_new("main_menu_build_join_player_request")
end

--multiplayer functions
function main_menu_ethernet_disconnected()
	vint_document_unload(vint_document_find("mp_player_info_popup"))
	vint_document_unload(vint_document_find("mp_leaderboards"))
	
--	main_menu_grab_input()
	--see if we have a mp menu active
	if Menu_active == nil or Menu_active == 0 then
		return
	end
	
	--we're only in the Pause_info_sub_menu in the main menu if we're online
	if Menu_active.multi_menu == true or Menu_active == Pause_info_sub_menu then
		--we're in a multiplayer menu, RETREAT!!
		main_menu_restore()
		--pop up a dialogue saying why we kicked them out, make this critical so we don't close it
		if is_mp_tutorial_playing() == false then
			if get_platform() ~= "XBOX360" then
				if vint_document_find("mp_server_browser") then --CDPLC close server browser if its opened
					vint_document_unload(vint_document_find("mp_server_browser"))
				end
				if get_disconnected_reason() ~= 6 then
					dialog_box_message_critical("MENU_TITLE_WARNING", "MULTI_LOST_CONNECTION_SYS")
				end
			else
				if vint_document_find("mp_leaderboards") ~= 0 then
					dialog_box_message_critical("MENU_TITLE_WARNING", "MULTI_LOST_CONNECTION")
				else
					dialog_box_message_critical("MENU_TITLE_WARNING", "MULTI_NO_ETHERNET")
				end
			end
		end
	end
end

function main_menu_internet_disconnected()
	vint_document_unload(vint_document_find("mp_player_info_popup"))
	vint_document_unload(vint_document_find("mp_leaderboards"))
	
--	main_menu_grab_input()
	--see if we have a mp menu active
	if Menu_active == nil or Menu_active == 0 then
		return
	end
	
	--we're only in the Pause_info_sub_menu in the main menu if we're online
	if Menu_active.multi_internet_menu == true or Menu_active == Pause_info_sub_menu then
		--kill any low priority dialog boxes
		dialog_box_force_close_all()
		--we're in a multiplayer menu, RETREAT!!
		main_menu_restore()
		--pop up a dialogue saying why we kicked them out, make this critical so we don't close it
		if is_mp_tutorial_playing() == false then
			if get_platform() ~= "XBOX360" then
				dialog_box_message("MENU_TITLE_WARNING", "MULTI_CONNECTION_LOST_PS3")
			else
				dialog_box_message("MENU_TITLE_WARNING", "MULTI_LOST_CONNECTION")
			end
		end
	end
end

function main_menu_maybe_show_multiplayer_menu()
	--release input
	main_menu_release_input()
	--Darken main menu
	main_menu_screen_fade(Main_menu_screen_alpha)

	Main_menu_multiplayer_ps3.highlighted_item = 0
    menu_show(Main_menu_multiplayer_pc, MENU_TRANSITION_SWAP)
end

function main_menu_show_mp_online_menu()
	if user_has_online_privlage() == false then
		if get_platform() ~= "XBOX360" then
			dialog_box_message("MENU_TITLE_WARNING", "MULTI_NO_PRIVILEGE_PS3")
		else
			dialog_box_message("MENU_TITLE_WARNING", "MULTI_NO_PRIVILEGE")
		end
		return
	end
	
	--maybe prompt them for customization
	if is_mp_storage_blank() then
		dialog_box_message("MENU_TITLE_NOTICE", "MP_CUSTOMIZATION_PROMPT")
	end
	
	if get_platform() ~= "XBOX360" then
		menu_show(Main_menu_online_pc, MENU_TRANSITION_SWEEP_LEFT)
	else
		menu_show(Main_menu_xbox_live, MENU_TRANSITION_SWEEP_LEFT)
	end
end

function main_menu_maybe_show_online_menu()
	if is_connected_to_service() == false then
		if get_platform() ~= "XBOX360" then
			login_to_online_network(false)
		elseif user_has_online_privlage() == false then
			dialog_box_message("MENU_TITLE_WARNING", "MULTI_NO_PRIVILEGE")
		else
			dialog_box_message("MENU_TITLE_WARNING", "NOT_LOGGED_IN_XBOX")
		end
		return
	end
	
	--good, open the menu
	main_menu_show_mp_online_menu()
end

function main_menu_build_online_menu()
	debug_print("vint","main_menu: building online servers\n")
	Main_menu_syslink_server_data.num_items = 0
	Main_menu_showing_coop_games = false	

	Multi_ranked = 0

	start_find_online_servers(0);
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "update", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "insert", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "remove", "main_menu_syslink_menu_update")
	debug_print("vint","main_menu: building online servers size: ".. Main_menu_system_link_find_game.num_items .."\n")

end

function main_menu_build_online_ranked_menu()
	debug_print("vint","main_menu: building online servers\n")
	Main_menu_syslink_server_data.num_items = 0
	Main_menu_showing_coop_games = false	

	Multi_ranked = 1

	start_find_online_servers(1);
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "update", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "insert", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "remove", "main_menu_syslink_menu_update")
	debug_print("vint","main_menu: building online servers size: ".. Main_menu_system_link_find_game.num_items .."\n")

end

function main_menu_build_coop_online_menu()
	Main_menu_syslink_server_data.num_items = 0
	Main_menu_showing_coop_games = false	

	start_coop_matchmaking();
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "update", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "insert", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "remove", "main_menu_syslink_menu_update")
	debug_print("vint","main_menu: building online servers size: ".. Main_menu_system_link_find_game.num_items .."\n")
end

function main_menu_cleanup_online_menu()
	vint_datagroup_remove_subscription("sr2_syslink_servers_group", "update", "main_menu_syslink_menu_update")
	vint_datagroup_remove_subscription("sr2_syslink_servers_group", "insert", "main_menu_syslink_menu_update")
	vint_datagroup_remove_subscription("sr2_syslink_servers_group", "remove", "main_menu_syslink_menu_update")
	stop_find_online_servers()
end

function main_menu_start_ranked_quickmatch()
	screen_fx_fadeout(0)
	mp_start_quickmatch(true)
end

function main_menu_start_unranked_quickmatch()
	screen_fx_fadeout(0)
	mp_start_quickmatch(false)
end

function main_menu_start_lobby(lobby_type)
	mp_start_lobby(lobby_type)
end

function main_menu_start_public_lobby()
	main_menu_start_lobby(0)
end

function main_menu_start_friends_lobby()
	main_menu_start_lobby(1)
end
	
function main_menu_start_private_lobby()
	main_menu_start_lobby(2)
end	

function main_menu_start_syslink_lobby()
	main_menu_start_lobby(5)
end

function main_menu_show_mp_stats()

end

function main_menu_show_mp_leaderboard()
	if is_sake_ready() == 1 then
		vint_document_load("mp_leaderboards")
	end
end

function main_menu_start_player_create()
	main_menu_start_lobby(3)
end

function main_menu_start_player_customize()
	main_menu_start_lobby(4)
end

function main_menu_start_player_wardrobe()
	main_menu_start_lobby(6)
end

function main_menu_start_mp_help()

end

function main_menu_get_gamercard()
	debug_print("vint", "Get friend gamercard here!\n")
	show_main_menu_friend_gamercard(Menu_active.highlighted_item)
end

function main_menu_add_syslink_host(display_name)
	local num_servers = Main_menu_system_link_find_game.num_values
	Main_menu_system_link_find_game.num_values = num_servers + 1
	Main_menu_system_link_find_game[num_servers] = { label = display_name, type = MENU_ITEM_TYPE_SELECTABLE,	on_select = nil}
end

Main_menu_showing_coop_games = false

function main_menu_syslink_menu_update(di_h, event)

	local display_string, is_mp, id = vint_dataitem_get(di_h)
	
	if Main_menu_showing_coop_games == true then
		if is_mp == true then
			mp_player_has_favorite_match_settings()
			return
		end
	elseif is_mp == false then
		mp_load_custom_match_settings_from_favorite()
		return
	end

	if event == "insert" then 
		--see if we already have this item 
		for index = 0, Main_menu_syslink_server_data.num_items - 1 do
			if Main_menu_syslink_server_data[index].id == id then
				--we already got disnot ar
				return
			end
		end
		--add new item
		Main_menu_syslink_server_data[Main_menu_syslink_server_data.num_items] = {display_string = display_string, id = id}
		Main_menu_syslink_server_data.num_items = Main_menu_syslink_server_data.num_items + 1
	elseif event == "remove" then
		--compress the table.. for loop blah
		local removed = false
		local my_highlight = Menu_option_labels[0].hlight_bar_pos
		if my_highlight == nil then
			my_highlight = 0
		end
		for index = 0, Main_menu_syslink_server_data.num_items - 1 do
			local test_var = true
			if Main_menu_syslink_server_data[index].id == id then
				removed = true
				Main_menu_syslink_server_data[index] = nil
				--do we need to move the bar up?
				if my_highlight >= index and my_highlight ~= 0 then
					Menu_option_labels[0].hlight_bar_pos = my_highlight - 1
				end
				test_var = false
			end
			if removed and test_var then
				Main_menu_syslink_server_data[index - 1] = Main_menu_syslink_server_data[index]
			end
		end
		
		if removed then
			Main_menu_syslink_server_data.num_items = Main_menu_syslink_server_data.num_items -1
		end
	elseif event == "update" then
		if Main_menu_syslink_server_data[di_h] ~= nil then
			-- Update the values
			Main_menu_syslink_server_data[di_h].display_string = display_string
			Main_menu_syslink_server_data[di_h].id = id
		end
	end

	--update the menu
	if Menu_active == Main_menu_system_link_find_game or Menu_active == Main_menu_system_link_find_game_coop or Menu_active == Main_menu_online_find_game or Menu_active == Main_menu_online_find_ranked_game then
		find_server_control_update(Menu_option_labels[0], Menu_active[0]);
	end

end

function main_menu_build_syslink_menu()
	Main_menu_showing_coop_games = false
	Main_menu_syslink_server_data.num_items = 0
	
	start_find_syslink_servers();
	
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "update", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "insert", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "remove", "main_menu_syslink_menu_update")
end

function main_menu_build_syslink_menu_coop()
	Main_menu_syslink_server_data.num_items = 0
	Main_menu_showing_coop_games = true
	start_find_syslink_servers();
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "update", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "insert", "main_menu_syslink_menu_update")
	vint_datagroup_add_subscription("sr2_syslink_servers_group", "remove", "main_menu_syslink_menu_update")

end

function main_menu_cleanup_syslink_menu()
	vint_datagroup_remove_subscription("sr2_syslink_servers_group", "update", "main_menu_syslink_menu_update")
	vint_datagroup_remove_subscription("sr2_syslink_servers_group", "insert", "main_menu_syslink_menu_update")
	vint_datagroup_remove_subscription("sr2_syslink_servers_group", "remove", "main_menu_syslink_menu_update")
	stop_find_syslink_servers()
	
end

function find_server_control_get_width()
	return FIND_SERVER_WIDTH
end

function find_server_control_get_height()
	return FIND_SERVER_HEIGHT
end

function main_menu_join_syslink_server()
	if Find_server_control_no_games == true then
		return
	end
	--find the id of the server we currently have selected
	local selection = Menu_option_labels[0].hlight_bar_pos
	local server_id = Main_menu_syslink_server_data[selection].id
	join_syslink_game(server_id)
end

function main_menu_join_online_server()
	--find the id of the server we currently have selected
	local selection = Menu_option_labels[0].hlight_bar_pos
	local server_id = Main_menu_syslink_server_data[selection].id
	join_online_game(server_id, Multi_ranked)
	--stop_find_syslink_servers()
end

function main_menu_join_online_ranked_server()
	--find the id of the server we currently have selected
	local selection = Menu_option_labels[0].hlight_bar_pos
	local server_id = Main_menu_syslink_server_data[selection].id
	join_online_game(server_id, 1)
	--stop_find_syslink_servers()
end

function main_menu_join_online_coop_server()
	--find the id of the server we currently have selected
	local selection = Menu_option_labels[0].hlight_bar_pos
	local server_id = Main_menu_syslink_server_data[selection].id
	join_online_coop_game(server_id)

end

function main_menu_start_tutorial_mode()
	start_mp_tutorial()
end

--mp help stuff
function main_menu_populate_help(description_tag)
	local idx = Main_menu_mp_help_info.num_items
	Main_menu_mp_help_info[idx] = { label = description_tag, type = PAUSE_MENU_CONTROL_HELP_TEXT }
	Main_menu_mp_help_info.num_items = 1	--	Should never be more than one
end

function main_menu_build_help_menu_info(menu_data)
	if Menu_active.do_not_rebuild_previous == true and Menu_active.parent_menu == menu_data then
		Menu_active.do_not_rebuild = nil
		return
	end
	
	local hl_item = Menu_active.highlighted_item
	if hl_item == nil then
		hl_item = 0
	end

	local idx = Menu_active[hl_item].id
	
	-- Set the header
	menu_data.header_label_str = Menu_active[hl_item].label
	
	--prep
	menu_data.get_width = pause_menu_control_help_text_get_width
	menu_data.get_height = pause_menu_control_help_text_get_height
	menu_data.do_not_rebuild_previous = true
		
	--	Prepare to populate
	menu_data.num_items = 0
	menu_data.highlighted_item = 0
	
	--???
	if Menu_active[hl_item].index ~= nil then
		hl_item = Menu_active[hl_item].index
	end
	
	--	Populate the menu
	vint_dataresponder_request("pause_menu_populate", "main_menu_populate_help", 0, idx, hl_item)
	
	if menu_data.num_items == 0 then
		menu_data.num_items = 1
		menu_data[0] = { label = "STORE_NO_ITEMS_IN_CATEGORY", type=PAUSE_MENU_CONTROL_OBJECTIVE_TEXT_LINE }
	end

end

function main_menu_populate_help_topics(title_tag)
	local idx = Main_menu_mp_help_topics.num_items
	Main_menu_mp_help_topics[idx] = { label = title_tag, type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_mp_help_info, id = 13 }
	Main_menu_mp_help_topics.num_items = Main_menu_mp_help_topics.num_items + 1
end

function main_menu_build_help_menu_topics(menu_data)
	if Menu_active.do_not_rebuild_previous == true and Menu_active.parent_menu == menu_data then
		Menu_active.do_not_rebuild = nil
		return
	end
	
	local hl_item = Menu_active.highlighted_item
	if hl_item == nil then
		hl_item = 0
	end

	local idx = Menu_active[hl_item].id
	
	-- Set the header
	menu_data.header_label_str = Menu_active[hl_item].label
	
	--prep
	menu_data.do_not_rebuild_previous = true

	--	Prepare to populate
	menu_data.num_items = 0
	menu_data.highlighted_item = 0
	
	--???
	if Menu_active[hl_item].index ~= nil then
		hl_item = Menu_active[hl_item].index
	end
	
	--	Populate the menu
	vint_dataresponder_request("pause_menu_populate", "main_menu_populate_help_topics", 0, 12, 13) --12 = help topics, 13 = mp help
	
	if menu_data.num_items == 0 then
		menu_data.num_items = 1
		menu_data[0] = { label = "STORE_NO_ITEMS_IN_CATEGORY", type=PAUSE_MENU_CONTROL_OBJECTIVE_TEXT_LINE }
	end

end

Main_menu_friends_tips = {
	a_button 	= 	{ label = "CONTROL_SELECT", 					enabled = btn_tips_default_a, },
	x_button 	= 	{ label = "MULTI_MENU_SHOW_GAMERCARD", 			enabled = btn_tips_default_a, },
	b_button 	= 	{ label = "MENU_BACK", 						enabled = btn_tips_default_a, },
}

Main_menu_friends_tips_ps3 = {
	a_button 	= 	{ label = "CONTROL_SELECT", 					enabled = btn_tips_default_a, },
	b_button 	= 	{ label = "MENU_BACK", 						enabled = btn_tips_default_a, },
}
function main_manu_switch_controls_syslink(menu_data)
	menu_set_custom_control_callbacks(Main_menu_controls)
end

function main_manu_switch_controls_online(menu_data)
	menu_set_custom_control_callbacks(Main_menu_controls_online)
	if get_platform() == "PC" then
		show_password_window_pc();
	end
end

function main_manu_switch_controls_coop(menu_data)
	menu_set_custom_control_callbacks(Main_menu_controls)
end

function maybe_show_login_window()
	if get_platform() == "PC" then
		show_login_window_pc();
	end
end

function main_menu_exit_game_cb_pc(result, action)
	if action ~= DIALOG_ACTION_CLOSE then
		return
	end
	if result == 0 then
		pc_invoke_exit_game()
	end
end

function main_menu_exit_game_pc(menu_data)
	local header = "MAINMENU_EXIT"
	local body = "MAINMENU_EXIT_PROMPT" 	-- Do you really want to quit ?
	local options = { [0] = "CONTROL_YES", [1] = "CONTROL_NO" }
	dialog_box_open(header, body, options, "main_menu_exit_game_cb_pc", 0, DIALOG_PRIORITY_ACTION)
end

function dummy_login_pc_func()
end

function on_multi_login_enter_login(menu_label, menu_data)
	menu_open_email_window()
	vint_dataresponder_request( "enter_login_PC_data_responder", "dummy_login_pc_func", 0 )
	if menu_open_enter_name_result() == 1 then
		local entered_text = menu_open_enter_name_result_string(0)
		if entered_text == nil then
			entered_text = ""
		end
		vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", entered_text )
	end
	menu_resize()
end

function on_multi_login_enter_pass(menu_label, menu_data)
	menu_open_password_window()
	vint_dataresponder_request( "enter_login_PC_data_responder", "dummy_login_pc_func", 0 )
	if menu_open_enter_name_result() == 1 then
		local entered_text = menu_open_enter_name_result_string(1)
		if entered_text == nil then
			vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", "" )
		else
			vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", "***" )
		end
	end
	menu_resize()
end
function on_multi_login_enter_repass(menu_label, menu_data)
	menu_open_repassword_window()
	vint_dataresponder_request( "enter_login_PC_data_responder", "dummy_login_pc_func", 0 )
	if menu_open_enter_name_result() == 1 then
		local entered_text = menu_open_enter_name_result_string(997)
		if entered_text == nil then
			vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", "" )
		else
			vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", "***" )
		end
	end
	menu_resize()
end

function on_multi_login_enter_player_name(menu_label, menu_data)
	menu_open_player_name_window()
	vint_dataresponder_request( "enter_login_PC_data_responder", "dummy_login_pc_func", 0 )
	if menu_open_enter_name_result() == 1 then
		local entered_text = menu_open_enter_name_result_string(2)
		if entered_text == nil then
			entered_text = ""
		end
		vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", entered_text )
	end
	menu_resize()
end

function on_multi_login_enter_player_syslink_name(menu_label, menu_data)
	menu_open_player_syslink_name_window()
	vint_dataresponder_request( "enter_login_PC_data_responder", "dummy_login_pc_func", 0 )
	if menu_open_enter_name_result() == 1 then
		local entered_text = menu_open_enter_name_result_string(4)
		if entered_text == nil then
			entered_text = ""
		end
		vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", entered_text )
	end
	menu_resize()
end

function on_multi_login_enter_session_pass(menu_label, menu_data)
	menu_open_multi_game_password_window()
	vint_dataresponder_request( "enter_login_PC_data_responder", "dummy_login_pc_func", 0 )
	if menu_open_enter_name_result() == 1 then
		local entered_text = menu_open_enter_name_result_string(3)
		if entered_text == nil then
			vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", "" )
		else
			vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", "***" )
		end
	end
	menu_resize()
end

function on_coop_login_enter_session_pass(menu_label, menu_data)
	menu_open_coop_game_password_window()
	vint_dataresponder_request( "enter_login_PC_data_responder", "dummy_login_pc_func", 0 )
	if menu_open_enter_name_result() == 1 then
		local entered_text = menu_open_enter_name_result_string(7)
		if entered_text == nil then
			vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", "" )
		else
			vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", "***" )
		end
	end
	menu_resize()
end

function on_multi_login_enter_session_name(menu_label, menu_data)
	menu_open_game_name_window()
	vint_dataresponder_request( "enter_login_PC_data_responder", "dummy_login_pc_func", 0 )
	if menu_open_enter_name_result() == 1 then
		local entered_text = menu_open_enter_name_result_string(5)
		if entered_text == nil then
			entered_text = ""
		end
		vint_set_property(vint_object_find("value_text", menu_label.control.grp_h), "text_tag", entered_text )
	end
	menu_resize()
end

function on_multi_login_try_login()
	try_and_login_to_gamespy()
end

function on_multi_login_create()
	local error = try_and_create_profile()
	if error == 0 then
		local options = { [0] = "PAUSE_MENU_ACCEPT" }
		dialog_box_open("MULTI_MENU_CREATE_PROFILE", "MULTI_MENU_CREATE_PROFILE_SUCCESSFUL", options, "multi_menu_log_restore", 0, DIALOG_PRIORITY_ACTION)
	elseif error == 1 then
		-- server error
		local options = { [0] = "PAUSE_MENU_ACCEPT" }
		dialog_box_open("MULTI_MENU_ERROR", "MULTI_MENU_SERVER_ERROR", options, "dummy_login_pc_func", 0, DIALOG_PRIORITY_ACTION)
	elseif error == 2 then 
		-- connection error
		local options = { [0] = "PAUSE_MENU_ACCEPT" }
		dialog_box_open("MULTI_MENU_ERROR", "MULTI_MENU_CONNECTION_ERROR", options, "dummy_login_pc_func", 0, DIALOG_PRIORITY_ACTION)
	elseif error == 3 then
		-- bad nick
		local options = { [0] = "PAUSE_MENU_ACCEPT" }
		dialog_box_open("MULTI_MENU_ERROR", "MULTI_MENU_BAD_NICK_ERROR", options, "dummy_login_pc_func", 0, DIALOG_PRIORITY_ACTION)
	elseif error == 4 then
		-- bad password
		local options = { [0] = "PAUSE_MENU_ACCEPT" }
		dialog_box_open("MULTI_MENU_ERROR", "MULTI_MENU_BAD_PASSWORD_ERROR", options, "dummy_login_pc_func", 0, DIALOG_PRIORITY_ACTION)
	elseif error == 5 then
		-- misc error
		local options = { [0] = "PAUSE_MENU_ACCEPT" }
		dialog_box_open("MULTI_MENU_ERROR", "MULTI_MENU_MISC_ERROR", options, "dummy_login_pc_func", 0, DIALOG_PRIORITY_ACTION)
	elseif error == 666 then
		-- server error
		local options = { [0] = "PAUSE_MENU_ACCEPT" }
		dialog_box_open("MULTI_MENU_ERROR", "MULTI_MENU_PASS_REPASS_DIFFERS", options, "dummy_login_pc_func", 0, DIALOG_PRIORITY_ACTION)
	end
end

function after_gamespy_loggin()
	main_menu_restore()
--	main_menu_maybe_show_multiplayer_menu()
end

function main_menu_multiplayer_create_account_build()
	local t1 = menu_open_enter_name_result_string(0)
	if t1 == nil then
		t1 = ""
	end
	Main_menu_multiplayer_create_account_gamespy[0].info_text_str = t1

	local t2 = menu_open_enter_name_result_string(1)
	if t2 == nil then
		t2 = ""
	else
		t2 = "***"
	end
	Main_menu_multiplayer_create_account_gamespy[1].info_text_str = t2
	Main_menu_multiplayer_create_account_gamespy[2].info_text_str = t2

	menu_reset_repass()

	local t3 = menu_open_enter_name_result_string(2)
	if t3 == nil then
		t3 = ""
	end
	Main_menu_multiplayer_create_account_gamespy[3].info_text_str = t3

end

function main_menu_multiplayer_sign_in_build()
	local t1 = menu_open_enter_name_result_string(0)
	if t1 == nil then
		t1 = ""
	end
	Main_menu_multiplayer_sign_in_to_gamespy[0].info_text_str = t1

	local t2 = menu_open_enter_name_result_string(1)
	if t2 == nil then
		t2 = ""
	else
		t2 = "***"
	end
	Main_menu_multiplayer_sign_in_to_gamespy[1].info_text_str = t2

	local t3 = menu_open_enter_name_result_string(2)
	if t3 == nil then
		t3 = ""
	end
	Main_menu_multiplayer_sign_in_to_gamespy[2].info_text_str = t3

end

function main_menu_multiplayer_syslink_sign_in_build()
	local t1 = menu_open_enter_name_result_string(4)
	if t1 == nil then
		t1 = ""
	end
	Main_menu_system_link[0].info_text_str = t1
	
	if get_platform() == "PC" then
		set_name_to_use(0);
	end
end

function main_menu_coop_syslink_sign_in_build()
	local t1 = menu_open_enter_name_result_string(4)
	if t1 == nil then
		t1 = ""
	end
	Main_menu_coop_syslink[0].info_text_str = t1
	
	if get_platform() == "PC" then
		set_name_to_use(0);
	end
end

Main_menu_empty_menu = {
	header_label_str = "NOT IMPLEMENTED",
	num_items = 1,
	on_back = main_menu_restore,
	[0] = { label = "DON'T CLICK THIS", type = MENU_ITEM_TYPE_SELECTABLE},
}

Main_menu_quickmatch = {
	header_label_str = "MULTI_MENU_QUICK_MATCH",
	num_items = 2,
	on_map = main_menu_restore,
	multi_menu = true,
	multi_internet_menu = true,

	[0] = { label = "MULTI_MENU_RANKED_MATCH", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_ranked_quickmatch},
	[1] = { label = "MULTI_MENU_PLAYER_MATCH", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_unranked_quickmatch},
}

ranked_values = { [0] = { label = "MULTI_MENU_PLAYER_MATCH", }, [1] = { label = "MULTI_MENU_RANKED_MATCH", }, [2] = { label = "MULTI_PARTY_GAME", }, cur_value = 0, num_values = 3 }
game_type_values = { [0] = { label = "MULTI_MODE_13", }, [1] = { label = "MULTI_MODE_14", }, [2] = { label = "MULTI_MODE_STRONGARM", }, cur_value = 0, num_values = 3 }
game_mode_values = { [0] = { label = "MULTI_MENU_PLAYER_MATCH", }, [1] = { label = "MULTI_PARTY_GAME", }, cur_value = 0, num_values = 2 }

function multi_player_switch_match_rank_type_pc()
	--change the ranking type
	local rank_val = ranked_values.cur_value
	--local ranked = false
	--if rank_val == 1 then
	--	ranked = true
	--end
	mp_change_ranked_game_type(rank_val)
end

function multi_player_switch_match_type_pc()
	--change the game type
	mp_switch_modes(game_type_values.cur_value)
end

function multi_player_build_ranked_type_pc()
	mp_switch_modes(game_type_values.cur_value)
	
	local is_ranked = get_mp_is_ranked()
	if is_ranked == true then
		ranked_values.cur_value = 1
	else
		ranked_values.cur_value = 0
	end
	mp_change_ranked_game_type(ranked_values.cur_value)
	
	--is_ranked = get_mp_is_ranked()
	--if is_ranked == true then
	--	ranked_values.cur_value = 1
	--else
	--	ranked_values.cur_value = 0
	--end
	
	local selected_mp_mode = get_mp_mode_index()
	game_type_values.cur_value = selected_mp_mode
		
	local gamename = menu_open_enter_name_result_string(5)
	if gamename == nil then
		gamename = ""
	end
	Main_menu_ranked_type_pc[3].info_text_str = gamename
	
	local pass = menu_open_enter_name_result_string(3)
	if pass == nil then
		pass = ""
	else
		pass = "***"
	end
	Main_menu_ranked_type_pc[4].info_text_str = pass
end

function start_online_game_pc()
	start_online_game()
end

Main_menu_ranked_type_pc = {
	header_label_str = "MULTI_MENU_GAME_OPTIONS",
	num_items = 5,
	on_map = main_menu_restore,
	on_show = multi_player_build_ranked_type_pc,
	multi_menu = true,
	multi_internet_menu = true,
	
	[0] = { label = "MULTI_LOBBY_START_GAME", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = start_online_game_pc },
	[1] = { label = "MULTI_MATCH_TYPE", 		type = MENU_ITEM_TYPE_TEXT_SLIDER, text_slider_values = ranked_values, on_value_update = multi_player_switch_match_rank_type_pc },
	[2] = { label = "MULTI_LOBBY_GAME_MODE", 		type = MENU_ITEM_TYPE_TEXT_SLIDER, text_slider_values = game_type_values, on_value_update = multi_player_switch_match_type_pc },
	[3] = { label = "MULTI_SESSION_NAME",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "", on_select = on_multi_login_enter_session_name },
	[4] = { label = "MULTI_SESSION_PASS",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "", on_select = on_multi_login_enter_session_pass },
}

Main_menu_start_lobby = {
	header_label_str = "MULTI_MENU_CREATE_PARTY",
	num_items = 3,
	on_map = main_menu_restore,
	multi_menu = true,
	multi_internet_menu = true,

	[0] = { label = "MULTI_LOBBY_OPEN", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_public_lobby},
	[1] = { label = "MULTI_LOBBY_FRIENDS_ONLY", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_friends_lobby},
	[2] = { label = "MULTI_LOBBY_INVITE_ONLY", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_private_lobby},
}

Main_menu_start_lobby_ps3 = {
	header_label_str = "MULTI_MENU_CREATE_PARTY",
	num_items = 2,
	on_map = main_menu_restore,
	multi_menu = true,
	multi_internet_menu = true,

	[0] = { label = "MULTI_LOBBY_FRIENDS_ONLY", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_friends_lobby},
	[1] = { label = "MULTI_LOBBY_INVITE_ONLY", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_private_lobby},
}

Main_menu_cuztomize_mp_player = {
	header_label_str = "MULTI_MENU_CUSTOMIZE",
	num_items = 3,
	on_map = main_menu_restore,
	multi_menu = true,
	multi_internet_menu = true,
	
	[0] = { label = "HELP_TEXT_SURGEON_TITLE", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_player_create	},
	[1] = { label = "MAP_HIDDENNAME_STORE", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_player_customize	},
	[2] = { label = "MULTI_MENU_CUSTOMIZATION_WARDROBE", type = MENU_ITEM_TYPE_SELECTABLE, on_select = main_menu_start_player_wardrobe },
}

Main_menu_ps3_online = {
	header_label_str = "MP_GANGERS_ONLINE",
	num_items = 6,
	on_map = main_menu_restore,
	multi_menu = true,
	multi_internet_menu = true,
	
	[0] = { label = "MULTI_MENU_XBOX_QUICK_MATCH", type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_quickmatch },
	[1] = { label = "MULTI_MENU_CREATE_PARTY_TEXT", type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_start_lobby_ps3 },
	[2] = { label = "MULTI_JOIN_FRIEND",					type = MENU_ITEM_TYPE_SELECTABLE, 	on_select = main_menu_load_friends },
	[3] = { label = "MULTI_MENU_XBOX_STATS", type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Pause_info_sub_menu, id = 1},
	[4] = { label = "MULTI_MENU_XBOX_LEADERBOARDS", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_show_mp_leaderboard	},
	[5] = { label = "MULTI_MENU_CUSTOMIZE", type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Main_menu_cuztomize_mp_player	},
}

Main_menu_xbox_live = {
	header_label_str = "MENU_XBOX_LIVE_TITLE",
	num_items = 6,
	on_map = main_menu_restore,
	multi_menu = true,
	multi_internet_menu = true,
	
	[0] = { label = "MULTI_MENU_XBOX_QUICK_MATCH", type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_quickmatch },
	[1] = { label = "MULTI_MENU_CREATE_PARTY_TEXT", type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_start_lobby },
	[2] = { label = "MULTI_JOIN_FRIEND",					type = MENU_ITEM_TYPE_SELECTABLE, 	on_select = main_menu_load_friends },
	[3] = { label = "MULTI_MENU_XBOX_STATS",  type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Pause_info_sub_menu, id = 1},
	[4] = { label = "MULTI_MENU_XBOX_LEADERBOARDS", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_show_mp_leaderboard	},
	[5] = { label = "MULTI_MENU_CUSTOMIZE", type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Main_menu_cuztomize_mp_player	},
}

Main_menu_system_link_find_game = {
	header_label_str = "MULTI_SYSLINK_LIST_HEADER",
	num_items = 1,
	on_show = main_menu_build_syslink_menu,
	on_exit = main_menu_cleanup_syslink_menu,
	get_width = find_server_control_get_width,
	get_height = find_server_control_get_height,
	multi_menu = true,
	on_select = main_menu_join_syslink_server,
	on_map = main_menu_restore,

	[0] = { label = "none", type = MAIN_MENU_FIND_SERVER_CONTROL, on_select = main_menu_join_syslink_server},
}

Main_menu_online_find_game = {
	header_label_str = "MULTI_SYSLINK_LIST_HEADER",
	num_items = 1,
	on_show = main_menu_build_online_menu,
	on_exit = main_menu_cleanup_online_menu,
	get_width = find_server_control_get_width,
	get_height = find_server_control_get_height,
	multi_menu = true,

	[0] = { label = "none", type = MAIN_MENU_FIND_SERVER_CONTROL, on_select = main_menu_join_online_server},
}

Main_menu_online_find_ranked_game = {
	header_label_str = "MULTI_SYSLINK_LIST_HEADER",
	num_items = 1,
	on_show = main_menu_build_online_ranked_menu,
	on_exit = main_menu_cleanup_online_menu,
	get_width = find_server_control_get_width,
	get_height = find_server_control_get_height,
	multi_menu = true,

	[0] = { label = "none", type = MAIN_MENU_FIND_SERVER_CONTROL, on_select = main_menu_join_online_ranked_server},
}

Main_menu_system_link = {
	header_label_str = "MULTI_LAN",
	num_items = 3,
	on_map = main_menu_restore,
	multi_menu = true,
	on_show = main_menu_multiplayer_syslink_sign_in_build,
	
	[0] = { label = "MULTI_NAME", type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "Unknown", on_select = on_multi_login_enter_player_syslink_name },
	--[1] = { label = "MULTI_FIND_GAMES", type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_system_link_find_game },
	[1] = { label = "MULTI_FIND_GAMES", type = MENU_ITEM_TYPE_SELECTABLE, on_select = Main_menu_LAN_find_game_PC },
	[2] = { label = "MULTI_LOBBY_START_GAME", type = MENU_ITEM_TYPE_SELECTABLE, on_select = main_menu_start_syslink_lobby },
}

Main_menu_online_pc = {
    header_label_str = "MP_GANGERS_ONLINE",
    num_items = 7,
    multi_menu = true,
    on_show = main_manu_switch_controls_online,
    
    [0] = { label = "MULTI_FIND_GAMES", type = MENU_ITEM_TYPE_SELECTABLE, on_select = Main_menu_Online_find_game_setup_PC },
    --[1] = { label = "MULTI_FIND_RANKED_GAMES", type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_online_find_ranked_game },
    [1] = { label = "MULTI_FIND_RANKED_GAMES", type = MENU_ITEM_TYPE_SELECTABLE, on_select = Main_menu_online_find_ranked_game_setup_PC },
    [2] = { label = "MULTI_LOBBY_START_GAME", type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Main_menu_ranked_type_pc},
	[3] = { label = "MULTI_MENU_XBOX_STATS",  type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Pause_info_sub_menu, id = 1},
	[4] = { label = "MULTI_MENU_XBOX_LEADERBOARDS", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_show_mp_leaderboard	},
	[5] = { label = "MULTI_MENU_CUSTOMIZE", type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Main_menu_cuztomize_mp_player	},
	[6] = { label = "MULTI_GAMESPY_LOGOUT", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = Main_menu_online_logout	},
}

Main_menu_mp_help_topics = {
	num_items = 0,
	on_show = main_menu_build_help_menu_topics,
	multi_menu = true,
}

Main_menu_mp_help_info = {
	num_items = 0,
	on_show = main_menu_build_help_menu_info,
	multi_menu = true,
	btn_tips = Pause_menu_back_only,
}

Main_menu_multiplayer_ps3 = {
	header_label_str = "MAINMENU_MULTI",
	num_items = 4,
	on_back = main_menu_restore,
	on_map = main_menu_restore,
	multi_menu = true,
	
	[0] = { label = "MP_GANGERS_ONLINE",			type = MENU_ITEM_TYPE_SELECTABLE, on_select = main_menu_maybe_show_online_menu },
	[1] = { label = "MULTI_GAMETYPE_1",				type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_system_link },
	[2] = { label = "INFO_HELP", 					type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Main_menu_mp_help_topics},
	[3] = { label = "MSGLOG_TITLE_TUT",						type = MENU_ITEM_TYPE_SELECTABLE, on_select = main_menu_start_tutorial_mode },
}

Main_menu_multiplayer = {
	header_label_str = "MAINMENU_MULTI",
	num_items = 4,
	on_back = main_menu_restore,
	on_map = main_menu_restore,
	multi_menu = true,
	
	[0] = { label = "MAINMENU_XBOX_LIVE",			type = MENU_ITEM_TYPE_SELECTABLE, on_select = main_menu_maybe_show_online_menu },
	[1] = { label = "MULTI_LAN",					type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_system_link },
	[2] = { label = "INFO_HELP", 					type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Main_menu_mp_help_topics},
	[3] = { label = "MSGLOG_TITLE_TUT",						type = MENU_ITEM_TYPE_SELECTABLE, on_select = main_menu_start_tutorial_mode },
}

Main_menu_multiplayer_pc = {
	header_label_str = "MAINMENU_MULTI",
	num_items = 4,
	on_back = main_menu_restore,
	on_map = main_menu_restore,
	multi_menu = true,	

	[0] = { label = "MP_GANGERS_ONLINE",			type = MENU_ITEM_TYPE_SUB_MENU, on_select = main_menu_maybe_show_online_pc },
	[1] = { label = "MULTI_LAN",				type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_system_link },
	[2] = { label = "INFO_HELP", 					type = MENU_ITEM_TYPE_SUB_MENU,	sub_menu = Main_menu_mp_help_topics},
	[3] = { label = "MSGLOG_TITLE_TUT",				type = MENU_ITEM_TYPE_SELECTABLE, on_select = main_menu_start_tutorial_mode },
}

Main_menu_coop_privacy = {
	
	header_label_str = "MAINMENU_NEW",
	num_items = 3,
	on_map = main_menu_restore,
	multi_menu = true,
	multi_internet_menu = true,

	[0] = { label = "MULTI_LOBBY_OPEN", 		type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_public_coop},
	[1] = { label = "MULTI_LOBBY_FRIENDS_ONLY", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_friends_coop},
	[2] = { label = "MULTI_LOBBY_INVITE_ONLY", 	type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_private_coop},

}


Main_menu_coop_join_player_menu = {
	num_items = 1,
	on_map = main_menu_restore,
	header_label_str = "MULTI_JOIN_FRIEND",
	multi_menu = true,
	multi_internet_menu = true,
	
	[0] =  { label = "MM_LOAD_LOADING", type = MENU_ITEM_TYPE_SELECTABLE, on_select = nil },
}

Main_menu_system_link_find_game_coop = {
	header_label_str = "MULTI_SYSLINK_LIST_HEADER",
	num_items = 1,
	on_map = main_menu_restore,
	on_show = main_menu_build_syslink_menu_coop,
	on_exit = main_menu_cleanup_syslink_menu,
	get_width = find_server_control_get_width,
	get_height = find_server_control_get_height,
	multi_menu = true,
	on_select = main_menu_join_syslink_server,

	[0] = { label = "none", type = MAIN_MENU_FIND_SERVER_CONTROL, on_select = main_menu_join_syslink_server},
}

Main_menu_coop_syslink = {
	header_label_str = "MULTI_LAN",
	num_items = 4,
	on_map = main_menu_restore,
	multi_menu = true,
	on_show = main_menu_coop_syslink_sign_in_build,
	
	[0] = { label = "MULTI_NAME", type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "Unknown", on_select = on_multi_login_enter_player_syslink_name },
	--[1] = { label = "MULTI_FIND_GAMES", 		type = MENU_ITEM_TYPE_SUB_MENU, 	sub_menu = Main_menu_system_link_find_game_coop },
	[1] = { label = "MULTI_FIND_GAMES", 		type = MENU_ITEM_TYPE_SELECTABLE, 	on_select = Main_menu_system_link_find_game_coop_PC },
	[2] = { label = "MAINMENU_NEW", 			type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_start_syslink_coop },
	[3] = { label = "SAVELOAD_LOAD_GAME", 		type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_load_coop_syslink},
	
}

Main_menu_coop_online_xbox = {
	header_label_str = "MAINMENU_XBOX_LIVE",
	num_items = 4,
	on_map = main_menu_restore,
	multi_menu = true,
	multi_internet_menu = true,
	
	[3] = { label = "MULTI_MENU_XBOX_QUICK_MATCH",	type = MENU_ITEM_TYPE_SELECTABLE, 	on_select =  start_coop_matchmaking },
	[0] = { label = "MULTI_JOIN_FRIEND",					type = MENU_ITEM_TYPE_SELECTABLE, 	on_select = main_menu_load_friends },
	[1] = { label = "MAINMENU_NEW",					type = MENU_ITEM_TYPE_SUB_MENU, 	sub_menu = Main_menu_coop_privacy },
	[2] = { label = "SAVELOAD_LOAD_GAME", 			type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_load_coop_online},
	
}

Main_menu_coop_online_ps3 = {
	header_label_str = "MP_GANGERS_ONLINE",
	num_items = 4,
	on_map = main_menu_restore,
	multi_menu = true,
	multi_internet_menu = true,

	[3] = { label = "MULTI_MENU_XBOX_QUICK_MATCH",	type = MENU_ITEM_TYPE_SELECTABLE, 	on_select =  start_coop_matchmaking },
	[0] = { label = "MULTI_JOIN_FRIEND",					type = MENU_ITEM_TYPE_SELECTABLE, 	on_select = main_menu_load_friends },
	[1] = { label = "MAINMENU_NEW",					type = MENU_ITEM_TYPE_SUB_MENU, 	sub_menu = Main_menu_coop_privacy },
	[2] = { label = "SAVELOAD_LOAD_GAME", 			type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_load_coop_online},
	
}

Main_menu_find_coop_online_menu = {
	header_label_str = "MULTI_SYSLINK_LIST_HEADER",
	num_items = 1,
	on_map = main_menu_restore,
	on_show = main_menu_build_coop_online_menu,
	on_exit = main_menu_cleanup_syslink_menu,
	get_width = find_server_control_get_width,
	get_height = find_server_control_get_height,
	multi_menu = true,
	on_select = main_menu_join_online_coop_server,

	[0] = { label = "none", type = MAIN_MENU_FIND_SERVER_CONTROL, on_select = main_menu_join_online_coop_server},
}

function multi_menu_coop_build_save_new_pc()

	local gamename = menu_open_enter_name_result_string(5)
	if gamename == nil then
		gamename = ""
	end
	Main_menu_coop_online_save_new_pc[2].info_text_str = gamename

	local pass = menu_open_enter_name_result_string(7)
	if pass == nil then
		pass = ""
	else
		pass = "***"
	end
	Main_menu_coop_online_save_new_pc[3].info_text_str = pass
end

Main_menu_coop_online_save_new_pc = {
	header_label_str = "MP_GANGERS_ONLINE",
	num_items = 4,
	multi_menu = true,
	on_show = multi_menu_coop_build_save_new_pc,
	
	[0] = { label = "MAINMENU_NEW",					type = MENU_ITEM_TYPE_SELECTABLE, 	on_select = main_menu_start_public_coop },
	[1] = { label = "SAVELOAD_LOAD_GAME", 			type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_load_coop_online},
	[2] = { label = "MULTI_SESSION_NAME",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "", on_select = on_multi_login_enter_session_name },
	[3] = { label = "MULTI_SESSION_PASS",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "", on_select = on_coop_login_enter_session_pass },
}

Main_menu_coop_online_pc = {
	header_label_str = "MP_GANGERS_ONLINE",
	num_items = 3,
	multi_menu = true,
	on_show = show_password_window_pc,

	--[0] = { label = "MULTI_FIND_GAMES",				type = MENU_ITEM_TYPE_SELECTABLE, 	on_select =  Main_menu_find_coop_online_menu_setup_PC },
	--[1] = { label = "MAINMENU_NEW",					type = MENU_ITEM_TYPE_SELECTABLE, 	on_select = main_menu_start_public_coop },
	--[2] = { label = "SAVELOAD_LOAD_GAME", 			type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_load_coop_online},
	
	[0] = { label = "MULTI_FIND_GAMES",				type = MENU_ITEM_TYPE_SELECTABLE, 	on_select =  Main_menu_find_coop_online_menu_setup_PC },
	[1] = { label = "MAINMENU_CREATE",				type = MENU_ITEM_TYPE_SUB_MENU, 	sub_menu = Main_menu_coop_online_save_new_pc },
	[2] = { label = "MULTI_GAMESPY_LOGOUT", type = MENU_ITEM_TYPE_SELECTABLE,	on_select = Main_menu_online_logout	},
}

Main_menu_coop_ps3 = {
	header_label_str = "MAINMENU_COOP",
	num_items = 2,
	on_map = main_menu_restore,
	on_back = main_menu_restore,
	multi_menu = true,

	[0] = { label = "MP_GANGERS_ONLINE", 		type = MENU_ITEM_TYPE_SELECTABLE,		on_select = main_menu_maybe_show_coop_online },
	[1] = { label = "MULTI_GAMETYPE_1",			type = MENU_ITEM_TYPE_SUB_MENU, 		sub_menu = Main_menu_coop_syslink },
	
}

Main_menu_coop_pc = {
	header_label_str = "MAINMENU_COOP",
	num_items = 2,
	on_map = main_menu_restore,
	on_back = main_menu_restore,
	multi_menu = true,
	--on_show = maybe_show_login_window,

	[0] = { label = "MP_GANGERS_ONLINE", 		type = MENU_ITEM_TYPE_SELECTABLE,		on_select = main_menu_maybe_show_coop_online },
	[1] = { label = "MULTI_LAN",			type = MENU_ITEM_TYPE_SUB_MENU, 		sub_menu = Main_menu_coop_syslink },
	
}

Main_menu_coop_xbox = {
	header_label_str = "MAINMENU_COOP",
	num_items = 2,
	on_map = main_menu_restore,
	on_back = main_menu_restore,
	multi_menu = true,
	
	[0] = { label = "MAINMENU_XBOX_LIVE", 		type = MENU_ITEM_TYPE_SELECTABLE,		on_select = main_menu_maybe_show_coop_online },
	[1] = { label = "MULTI_GAMETYPE_1",			type = MENU_ITEM_TYPE_SUB_MENU, 		sub_menu = Main_menu_coop_syslink },
	
}

Main_menu_extras = {
	header_label_str = "MAINMENU_EXTRAS",
	num_items = 2,
	on_map = main_menu_restore,
	on_back = main_menu_restore,
	
	[0] = { label = "MAINMENU_CREDITS",			type = MENU_ITEM_TYPE_SELECTABLE,  on_select = main_menu_roll_credits		},
	[1] = { label = "MAINMENU_WIRELESS",			type = MENU_ITEM_TYPE_SELECTABLE,  on_select = main_menu_wireless		},
}

Main_menu_top = {
	num_items = 0,
	current_selection = 0,
}

Main_menu_sr2ap_info = {
	header_label_str = "SR2AP_MAIN_MENU_LABEL",
	num_items = 2,
	on_map = main_menu_restore,
	on_back = main_menu_restore,

	[0] = { label = "SR2AP_MAIN_MENU_SEED_LABEL", type = MENU_ITEM_TYPE_SELECTABLE },
	[1] = { label = "SR2AP_MAIN_MENU_PLAYER_LABEL", type = MENU_ITEM_TYPE_SELECTABLE },
}

Main_menu_master = {
	num_items = 8,

	[0] = { label = "MAINMENU_CONTINUE", 		type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_continue_select	},
	[1] = { label = "MAINMENU_NEW",				type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_new_game_select	},
	[2] = { label = "MAINMENU_LOAD",				type = MENU_ITEM_TYPE_SELECTABLE,	on_select = main_menu_load_menu_select	},
	[3] = { label = "MAINMENU_COOP", 			type = MENU_ITEM_TYPE_SELECTABLE	},	
	[4] = { label = "MAINMENU_MULTI",			type = MENU_ITEM_TYPE_SELECTABLE	}, 
	[5] = { label = "MAINMENU_OPTIONS", 		type = MENU_ITEM_TYPE_SUB_MENU,		sub_menu = Pause_options_menu	}, 
	[6] = { label = "SR2AP_MAIN_MENU_LABEL",			type = MENU_ITEM_TYPE_SUB_MENU,  sub_menu = Main_menu_sr2ap_info		},
	--[6] = { label = "MAINMENU_EXTRAS",			type = MENU_ITEM_TYPE_SUB_MENU,		sub_menu = Main_menu_extras	}, 
	-- PC
	[7] = { label = "MAINMENU_EXIT",			type = MENU_ITEM_TYPE_SUB_MENU,		on_select = main_menu_exit_game_pc	}, 
}

function multi_menu_log_restore()
	menu_show(Main_menu_multiplayer_sign_in_to_gamespy, MENU_TRANSITION_SWEEP_RIGHT)
	audio_play(Menu_sound_back)
end


Main_menu_multiplayer_create_account_gamespy = {
	header_label_str = "MULTI_MENU_CREATE_ACCOUNT",
	num_items = 5,
	on_back = multi_menu_log_restore,
	on_map = multi_menu_log_restore,
	on_show = main_menu_multiplayer_create_account_build,
	multi_menu = true,
	
	[0] = { label = "MULTI_GAMESPY_EMAIL",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "Zupa@ala.pl.aoiuoiquoweuoiuaoswww.eweqiowe", on_select = on_multi_login_enter_login },
	[1] = { label = "MULTI_GAMESPY_PASS",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "*****", on_select = on_multi_login_enter_pass },
	[2] = { label = "MULTI_GAMESPY_RETYPE_PASS",	type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "*****", on_select = on_multi_login_enter_repass },
	[3] = { label = "MULTI_GAMESPY_NAME",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "nick", on_select = on_multi_login_enter_player_name },  
	[4] = { label = "MULTI_CREATE_ACCOUNT",			type = MENU_ITEM_TYPE_SELECTABLE, on_select = on_multi_login_create },
}

Main_menu_multiplayer_sign_in_to_gamespy = {
	header_label_str = "MULTI_GAMESPY_ID",
	num_items = 5,
	--on_back = main_menu_restore,
	--on_exit = main_menu_restore,
	on_map = main_menu_restore,
	on_show = main_menu_multiplayer_sign_in_build,
	multi_menu = true,
	
	[0] = { label = "MULTI_GAMESPY_EMAIL",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "Zupa@ala.pl.aoiuoiquoweuoiuaoswww.eweqiowe", on_select = on_multi_login_enter_login },
	[1] = { label = "MULTI_GAMESPY_PASS",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "*****", on_select = on_multi_login_enter_pass },
	[2] = { label = "MULTI_GAMESPY_NAME",			type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "ziutek", on_select = on_multi_login_enter_player_name },
	[3] = { label = "MULTI_LOG_INTO_GAMESPY",		type = MENU_ITEM_TYPE_SELECTABLE, on_select = on_multi_login_try_login },
	[4] = { label = "MULTI_LOG_CREATE_ACCOUNT",		type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Main_menu_multiplayer_create_account_gamespy },
}

Main_menu_server_filter = {
	header_label_str = "MAINNEMU_SERVER_FILTER",
	num_items = 4,
	--on_map = main_menu_restore,
	--on_back = main_menu_restore,
	on_show = multi_build_search_filter,
	multi_menu = true,
	multi_internet_menu = true,
	
	[0] = { label = "MULTI_NAME_FILTER",		type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "", on_select = on_multi_change_name_filter },
	[1] = { label = "MULTI_SORT_BY",			type = MENU_ITEM_TYPE_TEXT_SLIDER, text_slider_values = sort_by_values, on_value_update = multi_change_sort },
	[2] = { label = "MULTI_LOBBY_GAME_MODE",	type = MENU_ITEM_TYPE_TEXT_SLIDER, text_slider_values = game_mode_values, on_value_update = pc_multi_change_game_type_filter },
	[3] = { label = "MULTI_APPLY_FILTER", 		type = MENU_ITEM_TYPE_SELECTABLE,		on_select = main_menu_multi_apply_filter },

}

Main_menu_server_filter_short = {
	header_label_str = "MAINNEMU_SERVER_FILTER",
	num_items = 3,
	--on_map = main_menu_restore,
	--on_back = main_menu_restore,
	on_show = multi_build_search_filter,
	multi_menu = true,
	multi_internet_menu = true,
	
	[0] = { label = "MULTI_NAME_FILTER",		type = MENU_ITEM_TYPE_INFO_BOX, info_text_str = "", on_select = on_multi_change_name_filter },
	[1] = { label = "MULTI_SORT_BY",			type = MENU_ITEM_TYPE_TEXT_SLIDER, text_slider_values = sort_by_values, on_value_update = multi_change_sort },
	[2] = { label = "MULTI_APPLY_FILTER", 		type = MENU_ITEM_TYPE_SELECTABLE,		on_select = main_menu_multi_apply_filter },

}

--=====================================================
--Button Tips Show
--=====================================================
function main_menu_btn_tips_show(visible)
	--Fades in/out button tips
	local anim_h = vint_object_find("menu_btn_tips_anim", nil, MENU_BASE_DOC_HANDLE)
	local twn_h = vint_object_find("menu_btn_tips_alpha_twn", anim_h)
	local target_h = vint_object_find("menu_btn_tips", nil, MENU_BASE_DOC_HANDLE)
	local initial_alpha = vint_get_property(target_h, "alpha")
	vint_set_property(twn_h, "start_value", initial_alpha)
	local target_alpha = 0
	local delay = 0
	if visible == true then
		target_alpha = 1
		delay = .1
	end
	vint_set_property(twn_h, "end_value", target_alpha)
	lua_play_anim(anim_h, delay)
end

--=====================================================
--Bling
--=====================================================
Mm_bling = {}
Mm_bling.handles = {}

function main_menu_bling_init()
	Mm_bling.handles.bling_bmp = vint_object_find("bling_0")
	Mm_bling.max_bling = 2				--Max number of blings running
	Mm_bling.wait_max = 3				--Maximum wait time for bling to start playing
	Mm_bling.wait_min = 1				--Minimum wait time for bling to start playing
	Mm_bling.active_bling = {}			--Active bling indexes
	Mm_bling.total_blings = 15 		--Total Number of blings (ANIMATIONS MUST EXIST)
	Mm_bling.items = {}
	--Hide initial bling
	vint_set_property(Mm_bling.handles.bling_bmp, "alpha", 0)
	
	local bmp_h, anim_h, twn_h
	for i = 0, Mm_bling.total_blings - 1 do 
		--Clone and retarget bling, setup animations and callbacks
		bmp_h = vint_object_clone(Mm_bling.handles.bling_bmp)
		anim_h = vint_object_find("bling_" .. i .. "_anim")
		twn_h = vint_object_find("bling_0_alpha_twn_2", anim_h)
		vint_set_property(anim_h, "target_handle", bmp_h)
		vint_set_property(anim_h, "is_paused", true)
		vint_set_property(twn_h , "end_event", "main_menu_bling_end")
		Mm_bling.items[i] = {bmp_h = bmp_h, anim_h = anim_h, twn_h = twn_h}
	end
end



function main_menu_play_bling()	
	for i = 0, 1 do 
		main_menu_bling_trigger(i)
	end
end

function main_menu_bling_end(twn_h)
	--callback when a bling is done... 
	
	--Clear bling out of active blings
	for idx, val in Mm_bling.items do
		if Mm_bling.items[idx].twn_h == twn_h then
			--Found match now clear it out of active bling
			Mm_bling.active_bling[idx] = nil
		end
	end

	--Just trigger another bling...
	main_menu_bling_trigger()
end

function main_menu_bling_trigger(offset)
	local offset_time
	if offset == nil then
		offset_time = rand_int(Mm_bling.wait_min, Mm_bling.wait_max)
	else 
		offset_time = offset
	end
	
	local random_bling 
	local counter = 0 
	
	--At least thirty tries before we lose a bling.
	while counter < 30 do
		random_bling = rand_int(0, Mm_bling.total_blings - 1)
		if Mm_bling.active_bling[random_bling] == nil then
			--bling is not active so it is suitible
			--Add to list and play
			Mm_bling.active_bling[random_bling] = true
			lua_play_anim(Mm_bling.items[random_bling].anim_h, offset_time)
			return
		end
		counter = counter + 1
	end	
end

------------------
-- Syslink Server control
------------------
Find_server_control_no_games = true

function find_server_control_show(menu_label, menu_data)
	local control = menu_label.control

	if control ~= nil then
		if control.type ~= MAIN_MENU_FIND_SERVER_CONTROL then
			-- this isn't my control so release it
			menu_release_control(control)
			control = nil
		end
	end

	vint_set_property(menu_label.label_h, "text_tag", "")	
	
	if control == nil then
		local control_h = vint_object_clone(vint_object_find("syslink_server_grp"), Menu_option_labels.control_parent)
		control = { grp_h = control_h, type = MAIN_MENU_FIND_SERVER_CONTROL, is_highlighted = false }
		menu_label.control = control		
	end
	
	--	Create the labels
	menu_label.real_label_h = vint_object_clone(vint_object_find("menu_option_label00", nil, MENU_BASE_DOC_HANDLE), 	menu_label.control.grp_h)
	vint_set_property(menu_label.real_label_h, "visible", false)
	local x, y = vint_get_property(menu_label.real_label_h, "anchor")
	vint_set_property(menu_label.real_label_h, "anchor", x - 9, y - 87)
	---------
	
	---------
	
	--	Show the new control
	vint_set_property(control.grp_h, "visible", true)
	vint_set_property(control.grp_h, "anchor", menu_label.anchor_x, menu_label.anchor_y)
	vint_set_property(control.grp_h, "depth", menu_label.depth)
	
	menu_label.server_list = {num_labels = 0, labels = {}}

	--add 12 invisible labels
	for item_index = 0, FIND_SERVER_MAX_ITEMS - 1 do
		menu_label.server_list.labels[item_index] = vint_object_clone(menu_label.real_label_h)
		vint_set_property(menu_label.server_list.labels[item_index], "visible", false)
		if item_index == 0 then
			local x, y = vint_get_property(menu_label.real_label_h, "anchor")
			vint_set_property(menu_label.server_list.labels[item_index], "anchor", x, y)
		else 
			local x, y = vint_get_property(menu_label.server_list.labels[item_index - 1], "anchor")
			local w, h = element_get_actual_size(menu_label.server_list.labels[item_index - 1])
			vint_set_property(menu_label.server_list.labels[item_index], "anchor", x, y + h)
		end
	end
	
	--put highlight bar in pos 0
	menu_label.hlight_bar_pos = 0
	
	menu_data.has_scroll_bar = false
	find_server_control_update(menu_label, menu_data)

end

function pc_update_find_server()
	find_server_control_update(Menu_option_labels[0], Menu_active[0] )
end


function find_server_control_release(control)
	local menu_label = Menu_option_labels_inactive[0]
	for i = 1, FIND_SERVER_MAX_ITEMS - 1 do
		vint_object_destroy(menu_label.server_list.labels[i])
		vint_object_destroy(menu_label.server_list.labels[i])
	end

	vint_object_destroy(menu_label.real_label_h)
	--vint_set_property(Menu_option_labels.hl_bar, "visible", false)
	pause_menu_control_release(menu_label.control)
	local menu_data = Main_menu_system_link_find_game[0]
	if Main_menu_showing_coop_games == true then
		menu_data = Main_menu_system_link_find_game_coop[0]
	end
	if menu_data.has_scroll_bar == true then
		menu_data.has_scroll_bar = false
		vint_object_destroy(menu_data.scroll_bar.bar_grp)
		menu_data.scroll_bar = nil
	end
end

function find_server_control_resize_select_bar()
	
	local width = FIND_SERVER_SELECT_BAR_WIDTH
	local bg_h
	local frame_h = Menu_option_labels.frame
	local sel_bar_width = width
	
	-- Resize the scroll bar
	if Main_menu_system_link_find_game[0].has_scroll_bar == true then
		sel_bar_width = sel_bar_width - 40
	end
	
	Menu_option_labels.item_width = sel_bar_width
	
	bg_h = vint_object_find("menu_sel_bar_w", frame_h)
	vint_set_property(bg_h, "source_se", sel_bar_width, 36)
	
	bg_h = vint_object_find("menu_sel_bar_w_hl", frame_h)
	vint_set_property(bg_h, "source_se", sel_bar_width, 36)
	
	bg_h = vint_object_find("menu_sel_bar_e", frame_h)
	local menu_bar_e_anchor_x, menu_bar_e_anchor_y = vint_get_property(bg_h, "anchor")
	menu_bar_e_anchor_x = sel_bar_width + 5
	vint_set_property(bg_h, "anchor", menu_bar_e_anchor_x, menu_bar_e_anchor_y)
	
	local bg_h = vint_object_find("menu_sel_bar_e_hl", frame_h)
	vint_set_property(bg_h, "anchor", menu_bar_e_anchor_x - 1, menu_bar_e_anchor_y)
	
	vint_set_property(Menu_option_labels.hl_clip, "offset", 12, -15)
	vint_set_property(Menu_option_labels.hl_clip, "clip_size", width - 17, MENU_ITEM_HEIGHT)

end

function find_server_control_update_highlight(menu_label, menu_data)
	local highlight_pos = menu_label.hlight_bar_pos - menu_label.first_vis_item
	local hbar_x, hbar_y = vint_get_property(menu_label.server_list.labels[highlight_pos], "anchor")
	hbar_y = hbar_y + 86
	hbar_x = hbar_x + 14
	vint_set_property(Menu_option_labels.hl_bar, "anchor", hbar_x, hbar_y)
	local highlight_bar_label_h = vint_object_find("menu_sel_bar_label", Menu_option_labels.hl_bar)
	vint_set_property(highlight_bar_label_h, "text_tag", Main_menu_syslink_server_data[menu_label.hlight_bar_pos].display_string)
	vint_set_property(Menu_option_labels.hl_bar, "visible", true)
end

--SCROLL BAR FUNCTIONS
--create a scroll bar if needed
function find_server_control_create_scroll_bar(menu_label, menu_data)
	if menu_data.has_scroll_bar == true then
		menu_data.has_scroll_bar = false
		vint_object_destroy(menu_data.scroll_bar.bar_grp)
		menu_data.scroll_bar = nil
	end
	
	if menu_label.num_items > FIND_SERVER_MAX_ITEMS then
		local thumb_size = FIND_SERVER_MAX_ITEMS / menu_label.num_items
		menu_data.has_scroll_bar = true;

		local bar_grp = vint_object_clone(vint_object_find("menu_scroll_bar", nil, MENU_BASE_DOC_HANDLE), Menu_option_labels.control_parent)
		
		
		local scroll_data = {
			visible =			false,
			bar_grp =			bar_grp,
			bg_n_h =				vint_object_find("menu_scroll_bg_n", bar_grp),
			bg_c_h =				vint_object_find("menu_scroll_bg_c", bar_grp),
			bg_s_h =				vint_object_find("menu_scroll_bg_s", bar_grp),
			thumb_n_h =			vint_object_find("menu_scroll_thumb_n", bar_grp),
			thumb_s_h =			vint_object_find("menu_scroll_thumb_s", bar_grp),
			thumb_blend_h =	vint_object_find("menu_scroll_thumb_blend", bar_grp),
			thumb_pos =			0,
			bar_height =		332,
			thumb_height =		332,
		}

		menu_data.scroll_bar = scroll_data
		
		find_server_control_show_scroll_bar(menu_data)
		find_server_control_set_bar_height(menu_data, 400)
		find_server_control_set_thumb_size_scroll_bar(menu_data, thumb_size)
		
		--find_server_control_set_pos_scroll_bar(menu_data, menu_label.offset_x + menu_label.select_bar_width - 25, Pause_menu_playlist_editor.header_height + 28)
		vint_set_property(menu_data.scroll_bar.bar_grp, "anchor", 388, 67)--??places the scroll bar, no idea where
	end
end



function find_server_control_update_scroll_bar(menu_label, menu_data)
	local first_vis_item 		= 0
	local new_idx 					= menu_label.hlight_bar_pos
	local num_items = Main_menu_syslink_server_data.num_items
	
	-- Gotta keep it in the middle
	if num_items > FIND_SERVER_MAX_ITEMS then
		local half_max = floor(FIND_SERVER_MAX_ITEMS / 2)
		first_vis_item = limit(new_idx - half_max, 0, num_items - FIND_SERVER_MAX_ITEMS)
	end
	
	--pause_menu_playlist_update_labels(menu_label, menu_data)
	--pause_menu_control_playlist_update_arrow(menu_label, menu_data)
	find_server_control_set_thumb_pos_scroll_bar(menu_data, first_vis_item/(num_items - FIND_SERVER_MAX_ITEMS))--pause_menu_playlist_set_thumb_pos_scroll_bar(active_pane, first_vis_item / active_pane.num_items)

	return first_vis_item
end	
	
	
function find_server_control_show_scroll_bar(menu_data)
	local scroll_data = menu_data.scroll_bar
	vint_set_property(scroll_data.bar_grp, "visible", true)
end

function find_server_control_set_bar_height(menu_data, new_height)
	local scroll_data = menu_data.scroll_bar
	scroll_data.bar_height = new_height - 54 --This magic number will allow you to change the base height of the scrollbar

	vint_set_property(scroll_data.bg_s_h, "anchor", 38, new_height - 10)
	vint_set_property(scroll_data.bg_c_h, "source_se", 10, new_height - 28)

	find_server_control_update_scroll_bar_scroll_bar(menu_data, scroll_data)
end

function find_server_control_set_thumb_size_scroll_bar(menu_data, height)
	local scroll_data = menu_data.scroll_bar
	scroll_data.thumb_height = floor(height * scroll_data.bar_height)
	
	if scroll_data.thumb_height < 60 then
		scroll_data.thumb_height = 60
	end
	
	vint_set_property(scroll_data.thumb_n_h, "source_se", 32, scroll_data.thumb_height - 20)
	find_server_control_update_scroll_bar_scroll_bar(menu_data, scroll_data)
end

function find_server_control_set_thumb_pos_scroll_bar(menu_data, value)
	if menu_data.has_scroll_bar ~= true then
		return
	end
	
	local scroll_data = menu_data.scroll_bar
	if scroll_data.thumb_pos ~= value then
		scroll_data.thumb_pos = value
		find_server_control_update_scroll_bar_scroll_bar(menu_data, scroll_data)
	end
end


function find_server_control_update_scroll_bar_scroll_bar(menu_data, scroll_data)

	if menu_data.has_scroll_bar ~= true then
		return
	end
	
	-- update the thumb pos
	local thumb_offset = scroll_data.thumb_pos * (scroll_data.bar_height - scroll_data.thumb_height) + 10 
	thumb_offset = floor(thumb_offset)
	
	vint_set_property(scroll_data.thumb_n_h,		"anchor", 3, thumb_offset)
	vint_set_property(scroll_data.thumb_s_h,		"anchor", 3, thumb_offset + scroll_data.thumb_height - 20) --Magic Number represents the offset from the thumb height
	vint_set_property(scroll_data.thumb_blend_h,	"anchor", 3, thumb_offset + scroll_data.thumb_height - 63) --Magic Number represents the offset from the thumb height
end
	
	
--OTHER server control functions
function find_server_control_update(menu_label, menu_data)
	--read data from Main_menu_syslink_server_data[table_index] and set text tags in Menu_option_labels[0]
	--up to max, and just ignore the rest
	if menu_label.server_list == nil then
		-- not active yet, ignore
		return
	end

	--[[-----------------------
	-- For debugging
	Main_menu_syslink_server_data = { }
	Main_menu_syslink_server_data.num_items = FIND_SERVER_MAX_ITEMS + 5
	for i = 0, Main_menu_syslink_server_data.num_items - 1 do
		Main_menu_syslink_server_data[i] = { display_string = "GAME " .. i }
	end
	
	-- Comment out the event logic statements in main_menu_syslink_menu_update()
	]]----------------------

	local start_index = 0
	local items_size = Main_menu_syslink_server_data.num_items
	
	menu_label.num_items = Main_menu_syslink_server_data.num_items
	
	if items_size == 0 then
		Find_server_control_no_games = true
		menu_label.num_items = 1
		vint_set_property(menu_label.server_list.labels[0], "text_tag", "MP3_PLAYLIST_EMPTY")
		vint_set_property(menu_label.server_list.labels[0], "visible", true)
		return
	else
		Find_server_control_no_games = false
	end
	
	find_server_control_create_scroll_bar(menu_label, menu_data)
	menu_label.first_vis_item = find_server_control_update_scroll_bar(menu_label, menu_data)
	
	if items_size > FIND_SERVER_MAX_ITEMS then
		items_size = 12
	end
	
	menu_label.display_list_size = items_size
	
	for item_index = 0, FIND_SERVER_MAX_ITEMS - 1 do
		if item_index < items_size then
			--setup visible labels
			local real_index = item_index + menu_label.first_vis_item
			vint_set_property(menu_label.server_list.labels[item_index], "text_tag", Main_menu_syslink_server_data[real_index].display_string)
			vint_set_property(menu_label.server_list.labels[item_index], "visible", true)
		else
			--hide the rest
			vint_set_property(menu_label.server_list.labels[item_index], "visible", false)
		end
	end
	
	find_server_control_update_highlight(menu_label, menu_data)
	find_server_control_resize_select_bar()
end

function find_server_control_nav_up(menu_label, menu_data)
	menu_label.hlight_bar_pos = menu_label.hlight_bar_pos - 1
	if menu_label.hlight_bar_pos < 0 then
		--wrap
		menu_label.hlight_bar_pos = menu_label.num_items - 1
	end
	find_server_control_update(menu_label, menu_data)
end

function find_server_control_nav_down(menu_label, menu_data)
	menu_label.hlight_bar_pos = menu_label.hlight_bar_pos + 1
	if menu_label.hlight_bar_pos > menu_label.num_items - 1 then
		--wrap
		menu_label.hlight_bar_pos = 0
	end
	find_server_control_update(menu_label, menu_data)
end


Main_menu_controls = {
	[MAIN_MENU_FIND_SERVER_CONTROL] = {
		on_show 		=	find_server_control_show,
		--on_select 	=	main_menu_join_syslink_server,--find_server_control_join_server,
		on_nav_up	=	find_server_control_nav_up,
		on_nav_down	=	find_server_control_nav_down,
		on_release	=  find_server_control_release, 
		uses_scroll_bar = false,
		hide_select_bar = false,
	},
}

Main_menu_controls_online = {
	[MAIN_MENU_FIND_SERVER_CONTROL] = {
		on_show 		=	find_server_control_show,
		on_select 	=	main_menu_join_online_server,--find_server_control_join_server,
		on_nav_up	=	find_server_control_nav_up,
		on_nav_down	=	find_server_control_nav_down,
		on_release	=  find_server_control_release, 
		uses_scroll_bar = true,
		hide_select_bar = true,
	},
}
