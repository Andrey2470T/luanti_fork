// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "mapblock_mesh.h"
#include "Mesh/SMeshBuffer.h"
#include "client/core/client.h"
#include "client/render/material.h"
#include "mapblock.h"
#include "map.h"
#include "noise.h"
#include "profiler.h"
#include "client/media/shader.h"
#include "mesh.h"
#include "client/ui/minimap.h"
#include "client/mesh/meshgen.h"
#include "util/directiontables.h"
#include "util/tracy_wrapper.h"
#include "client/mesh/collector.h"
#include "client/render/renderingengine.h"
#include <array>
#include <algorithm>
#include <cmath>
#include "client/media/texturesource.h"
#include <Mesh/SMesh.h>
#include <Mesh/IMeshBuffer.h>
#include <Mesh/SMeshBuffer.h>

/*
	MeshMakeData
*/

MeshMakeData::MeshMakeData(const BlockLightPropagator *blocklight_fill,
		const NodeDefManager *ndef, u16 side_length, MeshGrid mesh_grid) :
	m_blocklight_fill(blocklight_fill),
	m_side_length(side_length),
	m_mesh_grid(mesh_grid),
	m_nodedef(ndef)
{
	assert(m_side_length > 0);
}

void MeshMakeData::fillBlockDataBegin(const v3s16 &blockpos)
{
	m_blockpos = blockpos;

	v3s16 blockpos_nodes = m_blockpos*MAP_BLOCKSIZE;

	m_vmanip.clear();
	// extra 1 block thick layer around the mesh
	VoxelArea voxel_area(blockpos_nodes - v3s16(1,1,1) * MAP_BLOCKSIZE,
			blockpos_nodes + v3s16(1,1,1) * (m_side_length + MAP_BLOCKSIZE) - v3s16(1,1,1));
	m_vmanip.addArea(voxel_area);
}

void MeshMakeData::fillBlockData(const v3s16 &bp, MapNode *data)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));

	v3s16 blockpos_nodes = bp * MAP_BLOCKSIZE;
	m_vmanip.copyFrom(data, data_area, v3s16(0,0,0), blockpos_nodes, data_size);
}

void MeshMakeData::fillSingleNode(MapNode data, MapNode padding)
{
	m_blockpos = {0, 0, 0};

	m_vmanip.clear();
	// area around 0,0,0 so that this positon has neighbors
	const s16 sz = 3;
	m_vmanip.addArea({v3s16(-sz), v3s16(sz)});

	u32 count = m_vmanip.m_area.getVolume();
	for (u32 i = 0; i < count; i++) {
		m_vmanip.m_data[i] = padding;
		m_vmanip.m_flags[i] &= ~VOXELFLAG_NO_DATA;
	}

	m_vmanip.setNodeNoEmerge({0, 0, 0}, data);
}

void MeshMakeData::setCrack(int crack_level, v3s16 crack_pos)
{
	if (crack_level >= 0)
		m_crack_pos_relative = crack_pos - m_blockpos*MAP_BLOCKSIZE;
}

/*
	MapBlockBspTree
*/

void MapBlockBspTree::buildTree(const std::vector<MeshTriangle> *triangles, u16 side_length)
{
	this->triangles = triangles;

	nodes.clear();

	// assert that triangle index can fit into s32
	assert(triangles->size() <= 0x7FFFFFFFL);
	std::vector<s32> indexes;
	indexes.reserve(triangles->size());
	for (u32 i = 0; i < triangles->size(); i++)
		indexes.push_back(i);

	if (!indexes.empty()) {
		// Start in the center of the block with increment of one quarter in each direction
		root = buildTree(v3f(1, 0, 0), v3f((side_length + 1) * 0.5f * BS), side_length * 0.25f * BS, indexes, 0);
	} else {
		root = -1;
	}
}

/**
 * @brief Find a candidate plane to split a set of triangles in two
 *
 * The candidate plane is represented by one of the triangles from the set.
 *
 * @param list Vector of indexes of the triangles in the set
 * @param triangles Vector of all triangles in the BSP tree
 * @return Address of the triangle that represents the proposed split plane
 */
static const MeshTriangle *findSplitCandidate(const std::vector<s32> &list, const std::vector<MeshTriangle> &triangles)
{
	// find the center of the cluster.
	v3f center(0, 0, 0);
	size_t n = list.size();
	for (s32 i : list) {
		center += triangles[i].centroid / n;
	}

	// find the triangle with the largest area and closest to the center
	const MeshTriangle *candidate_triangle = &triangles[list[0]];
	const MeshTriangle *ith_triangle;
	for (s32 i : list) {
		ith_triangle = &triangles[i];
		if (ith_triangle->areaSQ > candidate_triangle->areaSQ ||
				(ith_triangle->areaSQ == candidate_triangle->areaSQ &&
				ith_triangle->centroid.getDistanceFromSQ(center) < candidate_triangle->centroid.getDistanceFromSQ(center))) {
			candidate_triangle = ith_triangle;
		}
	}
	return candidate_triangle;
}

s32 MapBlockBspTree::buildTree(v3f normal, v3f origin, float delta, const std::vector<s32> &list, u32 depth)
{
	// if the list is empty, don't bother
	if (list.empty())
		return -1;

	// if there is only one triangle, or the delta is insanely small, this is a leaf node
	if (list.size() == 1 || delta < 0.01) {
		nodes.emplace_back(normal, origin, list, -1, -1);
		return nodes.size() - 1;
	}

	std::vector<s32> front_list;
	std::vector<s32> back_list;
	std::vector<s32> node_list;

	// split the list
	for (s32 i : list) {
		const MeshTriangle &triangle = (*triangles)[i];
		float factor = normal.dotProduct(triangle.centroid - origin);
		if (factor == 0)
			node_list.push_back(i);
		else if (factor > 0)
			front_list.push_back(i);
		else
			back_list.push_back(i);
	}

	// define the new split-plane
	v3f candidate_normal(normal.Z, normal.X, normal.Y);
	float candidate_delta = delta;
	if (depth % 3 == 2)
		candidate_delta /= 2;

	s32 front_index = -1;
	s32 back_index = -1;

	if (!front_list.empty()) {
		v3f next_normal = candidate_normal;
		v3f next_origin = origin + delta * normal;
		float next_delta = candidate_delta;
		if (next_delta < 5) {
			const MeshTriangle *candidate = findSplitCandidate(front_list, *triangles);
			next_normal = candidate->getNormal();
			next_origin = candidate->centroid;
		}
		front_index = buildTree(next_normal, next_origin, next_delta, front_list, depth + 1);

		// if there are no other triangles, don't create a new node
		if (back_list.empty() && node_list.empty())
			return front_index;
	}

	if (!back_list.empty()) {
		v3f next_normal = candidate_normal;
		v3f next_origin = origin - delta * normal;
		float next_delta = candidate_delta;
		if (next_delta < 5) {
			const MeshTriangle *candidate = findSplitCandidate(back_list, *triangles);
			next_normal = candidate->getNormal();
			next_origin = candidate->centroid;
		}

		back_index = buildTree(next_normal, next_origin, next_delta, back_list, depth + 1);

		// if there are no other triangles, don't create a new node
		if (front_list.empty() && node_list.empty())
			return back_index;
	}

	nodes.emplace_back(normal, origin, node_list, front_index, back_index);

	return nodes.size() - 1;
}

void MapBlockBspTree::traverse(s32 node, v3f viewpoint, std::vector<s32> &output) const
{
	if (node < 0) return; // recursion break;

	const TreeNode &n = nodes[node];
	float factor = n.normal.dotProduct(viewpoint - n.origin);

	if (factor > 0)
		traverse(n.back_ref, viewpoint, output);
	else
		traverse(n.front_ref, viewpoint, output);

	if (factor != 0)
		for (s32 i : n.triangle_refs)
			output.push_back(i);

	if (factor > 0)
		traverse(n.front_ref, viewpoint, output);
	else
		traverse(n.back_ref, viewpoint, output);
}



/*
	PartialMeshBuffer
*/

void PartialMeshBuffer::draw(video::VideoDriver *driver) const
{
	driver->drawMeshBuffer(m_buffer, m_indices.get());
}

/*
	MapBlockMesh
*/

MapBlockMesh::MapBlockMesh(Client *client, MeshMakeData *data):
	m_tsrc(client->getTextureSource()),
	m_shdrsrc(client->getShaderSource()),
	m_bounding_sphere_center((data->m_side_length * 0.5f - 0.5f) * BS)
{
	ZoneScoped;

	for (auto &m : m_mesh)
		m = make_irr<scene::SMesh>();

	auto mesh_grid = data->m_mesh_grid;
	v3s16 bp = data->m_blockpos;
	// Only generate minimap mapblocks at grid aligned coordinates.
	// FIXME: ^ doesn't really make sense. and in practice, bp is always aligned
	if (mesh_grid.isMeshPos(bp) && data->m_generate_minimap) {
		// meshgen area always fits into a grid cell
		m_minimap_mapblocks.resize(mesh_grid.getCellVolume(), nullptr);
		v3s16 ofs;

		// See also client.cpp for the code that reads the array of minimap blocks.
		for (ofs.Z = 0; ofs.Z < mesh_grid.cell_size; ofs.Z++)
		for (ofs.Y = 0; ofs.Y < mesh_grid.cell_size; ofs.Y++)
		for (ofs.X = 0; ofs.X < mesh_grid.cell_size; ofs.X++) {
			v3s16 p = (bp + ofs) * MAP_BLOCKSIZE;
			if (data->m_vmanip.getNodeNoEx(p).getContent() != CONTENT_IGNORE) {
				MinimapMapblock *block = new MinimapMapblock;
				m_minimap_mapblocks[mesh_grid.getOffsetIndex(ofs)] = block;
				block->getMinimapNodes(&data->m_vmanip, p);
			}
		}
	}

	// algin vertices to mesh grid, not meshgen area
	v3f offset = intToFloat((data->m_blockpos - mesh_grid.getMeshPos(data->m_blockpos)) * MAP_BLOCKSIZE, BS);

	MeshCollector collector(client->getRenderingEngine()->getAtlasPool(),
		 m_bounding_sphere_center, offset);

	{
		// Generate everything
		MapblockMeshGenerator(data, &collector).generate();
	}

	auto mat_storage = client->getMaterialStorage();

	/*
		Convert MeshCollector to SMesh
	*/

	m_bounding_radius = std::sqrt(collector.bounding_radius_sq);

	for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
		scene::SMesh *mesh = static_cast<scene::SMesh *>(m_mesh[layer].get());

		for(u32 i = 0; i < collector.prebuffers[layer].size(); i++)
		{
			PreMeshBuffer &p = collector.prebuffers[layer][i];

			// Create material
			video::SMaterial material;
			material.FogEnable = true;
			material.forEachTexture([] (auto &tex) {
				tex.MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;
				tex.MagFilter = video::ETMAGF_NEAREST;
			});

			{
				material.MaterialType = m_shdrsrc->getShaderInfo(
						p.layer.shader_id).material;
				p.layer.applyMaterialOptions(material, layer);
			}

			// Bind the cracks texture if the buffer was cracked
			if (p.layer.material_flags & MATERIAL_FLAG_CRACK) {
				u8 crackTexIndex = video::MATERIAL_MAX_TEXTURES - 1;
				material.setTexture(crackTexIndex, client->getCrackTexture());
				material.TextureLayers[crackTexIndex].TextureWrapU = video::ETC_REPEAT;
				material.TextureLayers[crackTexIndex].TextureWrapV = video::ETC_REPEAT;
			}

			// Apply properties from custom material
			mat_storage->applyToSMaterial(p.layer.override_material, &material);

			auto *buf = new NodeMeshBuffer();
			buf->Material = material;

			if (p.layer.isTransparent()) {
				buf->append(&p.vertices[0], p.vertices.size(), nullptr, 0);

				MeshTriangle t;
				t.buffer = buf;
				m_transparent_triangles.reserve(p.indices.size() / 3);
				for (u32 i = 0; i < p.indices.size(); i += 3) {
					t.p1 = p.indices[i];
					t.p2 = p.indices[i + 1];
					t.p3 = p.indices[i + 2];
					t.updateAttributes();
					m_transparent_triangles.push_back(t);
				}
			} else {
				buf->append(&p.vertices[0], p.vertices.size(),
					&p.indices[0], p.indices.size());
			}
			mesh->addMeshBuffer(buf);
			buf->drop();
		}

		if (mesh) {
			// Use VBO for mesh (this just would set this for every buffer)
			mesh->setHardwareMappingHint(scene::EHM_STATIC);
		}
	}

	m_bsp_tree.buildTree(&m_transparent_triangles, data->m_side_length);
}

MapBlockMesh::~MapBlockMesh()
{
	size_t sz = 0;
	for (auto &&m : m_mesh) {
		for (u32 i = 0; i < m->getMeshBufferCount(); i++)
			sz += m->getMeshBuffer(i)->getSize();
		m.reset();
	}
	for (MinimapMapblock *block : m_minimap_mapblocks)
		delete block;

	porting::TrackFreedMemory(sz);
}

void MapBlockMesh::updateTransparentBuffers(v3f camera_pos, v3s16 block_pos,
		bool group_by_buffers)
{
	// nothing to do if the entire block is opaque
	if (m_transparent_triangles.empty())
		return;

	v3f block_posf = intToFloat(block_pos * MAP_BLOCKSIZE, BS);
	v3f rel_camera_pos = camera_pos - block_posf;

	std::vector<s32> triangle_refs;
	m_bsp_tree.traverse(rel_camera_pos, triangle_refs);

	// arrange index sequences into partial buffers
	m_transparent_buffers_consolidated = false;
	m_transparent_buffers.clear();

	std::vector<std::pair<NodeMeshBuffer *, std::vector<u16>>> ordered_strains;
	std::unordered_map<NodeMeshBuffer *, size_t> strain_idxs;

	if (group_by_buffers) {
		// find (reversed) order for strains, by iterating front-to-back
		// (if a buffer A has a triangle nearer than all triangles of another
		// buffer B, A should be drawn in front of (=after) B)
		NodeMeshBuffer *current_buffer = nullptr;
		for (auto it = triangle_refs.rbegin(); it != triangle_refs.rend(); ++it) {
			const auto &t = m_transparent_triangles[*it];
			if (current_buffer == t.buffer)
				continue;
			current_buffer = t.buffer;
			auto [_it2, is_new] =
				strain_idxs.emplace(current_buffer, ordered_strains.size());
			if (is_new)
				ordered_strains.emplace_back(current_buffer, std::vector<u16>{});
		}
	}

	// find order for triangles, by iterating back-to-front
	NodeMeshBuffer *current_buffer = nullptr;
	std::vector<u16> *current_strain = nullptr;
	for (auto i : triangle_refs) {
		const auto &t = m_transparent_triangles[i];
		if (current_buffer != t.buffer) {
			current_buffer = t.buffer;
			if (group_by_buffers) {
				auto it = strain_idxs.find(current_buffer);
				assert(it != strain_idxs.end());
				current_strain = &ordered_strains[it->second].second;
			} else {
				ordered_strains.emplace_back(current_buffer, std::vector<u16>{});
				current_strain = &ordered_strains.back().second;
			}
		}
		current_strain->push_back(t.p1);
		current_strain->push_back(t.p2);
		current_strain->push_back(t.p3);
	}

	m_transparent_buffers.reserve(ordered_strains.size());
	if (group_by_buffers) {
		// the order was reversed
		for (auto it = ordered_strains.rbegin(); it != ordered_strains.rend(); ++it)
			m_transparent_buffers.emplace_back(it->first, std::move(it->second));
	} else {
		for (auto it = ordered_strains.begin(); it != ordered_strains.end(); ++it)
			m_transparent_buffers.emplace_back(it->first, std::move(it->second));
	}
}

void MapBlockMesh::consolidateTransparentBuffers()
{
	if (m_transparent_buffers_consolidated)
		return;
	m_transparent_buffers.clear();

	NodeMeshBuffer *current_buffer = nullptr;
	std::vector<u16> current_strain;

	// use the fact that m_transparent_triangles is already arranged by buffer
	for (const auto &t : m_transparent_triangles) {
		if (current_buffer != t.buffer) {
			if (current_buffer != nullptr) {
				this->m_transparent_buffers.emplace_back(current_buffer, std::move(current_strain));
				current_strain.clear();
			}
			current_buffer = t.buffer;
		}
		current_strain.push_back(t.p1);
		current_strain.push_back(t.p2);
		current_strain.push_back(t.p3);
	}

	if (!current_strain.empty()) {
		this->m_transparent_buffers.emplace_back(current_buffer, std::move(current_strain));
	}

	m_transparent_buffers_consolidated = true;
}
