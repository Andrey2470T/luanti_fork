Naming convention for registered textual names
==============================================

Registered names should generally be in this format:

    "modname:<whatever>" (<whatever> can have characters a-zA-Z0-9_)

This is to prevent conflicting names from corrupting maps and is
enforced by the mod loader.

### Example
In the mod `experimental`, there is the ideal item/node/entity name `tnt`.
So the name should be `experimental:tnt`.

Enforcement can be overridden by prefixing the name with `:`. This can
be used for overriding the registrations of some other mod.

Example: Any mod can redefine `experimental:tnt` by using the name

    :experimental:tnt

when registering it.
(also that mod is required to have `experimental` as a dependency)

The `:` prefix can also be used for maintaining backwards compatibility.

