Map terminology and coordinates
===============================

Nodes, mapblocks, mapchunks
---------------------------

A 'node' is the fundamental cubic unit of a world and appears to a player as
roughly 1x1x1 meters in size.

A 'mapblock' (often abbreviated to 'block') is 16x16x16 nodes and is the
fundamental region of a world that is stored in the world database, sent to
clients and handled by many parts of the engine.
'mapblock' is preferred terminology to 'block' to help avoid confusion with
'node', however 'block' often appears in the API.

A 'mapchunk' (sometimes abbreviated to 'chunk') is usually 5x5x5 mapblocks
(80x80x80 nodes) and is the volume of world generated in one operation by
the map generator.
The size in mapblocks has been chosen to optimize map generation.

Coordinates
-----------

### Orientation of axes

For node and mapblock coordinates, +X is East, +Y is up, +Z is North.

### Node coordinates

Almost all positions used in the API use node coordinates.

### Mapblock coordinates

Occasionally the API uses 'blockpos' which refers to mapblock coordinates that
specify a particular mapblock.
For example blockpos (0,0,0) specifies the mapblock that extends from
node position (0,0,0) to node position (15,15,15).

#### Converting node position to the containing blockpos

To calculate the blockpos of the mapblock that contains the node at 'nodepos',
for each axis:

* blockpos = math.floor(nodepos / 16)

#### Converting blockpos to min/max node positions

To calculate the min/max node positions contained in the mapblock at 'blockpos',
for each axis:

* Minimum:
  nodepos = blockpos * 16
* Maximum:
  nodepos = blockpos * 16 + 15
