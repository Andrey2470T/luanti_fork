LBM (LoadingBlockModifier) definition
-------------------------------------

Used by `core.register_lbm`.

A loading block modifier (LBM) is used to define a function that is called for
specific nodes (defined by `nodenames`) when a mapblock which contains such nodes
gets **activated** (**not loaded!**).

*Note*: LBMs operate on a "snapshot" of node positions taken once before they are triggered.
That means if an LBM callback adds a node, it won't be taken into account.
However the engine guarantees that at the point in time when the callback is called
that all given positions contain a matching node.

For `run_at_every_load = false` to work, both mapblocks and LBMs have timestamps
associated with them:

* Each mapblock has a "last active" timestamp. It is also updated when the
  mapblock is generated.
* For each LBM, an introduction timestamp is stored in the world data, identified
  by the LBM's `name` field. If an LBM disappears, the corresponding timestamp
  is cleared.

When a mapblock is activated, only LBMs whose introduction timestamp is newer
than the mapblock's timestamp are run.

*Note*: For maps generated in 5.11.0 or older, many newly generated mapblocks
did not get a timestamp set. This means LBMs introduced between generation time
and time of first activation will never run.
Currently the only workaround is to use `run_at_every_load = true`.

```lua
{
    label = "Upgrade legacy doors",
    -- Descriptive label for profiling purposes (optional).
    -- Definitions with identical labels will be listed as one.

    name = "modname:replace_legacy_door",
    -- Identifier of the LBM, should follow the modname:<whatever> convention

    nodenames = {"default:lava_source"},
    -- List of node names to trigger the LBM on.
    -- Names of non-registered nodes and groups (as group:groupname)
    -- will work as well.

    run_at_every_load = false,
    -- If `false`: The LBM only runs on mapblocks the first time they are
    -- activated after the LBM was introduced.
    -- It never runs on mapblocks generated after the LBM's introduction.
    -- See above for details.
    --
    -- If `true`: The LBM runs every time a mapblock is activated.

    action = function(pos, node, dtime_s) end,
    -- Function triggered for each qualifying node.
    -- `dtime_s` is the in-game time (in seconds) elapsed since the mapblock
    -- was last active (available since 5.7.0).

    bulk_action = function(pos_list, dtime_s) end,
    -- Function triggered with a list of all applicable node positions at once.
    -- This can be provided as an alternative to `action` (not both).
    -- Available since `core.features.bulk_lbms` (5.10.0)
    -- `dtime_s`: as above
}
```
