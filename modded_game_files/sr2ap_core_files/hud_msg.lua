-- Has to match the enum in hud_message.h
HUD_REGION_DEBUG		= 0
HUD_REGION_HELP			= 1
HUD_REGION_DIVERSION	= 2
HUD_REGION_SUBTITLES	= 3
HUD_REGION_CUTSCENE_HELP	= 4
HUD_REGION_SR2AP = 5
HUD_NUM_REGIONS			= 5

HUD_MSG_DIR_UP			= 0
HUD_MSG_DIR_DOWN		= 1
HUD_MSG_DIR_CENTER		= 2
HUD_MSG_DIR_UP_RIGHT	= 3

Hud_msg_std_text 		= { scale = 0.6, wrap_width = 0 }
Hud_msg_help_text		= { scale = 0.7, wrap_width = 390 } 	
Hud_msg_subtitle_text	= { scale = 0.7, wrap_width = 0 }	-- Needs to match subtitles.cpp
Hud_msg_sr2ap_text		= { scale = 0.5, wrap_width = 800 }

-- SR2AP notifications (region 5). Times are in seconds.
Sr2ap_msg_hold = 4	-- fully visible
Sr2ap_msg_fade = 1	-- fade out afterwards
Sr2ap_msg_count = 0

Hud_msg_regions = {
	[HUD_REGION_DEBUG]		= {
		text_fmt = Hud_msg_std_text,
		direction = HUD_MSG_DIR_CENTER,	max_msgs = 10,
		anchor = "msg_debug_anchor",
		msgs = { num_msgs = 0 }
	},
	
	[HUD_REGION_HELP]		= {
		text_fmt = Hud_msg_help_text,
		direction = HUD_MSG_DIR_CENTER,
		max_msgs = 3,
		anchor = "msg_help_anchor",
		msgs = { num_msgs = 0 }
	},
	
	[HUD_REGION_DIVERSION]	= {
		text_fmt = Hud_msg_std_text,
		direction = HUD_MSG_DIR_UP,
		max_msgs = 7,
		anchor = "msg_diversion_anchor",
		msgs = { num_msgs = 0 }
	},

	[HUD_REGION_SUBTITLES]	= {
		text_fmt = Hud_msg_subtitle_text,
		direction = HUD_MSG_DIR_CENTER,
		max_msgs = 3,
		anchor = "msg_subtitle_anchor",
		msgs = { num_msgs = 0 }
	},
	
	[HUD_REGION_CUTSCENE_HELP]	= {
		text_fmt = Hud_msg_help_text,
		direction = HUD_MSG_DIR_CENTER,
		max_msgs = 2,
		anchor = "cs_help_anchor",
		msgs = { num_msgs = 0 }
	},
	[HUD_REGION_SR2AP]	= {
		text_fmt = Hud_msg_sr2ap_text,
		direction = HUD_MSG_DIR_UP_RIGHT,
		max_msgs = 4,
		anchor = "msg_multi_chat_anchor",
		msgs = { num_msgs = 0 }
	},
}

-- all messages are stored here with the data item handle as index
Hud_msgs = { }

function hud_msg_init()
	-- resolve the anchors out to handles
	for i, v in Hud_msg_regions do
		v.anchor = vint_object_find(v.anchor)
	end

	vint_set_property(Hud_msg_regions[HUD_REGION_SR2AP].anchor, "anchor", 1260, 700)

	--Resize help region if we are in hd
	if vint_hack_is_std_res() == false then
		--HD mode
		Hud_msg_help_text.wrap_width = 635
	end
	
	vint_datagroup_add_subscription("hud_messages", "insert", "hud_msg_new_message")
	vint_datagroup_add_subscription("hud_messages", "update", "hud_msg_update_message")
	vint_datagroup_add_subscription("hud_messages", "remove", "hud_msg_remove_message")
end

function hud_msg_process()
	sr2ap_msg_expire()


	for i, region in Hud_msg_regions do
		if region.is_dirty == true then
			hud_msg_update_region(region)
			region.is_dirty = false
		end
	end
end

function hud_msg_update_region(region)
	local pos_x, pos_y = 0, 0
	local msgs_displayed = 0
	
	for msg_i = 0, region.msgs.num_msgs - 1 do
		local msg = region.msgs[msg_i]
		local o = msg.text_obj_h
		
		if msgs_displayed >= region.max_msgs then
			if o ~= 0 then
				vint_object_destroy(o)
			end
			msg.text_obj_h = 0
		else

			-- create a text item if needed
			if o == 0 then
				o = vint_object_create("hud_msg", "text", region.anchor)
				vint_set_property(o, "text_tag", msg.text)
				vint_set_property(o, "font", "thin_overlay")
				vint_set_property(o, "text_scale", region.text_fmt.scale, region.text_fmt.scale)
				
				local auto_offset, halign
				if region.direction == HUD_MSG_DIR_CENTER then
					auto_offset = "nw"
					halign = "center"
				elseif region.direction == HUD_MSG_DIR_UP then
					auto_offset = "sw"
					halign = "left"
				elseif region.direction == HUD_MSG_DIR_UP_RIGHT then
					auto_offset = "sw"
					halign = "right"
				else	-- HUD_MSG_DIR_DOWN
					auto_offset = "nw"
					halign = "left"
				end
				
				if region.text_fmt.wrap_width > 0 then
					vint_set_property(o, "word_wrap", true)
					vint_set_property(o, "wrap_width", region.text_fmt.wrap_width)
				end
				
				vint_set_property(o, "auto_offset", auto_offset)
				vint_set_property(o, "horz_align", halign)
				vint_set_property(o, "line_frame_enable", true)
				vint_set_property(o, "line_frame_w", "ui_hud_hlp_bg_w")
				vint_set_property(o, "line_frame_m", "ui_hud_hlp_bg_m")
				vint_set_property(o, "line_frame_e", "ui_hud_hlp_bg_e")
							
				if msg.audio_id ~= -1 then
					audio_play(msg.audio_id, msg.audio_type)
				end
			
				msg.text_obj_h = o

				if msg.region_index == HUD_REGION_SR2AP then
					sr2ap_msg_shown(msg)
				end
			end
			
			-- place the text
			vint_set_property(o, "anchor", pos_x, pos_y)
			
			local w, h = vint_get_property(o, "unscaled_size")
			
			-- figure position of next element
			if region.direction == HUD_MSG_DIR_UP or region.direction == HUD_MSG_DIR_UP_RIGHT then
				pos_y = pos_y - h
			else	-- HUD_MSG_DIR_DOWN or HUD_MSG_DIR_CENTER
				pos_y = pos_y + h
			end
			
		end
		
		msgs_displayed = msgs_displayed + 1
	end
end

function hud_msg_new_message(di_h)
	hud_msg_add(di_h, vint_dataitem_get(di_h))
end

function hud_msg_add(key, region_index, priority, text, fade_time, audio_type, audio_id)
	local region = Hud_msg_regions[region_index]
	
	if region == nil then
		debug_print("vint", "Message placed in invalid region, discarding\n")
		return
	end
	
	-- initialize message
	local msg = {
		di_h = key,
		region_index = region_index,
		priority = priority,
		text = text,
		audio_type = audio_type,
		audio_id = audio_id,
		text_obj_h = 0,
	}
	
	-- insert in region list in priority order
	local	insert_index = region.msgs.num_msgs
	for i = 0, region.msgs.num_msgs - 1 do
		local m = region.msgs[i]
		if m.priority < priority then
			insert_index = i
			break
		end
	end

	-- shift the list down
	for i = region.msgs.num_msgs, insert_index + 1, -1  do
		region.msgs[i] = region.msgs[i - 1]
	end
	
	-- insert new msg
	region.msgs[insert_index] = msg
	region.msgs.num_msgs = region.msgs.num_msgs + 1
	Hud_msgs[key] = msg
	region.is_dirty = true
	
	hud_msg_fade_process(msg, fade_time)
end

function hud_msg_update_message(di_h)
	local region_index, priority, text, fade_time = vint_dataitem_get(di_h)
	local msg = Hud_msgs[di_h]
	
	if msg == nil then
		-- we have no record of this data item
		hud_msg_new_message(di_h)
		return
	end

	local region = Hud_msg_regions[msg.region_index]
	
	-- update the text item if it's changed
	if msg.text ~= text then
		msg.text = text
		if msg.text_obj_h ~= 0 then
			vint_set_property(msg.text_obj_h, "text_tag", text)
		end
	end
	
	hud_msg_fade_process(msg, fade_time)
	
	region.is_dirty = true
end

function hud_msg_remove_message(di_h)
	local msg = Hud_msgs[di_h]
	
	if msg ~= nil then
		-- clean up any existing fade tween
		if msg.fade_tween_h ~= nil then
			vint_object_destroy(msg.fade_tween_h)
		end

		-- destroy text object
		if msg.text_obj_h ~= 0 then
			vint_object_destroy(msg.text_obj_h)
		end
		
		-- find msg in region
		local region = Hud_msg_regions[msg.region_index]
		local msg_index = -1
		for i = 0, region.msgs.num_msgs - 1 do
			if di_h == region.msgs[i].di_h then
				msg_index = i
				break
			end
		end
		
		-- remove it
		if msg_index > -1 then
			for i = msg_index + 1, region.msgs.num_msgs - 1 do
				region.msgs[i - 1] = region.msgs[i]
			end
			
			region.msgs.num_msgs = region.msgs.num_msgs - 1
			region.is_dirty = true
		end
	end
	
	Hud_msgs[di_h] = nil
end

function hud_msg_fade_process(msg, new_fade)
	local is_fading = (msg.fade_time ~= nil and msg.fade_time ~= 0)
	local should_be_fading = (new_fade > 0)
	
	if is_fading ~= should_be_fading then
		-- clean up any existing tween
		if msg.fade_tween_h ~= nil then
			vint_object_destroy(msg.fade_tween_h)
		end
		
		if should_be_fading == true then
			-- start fading
			local o = vint_object_create("msg_fade", "tween", vint_object_find("root_animation"))
			vint_set_property(o, "target_handle", msg.text_obj_h)
			vint_set_property(o, "target_property", "alpha")
			vint_set_property(o, "duration", new_fade)
			vint_set_property(o, "start_value", 1)
			vint_set_property(o, "end_value", 0)
			vint_set_property(o, "start_time", vint_get_time_index())
			msg.fade_tween_h = o
		else
			-- stop fading
			vint_set_property(msg.text_obj_h, "alpha", 1)
		end
	end
	
	msg.fade_time = new_fade
end

function hud_msg_hide_region(region, hide)
	local region = Hud_msg_regions[region]
	
	if region ~= nil then
		vint_set_property(region.anchor, "visible", hide == false)
	end
end


-- Called by the game plugin
function sr2ap_msg_add(text)
	Sr2ap_msg_count = Sr2ap_msg_count + 1
	hud_msg_add("sr2ap_" .. Sr2ap_msg_count, HUD_REGION_SR2AP, 0, text, 0, -1, -1)
end


-- Runs when a message's text object is created, so queued messages get
-- their full time once they're actually on screen.
function sr2ap_msg_shown(msg)
	if msg.expire_time ~= nil then
		return
	end

	audio_play("SYS_OBJECTIVE_COMPLETE")

	local now = vint_get_time_index()
	local o = vint_object_create("msg_fade", "tween", vint_object_find("root_animation"))
	vint_set_property(o, "target_handle", msg.text_obj_h)
	vint_set_property(o, "target_property", "alpha")
	vint_set_property(o, "duration", Sr2ap_msg_fade)
	vint_set_property(o, "start_value", 1)
	vint_set_property(o, "end_value", 0)
	vint_set_property(o, "start_time", now + Sr2ap_msg_hold)
	msg.fade_tween_h = o
	msg.expire_time = now + Sr2ap_msg_hold + Sr2ap_msg_fade
end

-- Removes messages whose fade has finished; the next queued one moves in.
function sr2ap_msg_expire()
	local region = Hud_msg_regions[HUD_REGION_SR2AP]
	local now = vint_get_time_index()
	for i = region.msgs.num_msgs - 1, 0, -1 do
		local msg = region.msgs[i]
		if msg.expire_time ~= nil and now >= msg.expire_time then
			hud_msg_remove_message(msg.di_h)
		end
	end
end