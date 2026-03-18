Chat command definition
-----------------------

Used by `core.register_chatcommand`.

Specifies the function to be called and the privileges required when a player
issues the command.  A help message that is the concatenation of the params and
description fields is shown when the "/help" chatcommand is issued.

```lua
{
    params = "",
    -- Short parameter description.  See the below note.

    description = "",
    -- General description of the command's purpose.

    privs = {},
    -- Required privileges to run. See `core.check_player_privs()` for
    -- the format and see [Privileges] for an overview of privileges.

    func = function(name, param),
    -- Called when command is run.
    -- * `name` is the name of the player who issued the command.
    -- * `param` is a string with the full arguments to the command.
    -- Returns a boolean for success and a string value.
    -- The string is shown to the issuing player upon exit of `func` or,
    -- if `func` returns `false` and no string, the help message is shown.
}
```

Note that in params, the conventional use of symbols is as follows:

* `<>` signifies a placeholder to be replaced when the command is used. For
  example, when a player name is needed: `<name>`
* `[]` signifies param is optional and not required when the command is used.
  For example, if you require param1 but param2 is optional:
  `<param1> [<param2>]`
* `|` signifies exclusive or. The command requires one param from the options
  provided. For example: `<param1> | <param2>`
* `()` signifies grouping. For example, when param1 and param2 are both
  required, or only param3 is required: `(<param1> <param2>) | <param3>`

Example:

```lua
{
    params = "<name> <privilege>",

    description = "Remove privilege from player",

    privs = {privs=true},  -- Require the "privs" privilege to run

    func = function(name, param),
}
```
