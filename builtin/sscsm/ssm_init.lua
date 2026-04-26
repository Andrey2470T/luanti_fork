--
-- SSCSM: Server-Sent Client-Side Mods proof-of-concept
--
-- Copyright © 2019-2020 by luk3yx
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.

-- You should have received a copy of the GNU Lesser General Public License
-- along with this program.  If not, see <https://www.gnu.org/licenses/>.
--

-- Prevent loading this SSCSM part from client-side
if INIT == "client" then
	return
end

-- Check for if CSM can send messages to server
local flags = tonumber(core.settings:get("csm_restriction_flags"))
if not flags or flags ~= flags then
	flags = 62
end
flags = math.floor(math.max(flags, 0)) % 64

if math.floor(flags / 2) % 2 == 1 then
	error("[SSCSM] SSCSMs enabled, however CSMs cannot "
		.. "send chat messages! Current set flags: " .. tostring(flags))
end

local sscsm = {}
sscsm.path = core.get_builtin_path() .. "sscsm" .. DIR_DELIM

local minify_code = dofile(sscsm.path .. "minify.lua")

sscsm.loaded_csms = {}

-- Check if 'name' entry (dir or file name) exists in 'dir' directory
local function entry_exists(dir, name, is_dir)
	is_dir = is_dir or false

	local dirlist = core.get_dir_list(dir, is_dir)

	for _, entry in ipairs(dirlist) do
		if name == entry then
			return true
		end
	end

	return false
end

-- Merge the original sscsm code with includes
local function append_includes(modname, code, includes_list)
	local extracted_code = {}
	local modpath = core.get_modpath(modname)

	for _, includename in ipairs(includes_list) do
		local f = io.open(modpath .. DIR_DELIM .. includename)

		if not f then
			error(("Could not open \'%s\' as an include for \'%s\' mod"):format(includename, modname), 2)
		end

		local includecode = f:read("*a")
		f:close()

		table.insert(extracted_code, includecode)
	end

	table.insert(extracted_code, code)

	return table.concat(extracted_code, '\n')
end

-- Load all SSCSMs provided by server mods from 'client' subpath
-- TODO: allow for loading secondary files (not init.lua) and defining depends from other SSCSMs
local function load_sscsms()
	core.log("action", "[SSCSM] Loading all SSCSMs "
		.. "provided by server mods from 'client' subpath...")

	local modspath = core.get_modnames()

	for _, name in ipairs(modspath) do
		local modpath = core.get_modpath(name)

		if entry_exists(modpath, "client", true) then
			local client_subpath = modpath .. DIR_DELIM .. "client"

			if entry_exists(client_subpath, "sscsm.conf") then
				core.log("action", "[SSCSM] open sscsm.conf...")
				local conf = Settings(client_subpath .. DIR_DELIM .. "sscsm.conf")

				if not conf then
					error("Required sscsm.conf in the client subpath of \'"
						.. name .. "\' mod", 2)
				end

				conf = conf:to_table()

				local f = io.open(client_subpath .. DIR_DELIM .. "init.lua")

				if not f then
					error("Could not load init.lua in the client subpath of \'"
						.. name .. "\' mod", 2)
				end

				local code = minify_code(f:read("*a"))

				core.debug(core.serialize(conf))
				if conf.includes then
					code = append_includes(name, code, string.split(conf.includes, ','))
				end
				core.log("action", "[SSCSM] loaded code: " .. code)
				f:close()

				if (#name + #code) > 65300 then
					error("The code + name of \'" .. name .. "\' sscsm is too large"
						.. "(> 65300 symbols limit)", 2)
				end

				sscsm.loaded_csms[name] = code
			end
		end
	end
end

-- Load the client code of each server mod after they are loaded
core.register_on_mods_loaded(load_sscsms)

-- Handle players joining
core.log("action", "[SSCSM] Join mod channel...")
local mod_channel = core.mod_channel_join("sscsm:exec_pipe")
if mod_channel then
	core.log("action", "[SSCSM] Joined mod channel!")
end

core.register_on_modchannel_message(function(channel_name, sender, message)
	if channel_name ~= "sscsm:exec_pipe" or not sender or
			not mod_channel:is_writeable() or message ~= "0" or sender:find("\n") then
		return
	end

	core.log("action", ("[SSCSM] Sending CSMs on request for %s ..."):format(sender))
	for name, code in pairs(sscsm.loaded_csms) do
		mod_channel:send_all(("0%s\n%s\n%s"):format(sender, name, code))
	end
end)
