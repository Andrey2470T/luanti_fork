Bans
====

* `core.get_ban_list()`: returns a list of all bans formatted as string
* `core.get_ban_description(ip_or_name)`: returns list of bans matching
  IP address or name formatted as string
* `core.ban_player(name)`: ban the IP of a currently connected player
    * Returns boolean indicating success
* `core.unban_player_or_ip(ip_or_name)`: remove ban record matching
  IP address or name
* `core.kick_player(name[, reason[, reconnect]])`: disconnect a player with an optional
  reason.
    * Returns boolean indicating success (false if player nonexistent)
    * If `reconnect` is true, allow the user to reconnect.
* `core.disconnect_player(name[, reason[, reconnect]])`: disconnect a player with an
  optional reason, this will not prefix with 'Kicked: ' like kick_player.
  If no reason is given, it will default to 'Disconnected.'
    * Returns boolean indicating success (false if player nonexistent)
