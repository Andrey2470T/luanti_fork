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

-- Whitelisted globals and functions for the SSCSM sandbox
-- According to http://lua-users.org/wiki/SandBoxes
---------------------------------------------------

local allowed = {}

-- standard
allowed.assert = assert
allowed.error = error
allowed.ipairs = ipairs
allowed.next = next
allowed.pairs = pairs
allowed.pcall = pcall
allowed.print = print
allowed.select = select
allowed.tonumber = tonumber
allowed.tostring = tostring
allowed.type = type
allowed.unpack = unpack
allowed._VERSION = _VERSION
allowed.xpcall = xpcall
-- string
allowed.string = table.copy_with_metatables(string)
-- math
allowed.math = table.copy_with_metatables(math)
-- table
allowed.table = table.copy_with_metatables(table)
-- vector
allowed.vector = table.copy_with_metatables(vector)
-- io
--allowed.io = {}
--allowed.io.read = io.read
--allowed.io.type = io.type
-- os
allowed.os = {}
allowed.os.clock = os.clock
allowed.os.date = os.date
allowed.os.difftime = os.difftime
allowed.os.time = os.time
-- core
allowed.core = core
allowed.minetest = allowed.core

if rawget(_G, "gfx") then
	allowed.gfx = gfx
end


-- Basic functions
------------------

-- Load and execute the code in the protected mode
function exec(code, file)
	if code:byte(1) == 27 then return nil, "Invalid code!" end

	local f, msg = loadstring(code, ("=%q"):format(file))
	if not f then
		core.log("error", "[SSCSM] Syntax error: " .. tostring(msg))
		return false
	end

	setfenv(f, allowed)

	local ok, res = pcall(f)
	if ok then
		return true
	else
		core.log("error", "[SSCSM] Runtime Error" .. tostring(res))
		return false
	end
end

local mod_channel
function join_mod_channel()
	if mod_channel then
		mod_channel:leave()
	end

	mod_channel = core.mod_channel_join("sscsm:exec_pipe")
end

function leave_mod_channel()
	if mod_channel then
		mod_channel:leave()
		mod_channel = false
	end
end

-- Creating modchannel callbacks
-------------------------------------------------------

--local env = Env:new()

local received_sscsms = {}

-- exec() code sent by the server.
core.register_on_modchannel_message(function(channel_name, sender, message)
	if channel_name ~= "sscsm:exec_pipe" or (sender and sender ~= "") then
		return
	end

	-- The first character is currently a version code, currently 0.
	-- Do not change unless absolutely necessary.
	local version = message:sub(1, 1)
	local name, code
	if version == "0" then
		local s, e = message:find("\n")
		if not s or not e then return end
		local target = message:sub(2, s - 1)
		if target ~= core.localplayer:get_name() then return end
		message = message:sub(e + 1)
		s, e = message:find("\n")
		if not s or not e then return end
		name = message:sub(1, s - 1)
		code = message:sub(e + 1)
	else
		return
	end

	-- Don't load the same SSCSM twice
	if not received_sscsms[name] then
		core.log("action", "[SSCSM] Loading " .. name)
		received_sscsms[name] = true
		exec(code, name)
	end
end)

-- Send "0" when the "sscsm:exec_pipe" channel is first joined.
local sent_request = false
local was_repeat_try = false

local function attempt_to_join_mod_channel()
	-- Wait for core.localplayer to become available.
	if not core.localplayer then
		core.after(0.05, attempt_to_join_mod_channel)
		return
	end

	-- Join the mod channel
	join_mod_channel()
end

core.register_on_modchannel_signal(function(channel_name, signal)
	if sent_request or channel_name ~= "sscsm:exec_pipe" then
		return
	end

	if signal == 0 then
		core.log("action", "[SSCSM] Retrieving SSCSMs code...")
		mod_channel:send_all("0")
		sent_request = true
	elseif signal == 1 then
		if not was_repeat_try then
			was_repeat_try = true
			attempt_to_join_mod_channel()
		else
			leave_mod_channel()
		end
	end
end)

core.after(0, attempt_to_join_mod_channel)
