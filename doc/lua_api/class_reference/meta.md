`MetaDataRef`
-------------

Base class used by [`StorageRef`], [`NodeMetaRef`], [`ItemStackMetaRef`],
and [`PlayerMetaRef`].

Note: If a metadata value is in the format `${k}`, an attempt to get the value
will return the value associated with key `k`. There is a low recursion limit.
This behavior is **deprecated** and will be removed in a future version. Usage
of the `${k}` syntax in formspecs is not deprecated.

### Methods

* `contains(key)`: Returns true if key present, otherwise false.
    * Returns `nil` when the MetaData is inexistent.
* `get(key)`: Returns `nil` if key not present, else the stored string.
* `set_string(key, value)`: Value of `""` will delete the key.
* `get_string(key)`: Returns `""` if key not present.
* `set_int(key, value)`
    * The range for the value is system-dependent (usually 32 bits).
      The value will be converted into a string when stored.
* `get_int(key)`: Returns `0` if key not present.
* `set_float(key, value)`
    * The range for the value is system-dependent (usually 32 bits).
      The value will be converted into a string when stored.
* `get_float(key)`: Returns `0` if key not present.
* `get_keys()`: returns a list of all keys in the metadata.
* `to_table()`:
    * Returns a metadata table (see below) or `nil` on failure.
* `from_table(data)`
    * Imports metadata from a metadata table
    * If `data` is a metadata table (see below), the metadata it represents
      will replace all metadata of this MetaDataRef object
    * Any non-table value for `data` will clear all metadata
    * Item table values the `inventory` field may also be itemstrings
    * Returns `true` on success
* `equals(other)`
    * returns `true` if this metadata has the same key-value pairs as `other`

### Metadata tables

Metadata tables represent MetaDataRef in a Lua table form (see `from_table`/`to_table`).

A metadata table is a table that has the following keys:

* `fields`: key-value storage of metadata fields
    * all values are stored as strings
    * numbers must be converted to strings first
* `inventory` (for NodeMetaRef only): A node inventory in table form
    * inventory table keys are inventory list names
    * inventory table values are item tables
    * item table keys are slot IDs (starting with 1)
    * item table values are ItemStacks

Example:

```lua
metadata_table = {
    -- metadata fields (key/value store)
    fields = {
        infotext = "Container",
        another_key = "Another Value",
    },

    -- inventory data (for nodes)
    inventory = {
        -- inventory list "main" with 4 slots
        main = {
            -- list of all item slots
            [1] = "example:dirt",
            [2] = "example:stone 25",
            [3] = "", -- empty slot
            [4] = "example:pickaxe",
        },
        -- inventory list "hidden" with 1 slot
        hidden = {
            [1] = "example:diamond",
        },
    },
}
```

`ItemStackMetaRef`
------------------

ItemStack metadata: reference extra data and functionality stored in a stack.
Can be obtained via `item:get_meta()`.

### Methods

* All methods in MetaDataRef
* `set_tool_capabilities([tool_capabilities])`
    * Overrides the item's tool capabilities
    * A nil value will clear the override data and restore the original
      behavior.
* `set_wear_bar_params([wear_bar_params])`
    * Overrides the item's wear bar parameters (see "Wear Bar Color" section)
    * A nil value will clear the override data and restore the original
      behavior.

`NodeMetaRef`
-------------

Node metadata: reference extra data and functionality stored in a node.
Can be obtained via `core.get_meta(pos)`.

### Methods

* All methods in MetaDataRef
* `get_inventory()`: returns `InvRef`
* `mark_as_private(name or {name1, name2, ...})`: Mark specific vars as private
  This will prevent them from being sent to the client. Note that the "private"
  status will only be remembered if an associated key-value pair exists,
  meaning it's best to call this when initializing all other meta (e.g.
  `on_construct`).

`PlayerMetaRef`
---------------

Player metadata.
Uses the same method of storage as the deprecated player attribute API, so
data there will also be in player meta.
Can be obtained using `player:get_meta()`.

### Methods

* All methods in MetaDataRef

`StorageRef`
------------

Mod metadata: per mod metadata, saved automatically.
Can be obtained via `core.get_mod_storage()` during load time.

WARNING: This storage backend is incapable of saving raw binary data due
to restrictions of JSON.

### Methods

* All methods in MetaDataRef
