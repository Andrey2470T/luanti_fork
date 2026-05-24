local COLOR_BLUE = "#7AF"
local COLOR_GREEN = "#7F7"
local COLOR_GRAY = "#BBB"

local LIST_FORMSPEC = [[
		size[13,6.5]
		label[0,-0.1;%s]
		tablecolumns[color;tree;text;text]
		table[0,0.5;12.8,5.5;list;%s;0]
		button_exit[5,6;3,1;quit;%s]
	]]

local LIST_FORMSPEC_DESCRIPTION = [[
		size[13,7.5]
		label[0,-0.1;%s]
		tablecolumns[color;tree;text;text]
		table[0,0.5;12.8,4.8;list;%s;%i]
		box[0,5.5;12.8,1.5;#000]
		textarea[0.3,5.5;13.05,1.9;;;%s]
		button_exit[5,7;3,1;quit;%s]
	]]

local F = core.formspec_escape
local S = core.get_translator("__builtin")
local check_player_privs = core.check_player_privs


-- CHAT COMMANDS FORMSPEC

local mod_cmds = {}

function load_mod_command_tree()
	mod_cmds = {}

	for name, def in pairs(core.registered_chatcommands) do
		mod_cmds[def.mod_origin] = mod_cmds[def.mod_origin] or {}
		local cmds = mod_cmds[def.mod_origin]

		-- Could be simplified, but avoid the priv checks whenever possible
		cmds[#cmds + 1] = { name, def }
	end
	for name, def in pairs(core.server_chatcommands) do
		mod_cmds[def.mod_origin] = mod_cmds[def.mod_origin] or {}
		local cmds = mod_cmds[def.mod_origin]

		-- Could be simplified, but avoid the priv checks whenever possible
		cmds[#cmds + 1] = { name, def }
	end
	local sorted_mod_cmds = {}
	for modname, cmds in pairs(mod_cmds) do
		table.sort(cmds, function(a, b) return a[1] < b[1] end)
		sorted_mod_cmds[#sorted_mod_cmds + 1] = { modname, cmds }
	end
	table.sort(sorted_mod_cmds, function(a, b) return a[1] < b[1] end)
	mod_cmds = sorted_mod_cmds
end

local function build_chatcommands_formspec(name, sel, copy)
	local rows = {}
	rows[1] = "#FFF,0,"..F(S("Command"))..","..F(S("Parameters"))

	local description = S("For more information, click on "
		.. "any entry in the list.").. "\n" ..
		S("Double-click to copy the entry to the chat history.")

	for i, data in ipairs(mod_cmds) do
		rows[#rows + 1] = COLOR_BLUE .. ",0," .. F(data[1]) .. ","
		for j, cmds in ipairs(data[2]) do
			local has_priv = cmds[2].allowed
			rows[#rows + 1] = ("%s,1,%s,%s"):format(
				has_priv and COLOR_GREEN or COLOR_GRAY,
				cmds[1], F(cmds[2].params))
			if sel == #rows then
				description = cmds[2].description
				if copy then
					local msg = S("Command: @1 @2",
						core.colorize("#0FF", "." .. cmds[1]), cmds[2].params)

					core.display_chat_message(msg)
				end
			end
		end
	end

	return LIST_FORMSPEC_DESCRIPTION:format(
			F(S("Available commands: (see also: /help <cmd>)")),
			table.concat(rows, ","), sel or 0,
			F(description), F(S("Close"))
		)
end


-- DETAILED CHAT COMMAND INFORMATION
core.register_on_formspec_input(function(formname, fields)
	if formname ~= "__builtin:help_cmds" or fields.quit then
		return
	end

	local event = core.explode_table_event(fields.list)
	if event.type ~= "INV" then
		core.show_formspec("__builtin:help_cmds",
			build_chatcommands_formspec(nil, event.row, event.type == "DCL"))
	end
end)

function core.show_general_help_formspec()
	core.show_formspec("__builtin:help_cmds",
		build_chatcommands_formspec())
end
