// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include "client/mesh/layeredmesh.h"
#include "util/numeric.h"
#include "client/render/tilelayer.h"
#include "voxel.h"

class Client;
class NodeDefManager;
class RenderSystem;
class ResourceCache;
class LayeredMesh;

/*
	Mesh making stuff
*/


class MapBlock;
struct MinimapMapblock;
class DistanceSortedDrawList;

struct MeshMakeData
{
	VoxelManipulator m_vmanip;

	// base pos of meshgen area, in blocks
	v3s16 m_blockpos = v3s16(-1337,-1337,-1337);
	// size of meshgen area, in nodes.
	// vmanip will have at least an extra 1 node onion layer.
	// area is expected to fit into mesh grid cell.
	u16 m_side_length;
	// vertex positions will be relative to this grid
	MeshGrid m_mesh_grid;

	// relative to blockpos
	//v3s16 m_crack_pos_relative = v3s16(-1337,-1337,-1337);
	bool m_generate_minimap = false;
	bool m_smooth_lighting = false;
	bool m_enable_water_reflections = false;

	const NodeDefManager *m_nodedef;

	MeshMakeData(const NodeDefManager *ndef, u16 side_lingth, MeshGrid mesh_grid);

	/*
		Copy block data manually (to allow optimizations by the caller)
	*/
	void fillBlockDataBegin(const v3s16 &blockpos);
	void fillBlockData(const v3s16 &bp, MapNode *data);

	/*
		Prepare block data for rendering a single node located at (0,0,0).
	*/
	void fillSingleNode(MapNode data, MapNode padding = MapNode(CONTENT_AIR));

	/*
		Set the (node) position of a crack
	*/
	//void setCrack(int crack_level, v3s16 crack_pos);
};

/*
	Holds a mesh for a mapblock.
*/
class MapBlockMesh
{
public:
	// Builds the mesh given
    MapBlockMesh(Client *client, MeshMakeData *data);
	~MapBlockMesh();

	LayeredMesh *getMesh() const
	{
        return m_mesh;
	}

    std::vector<std::shared_ptr<MinimapMapblock>> moveMinimapMapblocks()
	{
        std::vector<std::shared_ptr<MinimapMapblock>> minimap_mapblocks;
        minimap_mapblocks.swap(m_minimap_mapblocks);
		return minimap_mapblocks;
	}

	/// Radius of the bounding-sphere, in BS-space.
    f32 getBoundingRadius() const
    {
        return m_mesh->getBoundingSphereRadius();
    }

	/// Center of the bounding-sphere, in BS-space, relative to block pos.
    v3f getBoundingSphereCenter() const
    {
        return m_mesh->getBoundingSphereCenter();
    }
	
    void updateIndexBuffers(v3s16 camera_pos);

    void addActiveObject(u16 id);
    void removeActiveObject(u16 id);

    std::set<u16> getActiveObjects()
    {
        return m_active_objects;
    }

private:
    void addInDrawList(bool shadow=false);
    void removeFromDrawList(bool shadow=false);

    Client *m_client;

    LayeredMesh *m_mesh = nullptr;
    std::vector<std::shared_ptr<MinimapMapblock>> m_minimap_mapblocks;

    std::set<u16> m_active_objects; // all AOs currently locating within the mapblock
};
