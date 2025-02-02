L-system trees
==============

Tree definition
---------------

```lua
treedef={
    axiom,         --string  initial tree axiom
    rules_a,       --string  rules set A
    rules_b,       --string  rules set B
    rules_c,       --string  rules set C
    rules_d,       --string  rules set D
    trunk,         --string  trunk node name
    leaves,        --string  leaves node name
    leaves2,       --string  secondary leaves node name
    leaves2_chance,--num     chance (0-100) to replace leaves with leaves2
    angle,         --num     angle in deg
    iterations,    --num     max # of iterations, usually 2 -5
    random_level,  --num     factor to lower number of iterations, usually 0 - 3
    trunk_type,    --string  single/double/crossed) type of trunk: 1 node,
                    --        2x2 nodes or 3x3 in cross shape
    thin_branches, --boolean true -> use thin (1 node) branches
    fruit,         --string  fruit node name
    fruit_chance,  --num     chance (0-100) to replace leaves with fruit node
    seed,          --num     random seed, if no seed is provided, the engine will create one.
}
```

Key for special L-System symbols used in axioms
-----------------------------------------------

* `G`: move forward one unit with the pen up
* `F`: move forward one unit with the pen down drawing trunks and branches
* `f`: move forward one unit with the pen down drawing leaves (100% chance)
* `T`: move forward one unit with the pen down drawing trunks only
* `R`: move forward one unit with the pen down placing fruit
* `A`: replace with rules set A
* `B`: replace with rules set B
* `C`: replace with rules set C
* `D`: replace with rules set D
* `a`: replace with rules set A, chance 90%
* `b`: replace with rules set B, chance 80%
* `c`: replace with rules set C, chance 70%
* `d`: replace with rules set D, chance 60%
* `+`: yaw the turtle right by `angle` parameter
* `-`: yaw the turtle left by `angle` parameter
* `&`: pitch the turtle down by `angle` parameter
* `^`: pitch the turtle up by `angle` parameter
* `/`: roll the turtle to the right by `angle` parameter
* `*`: roll the turtle to the left by `angle` parameter
* `[`: save in stack current state info
* `]`: recover from stack state info

Example
-------

Spawn a small apple tree:

```lua
pos = {x=230,y=20,z=4}
apple_tree={
    axiom="FFFFFAFFBF",
    rules_a="[&&&FFFFF&&FFFF][&&&++++FFFFF&&FFFF][&&&----FFFFF&&FFFF]",
    rules_b="[&&&++FFFFF&&FFFF][&&&--FFFFF&&FFFF][&&&------FFFFF&&FFFF]",
    trunk="default:tree",
    leaves="default:leaves",
    angle=30,
    iterations=2,
    random_level=0,
    trunk_type="single",
    thin_branches=true,
    fruit_chance=10,
    fruit="default:apple"
}
core.spawn_tree(pos,apple_tree)
```
