Privileges
==========

Privileges provide a means for server administrators to give certain players
access to special abilities in the engine, games or mods.
For example, game moderators may need to travel instantly to any place in the world,
this ability is implemented in `/teleport` command which requires `teleport` privilege.

Registering privileges
----------------------

A mod can register a custom privilege using `core.register_privilege` function
to give server administrators fine-grained access control over mod functionality.

For consistency and practical reasons, privileges should strictly increase the abilities of the user.
Do not register custom privileges that e.g. restrict the player from certain in-game actions.

Checking privileges
-------------------

A mod can call `core.check_player_privs` to test whether a player has privileges
to perform an operation.
Also, when registering a chat command with `core.register_chatcommand` a mod can
declare privileges that the command requires using the `privs` field of the command
definition.

Managing player privileges
--------------------------

A mod can update player privileges using `core.set_player_privs` function.
Players holding the `privs` privilege can see and manage privileges for all
players on the server.

A mod can subscribe to changes in player privileges using `core.register_on_priv_grant`
and `core.register_on_priv_revoke` functions.

Built-in privileges
-------------------

Luanti includes a set of built-in privileges that control capabilities
provided by the Luanti engine and can be used by mods:

  * Basic privileges are normally granted to all players:
      * `shout`: can communicate using the in-game chat.
      * `interact`: can modify the world by digging, building and interacting
        with the nodes, entities and other players. Players without the `interact`
        privilege can only travel and observe the world.

  * Advanced privileges allow bypassing certain aspects of the gameplay:
      * `fast`: can use "fast mode" to move with maximum speed.
      * `fly`: can use "fly mode" to move freely above the ground without falling.
      * `noclip`: can use "noclip mode" to fly through solid nodes (e.g. walls).
      * `teleport`: can use `/teleport` command to move to any point in the world.
      * `creative`: can access creative inventory.
      * `bring`: can teleport other players to oneself.
      * `give`: can use `/give` and `/giveme` commands to give any item
        in the game to oneself or others.
      * `settime`: can use `/time` command to change current in-game time.
      * `debug`: can enable wireframe rendering mode.

  * Security-related privileges:
      * `privs`: can modify privileges of the players using `/grant[me]` and
        `/revoke[me]` commands.
      * `basic_privs`: can grant and revoke basic privileges as defined by
        the `basic_privs` setting.
      * `kick`: can kick other players from the server using `/kick` command.
      * `ban`: can ban other players using `/ban` command.
      * `password`: can use `/setpassword` and `/clearpassword` commands
        to manage players' passwords.
      * `protection_bypass`: can bypass node protection. Note that the engine does not act upon this privilege,
        it is only an implementation suggestion for games.

  * Administrative privileges:
      * `server`: can use `/fixlight`, `/deleteblocks` and `/deleteobjects`
        commands. Can clear inventory of other players using `/clearinv` command.
      * `rollback`: can use `/rollback_check` and `/rollback` commands.

Related settings
----------------

Luanti includes the following settings to control behavior of privileges:

   * `default_privs`: defines privileges granted to new players.
   * `basic_privs`: defines privileges that can be granted/revoked by players having
    the `basic_privs` privilege. This can be used, for example, to give
    limited moderation powers to selected users.
