// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "irrlichttypes.h"
#include "Utils/irr_ptr.h"
#include "Mesh/IMesh.h"
#include "Mesh/SMeshBuffer.h"

#include "util/numeric.h"
#include "client/render/tile.h"
#include "voxel.h"
#include <array>
#include <map>
#include <unordered_map>

namespace video {
	class VideoDriver;
}

class Client;
class NodeDefManager;
class ShaderSource;
class ITextureSource;
class BlockLightFloodFill;

/*
	Mesh making stuff
*/


class MapBlock;
struct MinimapMapblock;

struct MeshMakeData
{
	const BlockLightFloodFill *m_blocklight_fill = nullptr;
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
	v3s16 m_crack_pos_relative = v3s16(-1337,-1337,-1337);
	bool m_generate_minimap = false;
	bool m_smooth_lighting = false;
	bool m_enable_water_reflections = false;

	const NodeDefManager *m_nodedef;

	MeshMakeData(
		const BlockLightFloodFill *blocklight_fill, const NodeDefManager *ndef,
		u16 side_lingth, MeshGrid mesh_grid);

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
	void setCrack(int crack_level, v3s16 crack_pos);
};

using NodeMeshBuffer = scene::CMeshBuffer<scene::Vertex3DExt>;

// represents a triangle as indexes into the vertex buffer in NodeMeshBuffer
class MeshTriangle
{
public:
	NodeMeshBuffer *buffer;
	u16 p1, p2, p3;
	v3f centroid;
	float areaSQ;

	void updateAttributes()
	{
		v3f v1 = buffer->getPosition(p1);
		v3f v2 = buffer->getPosition(p2);
		v3f v3 = buffer->getPosition(p3);

		centroid = (v1 + v2 + v3) / 3;
		areaSQ = (v2-v1).crossProduct(v3-v1).getLengthSQ() / 4;
	}

	v3f getNormal() const {
		v3f v1 = buffer->getPosition(p1);
		v3f v2 = buffer->getPosition(p2);
		v3f v3 = buffer->getPosition(p3);

		return (v2-v1).crossProduct(v3-v1);
	}
};

/**
 * Implements a binary space partitioning tree
 * See also: https://en.wikipedia.org/wiki/Binary_space_partitioning
 */
class MapBlockBspTree
{
public:
	MapBlockBspTree() {}

	void buildTree(const std::vector<MeshTriangle> *triangles, u16 side_lingth);

	void traverse(v3f viewpoint, std::vector<s32> &output) const
	{
		traverse(root, viewpoint, output);
	}

private:
	// Tree node definition;
	struct TreeNode
	{
		v3f normal;
		v3f origin;
		std::vector<s32> triangle_refs;
		s32 front_ref;
		s32 back_ref;

		TreeNode() = default;
		TreeNode(v3f normal, v3f origin, const std::vector<s32> &triangle_refs, s32 front_ref, s32 back_ref) :
				normal(normal), origin(origin), triangle_refs(triangle_refs), front_ref(front_ref), back_ref(back_ref)
		{}
	};


	s32 buildTree(v3f normal, v3f origin, float delta, const std::vector<s32> &list, u32 depth);
	void traverse(s32 node, v3f viewpoint, std::vector<s32> &output) const;

	const std::vector<MeshTriangle> *triangles = nullptr; // this reference is managed externally
	std::vector<TreeNode> nodes; // list of nodes
	s32 root = -1; // index of the root node
};

/*
 * PartialMeshBuffer
 *
 * Attach alternate `Indices` to an existing mesh buffer, to make it possible to use different
 * indices with the same vertex buffer.
 */
class PartialMeshBuffer
{
public:
	PartialMeshBuffer(NodeMeshBuffer *buffer, std::vector<u16> &&vertex_indices) :
			m_buffer(buffer), m_indices(make_irr<scene::SIndexBuffer>())
	{
		m_indices->Data = std::move(vertex_indices);
		m_indices->setHardwareMappingHint(scene::EHM_STATIC);
	}

	auto *getBuffer() const { return m_buffer; }

	void draw(video::VideoDriver *driver) const;

private:
	NodeMeshBuffer *m_buffer;
	irr_ptr<scene::SIndexBuffer> m_indices;
};

/*
	Holds a mesh for a mapblock.

	Besides the SMesh*, this contains information used fortransparency sorting
	and texture animation.
	For example:
	- cracks
	- day/night transitions
*/
class MapBlockMesh
{
public:
	// Builds the mesh given
	MapBlockMesh(Client *client, MeshMakeData *data);
	~MapBlockMesh();

	/// @warning ClientMap requires that the vertex and index data is not modified
	scene::IMesh *getMesh()
	{
		return m_mesh[0].get();
	}

	/// @param layer layer index
	/// @warning ClientMap requires that the vertex and index data is not modified
	scene::IMesh *getMesh(u8 layer)
	{
		assert(layer < MAX_TILE_LAYERS);
		return m_mesh[layer].get();
	}

	std::vector<MinimapMapblock*> moveMinimapMapblocks()
	{
		std::vector<MinimapMapblock*> minimap_mapblocks;
		minimap_mapblocks.swap(m_minimap_mapblocks);
		return minimap_mapblocks;
	}

	/// Radius of the bounding-sphere, in BS-space.
	f32 getBoundingRadius() const { return m_bounding_radius; }

	/// Center of the bounding-sphere, in BS-space, relative to block pos.
	v3f getBoundingSphereCenter() const { return m_bounding_sphere_center; }

	/** Update transparent buffers to render towards the camera.
	 * @param group_by_buffers If true, triangles in the same buffer are batched
	 *     into the same PartialMeshBuffer, resulting in fewer draw calls, but
	 *     wrong order. Triangles within a single buffer are still ordered, and
	 *     buffers are ordered relative to each other (with respect to their nearest
	 *     triangle).
	 */
	void updateTransparentBuffers(v3f camera_pos, v3s16 block_pos, bool group_by_buffers);
	void consolidateTransparentBuffers();

	/// get the list of transparent buffers
	const std::vector<PartialMeshBuffer> &getTransparentBuffers() const
	{
		return this->m_transparent_buffers;
	}

private:

	irr_ptr<scene::IMesh> m_mesh[MAX_TILE_LAYERS];
	std::vector<MinimapMapblock*> m_minimap_mapblocks;
	ITextureSource *m_tsrc;
	ShaderSource *m_shdrsrc;

	f32 m_bounding_radius;
	v3f m_bounding_sphere_center;

	// list of all semitransparent triangles in the mapblock
	std::vector<MeshTriangle> m_transparent_triangles;
	// Binary Space Partitioning tree for the block
	MapBlockBspTree m_bsp_tree;
	// Ordered list of references to parts of transparent buffers to draw
	std::vector<PartialMeshBuffer> m_transparent_buffers;
	// Is m_transparent_buffers currently in consolidated form?
	bool m_transparent_buffers_consolidated = false;
};
