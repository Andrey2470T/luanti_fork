// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#pragma once
#include <array>
#include <vector>
#include "irrlichttypes.h"
#include "irr_v3d.h"
#include <Mesh/VertexTypes.h>
#include "client/render/tile.h"

struct PreMeshBuffer
{
	TileLayer layer;
	std::vector<u16> indices;
	std::vector<scene::Vertex3DExt> vertices;

	PreMeshBuffer() = default;
	explicit PreMeshBuffer(const TileLayer &layer) : layer(layer) {}
};

class AtlasPool;

struct MeshCollector
{
	AtlasPool *pool;

	std::array<std::vector<PreMeshBuffer>, MAX_TILE_LAYERS> prebuffers;
	// bounding sphere radius and center
	f32 bounding_radius_sq = 0.0f;
	v3f center_pos;
	v3f offset;

	// center_pos: pos to use for bounding-sphere, in BS-space
	// offset: offset added to vertices
	MeshCollector(AtlasPool *_pool, const v3f _center_pos, v3f _offset = v3f())
		: pool(_pool), center_pos(_center_pos), offset(_offset) {}

	void append(TileSpec &material,
			const scene::Vertex3D *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices);

private:
	void append(TileLayer &material,
			const scene::Vertex3D *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices,
			u8 layernum, bool use_scale = false);

	PreMeshBuffer &findBuffer(const TileLayer &layer, u8 layernum, u32 numVertices);
};
