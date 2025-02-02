Mod channels
============

You can find mod channels communication scheme in `doc/mod_channels.png`.

* `core.mod_channel_join(channel_name)`
    * Server joins channel `channel_name`, and creates it if necessary. You
      should listen for incoming messages with
      `core.register_on_modchannel_message`
