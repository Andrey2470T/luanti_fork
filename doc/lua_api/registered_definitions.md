Registered definitions
======================

Anything added using certain [Registration functions] gets added to one or more
of the global [Registered definition tables].

Note that in some cases you will stumble upon things that are not contained
in these tables (e.g. when a mod has been removed). Always check for
existence before trying to access the fields.

Example:

All nodes registered with `core.register_node` get added to the table
`core.registered_nodes`.

If you want to check the drawtype of a node, you could do it like this:

```lua
local def = core.registered_nodes[nodename]
local drawtype = def and def.drawtype
```
