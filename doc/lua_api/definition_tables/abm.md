ABM (ActiveBlockModifier) definition
------------------------------------

Used by `core.register_abm`.

```lua
{
    label = "Lava cooling",
    -- Descriptive label for profiling purposes (optional).
    -- Definitions with identical labels will be listed as one.

    nodenames = {"default:lava_source"},
    -- Apply `action` function to these nodes.
    -- `group:groupname` can also be used here.

    neighbors = {"default:water_source", "default:water_flowing"},
    -- Only apply `action` to nodes that have one of, or any
    -- combination of, these neighbors.
    -- If left out or empty, any neighbor will do.
    -- `group:groupname` can also be used here.

    without_neighbors = {"default:lava_source", "default:lava_flowing"},
    -- Only apply `action` to nodes that have no one of these neighbors.
    -- If left out or empty, it has no effect.
    -- `group:groupname` can also be used here.

    interval = 10.0,
    -- Operation interval in seconds

    chance = 50,
    -- Probability of triggering `action` per-node per-interval is 1.0 / chance (integers only)

    min_y = -32768,
    max_y = 32767,
    -- min and max height levels where ABM will be processed (inclusive)
    -- can be used to reduce CPU usage

    catch_up = true,
    -- If true, catch-up behavior is enabled: The `chance` value is
    -- temporarily reduced when returning to an area to simulate time lost
    -- by the area being unattended. Note that the `chance` value can often
    -- be reduced to 1.

    action = function(pos, node, active_object_count, active_object_count_wider),
    -- Function triggered for each qualifying node.
    -- `active_object_count` is number of active objects in the node's
    -- mapblock.
    -- `active_object_count_wider` is number of active objects in the node's
    -- mapblock plus all 26 neighboring mapblocks. If any neighboring
    -- mapblocks are unloaded an estimate is calculated for them based on
    -- loaded mapblocks.
}
```
