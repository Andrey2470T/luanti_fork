local S = core.get_translator("__builtin")

--
-- Privileges
--

core.registered_privileges = {}

function core.register_privilege(name, param)
	local function fill_defaults(def)
		if def.give_to_singleplayer == nil then
			def.give_to_singleplayer = true
		end
		if def.give_to_admin == nil then
			def.give_to_admin = def.give_to_singleplayer
		end
		if def.description == nil then
			def.description = S("(no description)")
		end
	end
	local def
	if type(param) == "table" then
		def = param
	else
		def = {description = param}
	end
	fill_defaults(def)
	core.registered_privileges[name] = def
end

core.register_privilege("interact", S("Can interact with things and modify the world"))
core.register_privilege("shout", S("Can speak in chat"))

local basic_privs =
	core.string_to_privs((core.settings:get("basic_privs") or "shout,interact"))
local basic_privs_desc = S("Can modify basic privileges (@1)",
	core.privs_to_string(basic_privs, ', '))
core.register_privilege("basic_privs", basic_privs_desc)

core.register_privilege("privs", S("Can modify privileges"))

core.register_privilege("teleport", S("Can teleport self"))
core.register_privilege("bring", S("Can teleport other players"))
core.register_privilege("settime", S("Can set the time of day using /time"))
core.register_privilege("server", S("Can do server maintenance stuff"))
core.register_privilege("protection_bypass", S("Can bypass node protection in the world"))
core.register_privilege("ban", S("Can ban and unban players"))
core.register_privilege("kick", S("Can kick players"))
core.register_privilege("give", S("Can use /give and /giveme"))
core.register_privilege("password", S("Can use /setpassword and /clearpassword"))
core.register_privilege("fly", S("Can use fly mode"))
core.register_privilege("fast", S("Can use fast mode"))
core.register_privilege("noclip", S("Can fly through solid nodes using noclip mode"))
core.register_privilege("rollback", S("Can use the rollback functionality"))
core.register_privilege("debug", S("Can enable wireframe"))

core.register_can_bypass_userlimit(function(name, ip)
	local privs = core.get_player_privs(name)
	return privs["server"] or privs["ban"] or privs["privs"] or privs["password"]
end)
