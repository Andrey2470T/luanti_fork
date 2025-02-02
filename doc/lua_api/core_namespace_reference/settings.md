Setting-related
===============

* `core.settings`: Settings object containing all of the settings from the
  main config file (`minetest.conf`). See [`Settings`].
* `core.setting_get_pos(name)`: Loads a setting from the main settings and
  parses it as a position (in the format `(1,2,3)`). Returns a position or nil. **Deprecated: use `core.settings:get_pos()` instead**
