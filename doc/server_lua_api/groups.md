Groups
======

In a number of places, there is a group table. Groups define the
properties of a thing (item, node, armor of entity, tool capabilities)
in such a way that the engine and other mods can can interact with
the thing without actually knowing what the thing is.

Usage
-----

Groups are stored in a table, having the group names with keys and the
group ratings as values. Group ratings are integer values within the
range [-32767, 32767]. For example:

```lua
-- Default dirt
groups = {crumbly=3, soil=1}

-- A more special dirt-kind of thing
groups = {crumbly=2, soil=1, level=2, outerspace=1}
```

Groups always have a rating associated with them. If there is no
useful meaning for a rating for an enabled group, it shall be `1`.

When not defined, the rating of a group defaults to `0`. Thus when you
read groups, you must interpret `nil` and `0` as the same value, `0`.

You can read the rating of a group for an item or a node by using

```lua
core.get_item_group(itemname, groupname)
```

Groups of items
---------------

Groups of items can define what kind of an item it is (e.g. wool).

Groups of nodes
---------------

In addition to the general item things, groups are used to define whether
a node is destroyable and how long it takes to destroy by a tool.

Groups of entities
------------------

For entities, groups are, as of now, used only for calculating damage.
The rating is the percentage of damage caused by items with this damage group.
See [Entity damage mechanism].

```lua
object:get_armor_groups() --> a group-rating table (e.g. {fleshy=100})
object:set_armor_groups({fleshy=30, cracky=80})
```

Groups of tool capabilities
---------------------------

Groups in tool capabilities define which groups of nodes and entities they
are effective towards.

Groups in crafting recipes
--------------------------

In crafting recipes, you can specify a group as an input item.
This means that any item in that group will be accepted as input.

The basic syntax is:

```lua
"group:<group_name>"
```

For example, `"group:meat"` will accept any item in the `meat` group.

It is also possible to require an input item to be in
multiple groups at once. The syntax for that is:

```lua
"group:<group_name_1>,<group_name_2>,(...),<group_name_n>"
```

For example, `"group:leaves,birch,trimmed"` accepts any item which is member
of *all* the groups `leaves` *and* `birch` *and* `trimmed`.

An example recipe: Craft a raw meat soup from any meat, any water and any bowl:

```lua
{
    output = "food:meat_soup_raw",
    recipe = {
        {"group:meat"},
        {"group:water"},
        {"group:bowl"},
    },
}
```

Another example: Craft red wool from white wool and red dye
(here, "red dye" is defined as any item which is member of
*both* the groups `dye` and `basecolor_red`).

```lua
{
    type = "shapeless",
    output = "wool:red",
    recipe = {"wool:white", "group:dye,basecolor_red"},
}
```

Special groups
--------------

The asterisk `(*)` after a group name describes that there is no engine
functionality bound to it, and implementation is left up as a suggestion
to games.

### Node and item groups

* `not_in_creative_inventory`: (*) Special group for inventory mods to indicate
  that the item should be hidden in item lists.


### Node-only groups

* `attached_node`: the node is 'attached' to a neighboring node. It checks
                   whether the node it is attached to is walkable. If it
                   isn't, the node will drop as an item.
    * `1`: if the node is wallmounted, the node is attached in the wallmounted
           direction. Otherwise, the node is attached to the node below.
    * `2`: if the node is facedir or 4dir, the facedir or 4dir direction is checked.
           No effect for other nodes.
           Note: The "attaching face" of this node is tile no. 5 (back face).
    * `3`: the node is always attached to the node below.
    * `4`: the node is always attached to the node above.
* `bouncy`: value is bounce speed in percent.
  If positive, jump/sneak on floor impact will increase/decrease bounce height.
  Negative value is the same bounciness, but non-controllable.
* `connect_to_raillike`: makes nodes of raillike drawtype with same group value
  connect to each other
* `dig_immediate`: Player can always pick up node without reducing tool wear
    * `2`: the node always gets the digging time 0.5 seconds (rail, sign)
    * `3`: the node always gets the digging time 0 seconds (torch)
* `disable_jump`: Player (and possibly other things) cannot jump from node
  or if their feet are in the node. Note: not supported for `new_move = false`
* `disable_descend`: Player (and possibly other things) cannot *actively*
  descend in node using Sneak or Aux1 key (for liquids and climbable nodes
  only). Note: not supported for `new_move = false`
* `fall_damage_add_percent`: modifies the fall damage suffered when hitting
  the top of this node. There's also an armor group with the same name.
  The final player damage is determined by the following formula:
    ```lua
    damage =
      collision speed
      * ((node_fall_damage_add_percent   + 100) / 100) -- node group
      * ((player_fall_damage_add_percent + 100) / 100) -- player armor group
      - (14)                                           -- constant tolerance
    ```
  Negative damage values are discarded as no damage.
* `falling_node`: if there is no walkable block under the node it will fall
* `float`: the node will not fall through liquids (`liquidtype ~= "none"`)
     * A liquid source with `groups = {falling_node = 1, float = 1}`
       will fall through flowing liquids.
* `level`: Can be used to give an additional sense of progression in the game.
     * A larger level will cause e.g. a weapon of a lower level make much less
       damage, and get worn out much faster, or not be able to get drops
       from destroyed nodes.
     * `0` is something that is directly accessible at the start of gameplay
     * There is no upper limit
     * See also: `leveldiff` in [Tool Capabilities]
* `slippery`: Players and items will slide on the node.
  Slipperiness rises steadily with `slippery` value, starting at 1.


### Tool-only groups

* `disable_repair`: If set to 1 for a tool, it cannot be repaired using the
  `"toolrepair"` crafting recipe


### `ObjectRef` armor groups

* `immortal`: Skips all damage and breath handling for an object. This group
  will also hide the integrated HUD status bars for players. It is
  automatically set to all players when damage is disabled on the server and
  cannot be reset (subject to change).
* `fall_damage_add_percent`: Modifies the fall damage suffered by players
  when they hit the ground. It is analog to the node group with the same
  name. See the node group above for the exact calculation.
* `punch_operable`: For entities; disables the regular damage mechanism for
  players punching it by hand or a non-tool item, so that it can do something
  else than take damage.



Known damage and digging time defining groups
---------------------------------------------

* `crumbly`: dirt, sand
* `cracky`: tough but crackable stuff like stone.
* `snappy`: something that can be cut using things like scissors, shears,
  bolt cutters and the like, e.g. leaves, small plants, wire, sheets of metal
* `choppy`: something that can be cut using force; e.g. trees, wooden planks
* `fleshy`: Living things like animals and the player. This could imply
  some blood effects when hitting.
* `explody`: Especially prone to explosions
* `oddly_breakable_by_hand`:
   Can be added to nodes that shouldn't logically be breakable by the
   hand but are. Somewhat similar to `dig_immediate`, but times are more
   like `{[1]=3.50,[2]=2.00,[3]=0.70}` and this does not override the
   digging speed of an item if it can dig at a faster speed than this
   suggests for the hand.

Examples of custom groups
-------------------------

Item groups are often used for defining, well, _groups of items_.

* `meat`: any meat-kind of a thing (rating might define the size or healing
  ability or be irrelevant -- it is not defined as of yet)
* `eatable`: anything that can be eaten. Rating might define HP gain in half
  hearts.
* `flammable`: can be set on fire. Rating might define the intensity of the
  fire, affecting e.g. the speed of the spreading of an open fire.
* `wool`: any wool (any origin, any color)
* `metal`: any metal
* `weapon`: any weapon
* `heavy`: anything considerably heavy

Digging time calculation specifics
----------------------------------

Groups such as `crumbly`, `cracky` and `snappy` are used for this
purpose. Rating is `1`, `2` or `3`. A higher rating for such a group implies
faster digging time.

The `level` group is used to limit the toughness of nodes an item capable
of digging can dig and to scale the digging times / damage to a greater extent.

**Please do understand this**, otherwise you cannot use the system to it's
full potential.

Items define their properties by a list of parameters for groups. They
cannot dig other groups; thus it is important to use a standard bunch of
groups to enable interaction with items.
