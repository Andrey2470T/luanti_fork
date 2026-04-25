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

local sscsmpath = core.get_builtin_path() .. "sscsm" .. DIR_DELIM

local sscsm = {}

sscsm.minify_code = dofile(sscsmpath .. "minify.lua")

--_G[modname] = sscsm

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

-- Load all SSCSMs provided by server mods from 'client' subpath
-- TODO: allow for loading secondary files (not init.lua) and defining depends from other SSCSMs
local function load_sscsms()
	core.log("action", "[SSCSM] Loading all SSCSMs "
		.. "provided by server mods from 'client' subpath...")

	local modspath = core.get_modnames()

	for _, name in ipairs(modspath) do
		local modpath = core.get_modpath(name)

		local client_subpath = modpath .. DIR_DELIM .. "client"
		if entry_exists(modpath, "client", true) then
			local client_subpath = modpath .. DIR_DELIM .. "client"

			if entry_exists(client_subpath, "init.lua") then
				core.log("action", "[SSCSM] open init.lua...")
				local f = io.open(client_subpath .. DIR_DELIM .. "init.lua")

				if not f then
					error("Could not load init.lua in the client subpath of \'"
						.. name .. "\' mod", 2)
				end

				local code = sscsm.minify_code(f:read("*a"))
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
