`ModChannel`
------------

An interface to use mod channels on client and server

### Methods

* `leave()`: leave the mod channel.
    * Server leaves channel `channel_name`.
    * No more incoming or outgoing messages can be sent to this channel from
      server mods.
    * This invalidate all future object usage.
    * Ensure you set mod_channel to nil after that to free Lua resources.
* `is_writeable()`: returns true if channel is writeable and mod can send over
  it.
* `send_all(message)`: Send `message` though the mod channel.
    * If mod channel is not writeable or invalid, message will be dropped.
    * Message size is limited to 65535 characters by protocol.
