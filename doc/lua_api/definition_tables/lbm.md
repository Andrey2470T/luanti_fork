LBM (LoadingBlockModifier) definition
-------------------------------------

Used by `core.register_lbm`.

A loading block modifier (LBM) is used to define a function that is called for
specific nodes (defined by `nodenames`) when a mapblock which contains such nodes
gets activated (not loaded!).

Note: LBMs operate on a "snapshot" of node positions taken once before they are triggered.
That means if an LBM callback adds a node, it won't be taken into account.
However the engine guarantees that when the callback is called that all given position(s)
contain a matching node.

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
    -- Whether to run the LBM's action every time a block gets activated,
    -- and not only the first time the block gets activated after the LBM
    -- was introduced.

    action = function(pos, node, dtime_s) end,
    -- Function triggered for each qualifying node.
    -- `dtime_s` is the in-game time (in seconds) elapsed since the block
    -- was last active.

    bulk_action = function(pos_list, dtime_s) end,
    -- Function triggered with a list of all applicable node positions at once.
    -- This can be provided as an alternative to `action` (not both).
    -- Available since `core.features.bulk_lbms` (5.10.0)
    -- `dtime_s`: as above
}
```
