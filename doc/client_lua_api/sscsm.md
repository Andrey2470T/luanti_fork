SSCSM (Server-Sent Client-Side Mods)
------------------------------------

Starting from 1.3.0 version each server mod can contain some SSCSM code
in `client` subfolder. This code will be sent to clients and executed in
the isolated lua environment. Thus some standard and table lua functions
like `dofile`, `loadfile`, `io.*` wont be accessible from that environment.

The goal of SSCSM is offload the server work by delegating some part of mod code
to running by the client lua scripting. Usually it can be some graphics-related code,
sounds, particlespawners, HUDs, formspecs and etc. Everything that doesn't make sense
to be implemented server-side.

Typical `client` subfolder structure would look like so:

    client
    ├── sscsm.conf
    ├── init.lua
    ├── some_include.lua

Here `init.lua` is the main lua file, `some_include.lua` is secondary file
which would be appended in the beginning of the main file code.
The sscsm configuration necessary for SSCSM identification and listing depends is `sscsm.conf`.

List of allowed globals and functions:
--------------------------

`assert`
`error`
`ipairs`
`next`
`pairs`
`pcall`
`print`
`select`
`tonumber`
`tostring`
`type`
`unpack`
`_VERSION`
`xpcall`

`string.*`
`math.*`
`table.*`
`vector.*`

`io.read`
`io.type`

`os.clock`
`os.date`
`os.difftime`
`os.time`

`core.*`
`minetest.*`

`gfx.*`
