Privilege definition
--------------------

Used by `core.register_privilege`.

```lua
{
    description = "",
    -- Privilege description

    give_to_singleplayer = true,
    -- Whether to grant the privilege to singleplayer.

    give_to_admin = true,
    -- Whether to grant the privilege to the server admin.
    -- Uses value of 'give_to_singleplayer' by default.

    on_grant = function(name, granter_name),
    -- Called when given to player 'name' by 'granter_name'.
    -- 'granter_name' will be nil if the priv was granted by a mod.

    on_revoke = function(name, revoker_name),
    -- Called when taken from player 'name' by 'revoker_name'.
    -- 'revoker_name' will be nil if the priv was revoked by a mod.

    -- Note that the above two callbacks will be called twice if a player is
    -- responsible, once with the player name, and then with a nil player
    -- name.
    -- Return true in the above callbacks to stop register_on_priv_grant or
    -- revoke being called.
}
```
