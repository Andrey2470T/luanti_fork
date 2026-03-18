Tool Capabilities
=================

'Tool capabilities' is a property of items that defines two things:

1) Which nodes it can dig and how fast
2) Which objects it can hurt by punching and by how much

Tool capabilities are available for all items, not just tools.
But only tools can receive wear from digging and punching.

Missing or incomplete tool capabilities will default to the
player's hand.

Tool capabilities definition
----------------------------

Tool capabilities define:

* Full punch interval
* Maximum drop level
* For an arbitrary list of node groups:
    * Uses (until the tool breaks)
    * Maximum level (usually `0`, `1`, `2` or `3`)
    * Digging times
* Damage groups
* Punch attack uses (until the tool breaks)

### Full punch interval `full_punch_interval`

When used as a weapon, the item will do full damage if this time is spent
between punches. If e.g. half the time is spent, the item will do half
damage.

### Maximum drop level `max_drop_level`

Suggests the maximum level of node, when dug with the item, that will drop
its useful item. (e.g. iron ore to drop a lump of iron).

This value is not used in the engine; it is the responsibility of the game/mod
code to implement this.

### Uses `uses` (tools only)

Determines how many uses the tool has when it is used for digging a node,
of this group, of the maximum level. The maximum supported number of
uses is 65535. The special number 0 is used for infinite uses.
For lower leveled nodes, the use count is multiplied by `3^leveldiff`.
`leveldiff` is the difference of the tool's `maxlevel` `groupcaps` and the
node's `level` group. The node cannot be dug if `leveldiff` is less than zero.

* `uses=10, leveldiff=0`: actual uses: 10
* `uses=10, leveldiff=1`: actual uses: 30
* `uses=10, leveldiff=2`: actual uses: 90

For non-tools, this has no effect.

### Maximum level `maxlevel`

Tells what is the maximum level of a node of this group that the item will
be able to dig.

### Digging times `times`

List of digging times for different ratings of the group, for nodes of the
maximum level.

For example, as a Lua table, `times={[2]=2.00, [3]=0.70}`. This would
result in the item to be able to dig nodes that have a rating of `2` or `3`
for this group, and unable to dig the rating `1`, which is the toughest.
Unless there is a matching group that enables digging otherwise.

If the result digging time is 0, a delay of 0.15 seconds is added between
digging nodes. If the player releases LMB after digging, this delay is set to 0,
i.e. players can more quickly click the nodes away instead of holding LMB.

This extra delay is not applied in case of a digging time between 0 and 0.15,
so a digging time of 0.01 is actually faster than a digging time of 0.

### Damage groups

List of damage for groups of entities. See [Entity damage mechanism].

### Punch attack uses (tools only)

Determines how many uses (before breaking) the tool has when dealing damage
to an object, when the full punch interval (see above) was always
waited out fully.

Wear received by the tool is proportional to the time spent, scaled by
the full punch interval.

For non-tools, this has no effect.

Example definition of the capabilities of an item
-------------------------------------------------

```lua
tool_capabilities = {
    groupcaps={
        crumbly={maxlevel=2, uses=20, times={[1]=1.60, [2]=1.20, [3]=0.80}}
    },
}
```

This makes the item capable of digging nodes that fulfill both of these:

* Have the `crumbly` group
* Have a `level` group less or equal to `2`

Table of resulting digging times:

    crumbly        0     1     2     3     4  <- level
         ->  0     -     -     -     -     -
             1  0.80  1.60  1.60     -     -
             2  0.60  1.20  1.20     -     -
             3  0.40  0.80  0.80     -     -

    level diff:    2     1     0    -1    -2

Table of resulting tool uses:

    ->  0     -     -     -     -     -
        1   180    60    20     -     -
        2   180    60    20     -     -
        3   180    60    20     -     -

**Notes**:

* At `crumbly==0`, the node is not diggable.
* At `crumbly==3`, the level difference digging time divider kicks in and makes
  easy nodes to be quickly breakable.
* At `level > 2`, the node is not diggable, because it's `level > maxlevel`
