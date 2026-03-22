`ItemStack`
-----------

An `ItemStack` is a stack of items.

It can be created via `ItemStack(x)`, where x is an `ItemStack`,
an itemstring, a table or `nil`.

### Methods

* `is_empty()`: returns `true` if stack is empty.
* `get_name()`: returns item name (e.g. `"default:stone"`).
* `set_name(item_name)`: returns a boolean indicating whether the item was
  cleared.
* `get_count()`: Returns number of items on the stack.
* `set_count(count)`: returns a boolean indicating whether the item was cleared
    * `count`: number, unsigned 16 bit integer
* `get_wear()`: returns tool wear (`0`-`65535`), `0` for non-tools.
* `set_wear(wear)`: returns boolean indicating whether item was cleared
    * `wear`: number, unsigned 16 bit integer
* `get_meta()`: returns ItemStackMetaRef. See section for more details
* `get_metadata()`: **Deprecated.** Returns metadata (a string attached to an item stack).
    * If you need to access this to maintain backwards compatibility,
      use `stack:get_meta():get_string("")` instead.
* `set_metadata(metadata)`: **Deprecated.** Returns true.
    * If you need to set this to maintain backwards compatibility,
      use `stack:get_meta():set_string("", metadata)` instead.
* `get_description()`: returns the description shown in inventory list tooltips.
    * The engine uses this when showing item descriptions in tooltips.
    * Fields for finding the description, in order:
        * `description` in item metadata (See [Item Metadata].)
        * `description` in item definition
        * item name
* `get_short_description()`: returns the short description or nil.
    * Unlike the description, this does not include new lines.
    * Fields for finding the short description, in order:
        * `short_description` in item metadata (See [Item Metadata].)
        * `short_description` in item definition
        * first line of the description (From item meta or def, see `get_description()`.)
        * Returns nil if none of the above are set
* `clear()`: removes all items from the stack, making it empty.
* `replace(item)`: replace the contents of this stack.
    * `item` can also be an itemstring or table.
* `to_string()`: returns the stack in itemstring form.
* `to_table()`: returns the stack in Lua table form.
* `get_stack_max()`: returns the maximum size of the stack (depends on the
  item).
* `get_free_space()`: returns `get_stack_max() - get_count()`.
* `is_known()`: returns `true` if the item name refers to a defined item type.
* `get_definition()`: returns the item definition table.
* `get_tool_capabilities()`: returns the digging properties of the item,
  or those of the hand if none are defined for this item type
* `add_wear(amount)`
    * Increases wear by `amount` if the item is a tool, otherwise does nothing
    * Valid `amount` range is [0,65536]
    * `amount`: number, integer
* `add_wear_by_uses(max_uses)`
    * Increases wear in such a way that, if only this function is called,
      the item breaks after `max_uses` times
    * Valid `max_uses` range is [0,65536]
    * Does nothing if item is not a tool or if `max_uses` is 0
* `get_wear_bar_params()`: returns the wear bar parameters of the item,
  or nil if none are defined for this item type or in the stack's meta
* `add_item(item)`: returns leftover `ItemStack`
    * Put some item or stack onto this stack
* `item_fits(item)`: returns `true` if item or stack can be fully added to
  this one.
* `take_item(n)`: returns taken `ItemStack`
    * Take (and remove) up to `n` items from this stack
    * `n`: number, default: `1`
* `peek_item(n)`: returns taken `ItemStack`
    * Copy (don't remove) up to `n` items from this stack
    * `n`: number, default: `1`
* `equals(other)`:
    * returns `true` if this stack is identical to `other`.
    * Note: `stack1:to_string() == stack2:to_string()` is not reliable,
      as stack metadata can be serialized in arbitrary order.
    * Note: if `other` is an itemstring or table representation of an
      ItemStack, this will always return false, even if it is
      "equivalent".

### Operators

* `stack1 == stack2`:
    * Returns whether `stack1` and `stack2` are identical.
    * Note: `stack1:to_string() == stack2:to_string()` is not reliable,
      as stack metadata can be serialized in arbitrary order.
    * Note: if `stack2` is an itemstring or table representation of an
      ItemStack, this will always return false, even if it is
      "equivalent".
