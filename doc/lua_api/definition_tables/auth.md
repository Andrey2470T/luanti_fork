Authentication handler definition
---------------------------------

Used by `core.register_authentication_handler`.

```lua
{
    get_auth = function(name),
    -- Get authentication data for existing player `name` (`nil` if player
    -- doesn't exist).
    -- Returns following structure:
    -- `{password=<string>, privileges=<table>, last_login=<number or nil>}`

    create_auth = function(name, password),
    -- Create new auth data for player `name`.
    -- Note that `password` is not plain-text but an arbitrary
    -- representation decided by the engine.

    delete_auth = function(name),
    -- Delete auth data of player `name`.
    -- Returns boolean indicating success (false if player is nonexistent).

    set_password = function(name, password),
    -- Set password of player `name` to `password`.
    -- Auth data should be created if not present.

    set_privileges = function(name, privileges),
    -- Set privileges of player `name`.
    -- `privileges` is in table form: keys are privilege names, values are `true`;
    -- auth data should be created if not present.

    reload = function(),
    -- Reload authentication data from the storage location.
    -- Returns boolean indicating success.

    record_login = function(name),
    -- Called when player joins, used for keeping track of last_login

    iterate = function(),
    -- Returns an iterator (use with `for` loops) for all player names
    -- currently in the auth database
}
```
