-- For server-side translations (if INIT == "game")
-- Otherwise, use core.gettext
local S = core.get_translator("__builtin")

core.registered_chatcommands = {}

if INIT == "client" then
	core.server_chatcommands = {}
end

-- Interpret the parameters of a command, separating options and arguments.
-- Input: command, param
--   command: name of command
--   param: parameters of command
-- Returns: opts, args
--   opts is a string of option letters, or false on error
--   args is an array with the non-option arguments in order, or an error message
-- Example: for this command line:
--      /command a b -cd e f -g
-- the function would receive:
--      a b -cd e f -g
-- and it would return:
--	"cdg", {"a", "b", "e", "f"}
-- Negative numbers are taken as arguments. Long options (--option) are
-- currently rejected as reserved.
function getopts(command, param)
	local opts = ""
	local args = {}
	for match in param:gmatch("%S+") do
		if match:byte(1) == 45 then -- 45 = '-'
			local second = match:byte(2)
			if second == 45 then
				return false, S("Invalid parameters (see /help @1).", command)
			elseif second and (second < 48 or second > 57) then -- 48 = '0', 57 = '9'
				opts = opts .. match:sub(2)
			else
				-- numeric, add it to args
				args[#args + 1] = match
			end
		else
			args[#args + 1] = match
		end
	end
	return opts, args
end

function core.register_chatcommand(cmd, def)
	def = def or {}
	def.params = def.params or ""
	def.description = def.description or ""
	def.privs = def.privs or {}
	def.mod_origin = core.get_current_modname() or "??"
	core.registered_chatcommands[cmd] = def
end

function core.unregister_chatcommand(name)
	if core.registered_chatcommands[name] then
		core.registered_chatcommands[name] = nil
	else
		core.log("warning", "Not unregistering chatcommand " ..name..
			" because it doesn't exist.")
	end
end

function core.override_chatcommand(name, redefinition)
	local chatcommand = core.registered_chatcommands[name]
	assert(chatcommand, "Attempt to override non-existent chatcommand "..name)
	for k, v in pairs(redefinition) do
		rawset(chatcommand, k, v)
	end
	core.registered_chatcommands[name] = chatcommand
end

-- The mechanism to send the registered chat commands by the server scripting
-- to the client one
local mod_channel = core.mod_channel_join("send_cmds_to_client")

if INIT == "client" then
	core.register_on_modchannel_signal(function(channel_name, signal)
		if channel_name ~= "send_cmds_to_client" then
			return
		end

		if signal == 0 then
			mod_channel:send_all("0")
		elseif signal == 1 then
			mod_channel:leave()
		end
	end)

	core.register_on_modchannel_message(function(channel_name, sender, message)
		if channel_name ~= "send_cmds_to_client" or not sender or message == "" then
			return
		end

		for name, def in pairs(core.deserialize(message)) do
			core.server_chatcommands[name] = def
		end
		load_mod_command_tree()
	end)
else
	core.register_on_modchannel_message(function(channel_name, sender, message)
		if channel_name ~= "send_cmds_to_client" or not sender or
				not mod_channel:is_writeable() or message ~= "0" or sender:find("\n") then
			return
		end

		local player_cmds = {}

		for cmd_name, cmd_def in pairs(core.registered_chatcommands) do
			player_cmds[cmd_name] = table.copy(cmd_def)
			player_cmds[cmd_name].allowed = core.check_player_privs(sender, cmd_def.privs)
		end
		mod_channel:send_all(core.serialize(player_cmds))
	end)
end
