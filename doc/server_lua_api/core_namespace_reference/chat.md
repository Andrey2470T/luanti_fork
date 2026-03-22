Chat
====

* `core.chat_send_all(text)`: send chat message to all players
* `core.chat_send_player(name, text)`: send chat message to specific player
    * `name`: Name of the player
* `core.format_chat_message(name, message)`
    * Used by the server to format a chat message, based on the setting `chat_message_format`.
      Refer to the documentation of the setting for a list of valid placeholders.
    * Takes player name and message, and returns the formatted string to be sent to players.
    * Can be redefined by mods if required, for things like colored names or messages.
    * **Only** the first occurrence of each placeholder will be replaced.
