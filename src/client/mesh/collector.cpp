// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "collector.h"
#include <stdexcept>
#include "log.h"
#include "client/mesh/mesh.h"
#include "client/render/atlas.h"

void MeshCollector::append(TileSpec &tile, const scene::Vertex3D *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		TileLayer *layer = &tile.layers[layernum];
		if (layer->empty())
			continue;
		append(*layer, vertices, numVertices, indices, numIndices, layernum,
				tile.world_aligned);
	}
}

void MeshCollector::append(TileLayer &layer, const scene::Vertex3D *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, u8 layernum,
		bool use_scale)
{
	auto atlas = pool->getAtlasByImage({layer.image, layer.anim_info});
	v2u32 tilePos, tileSize;

	if (atlas) {
		if (!layer.texture)
			layer.texture = atlas->getTexture();
		auto tile = atlas->getTileByImage(layer.image);
		u8 frameThickness = atlas->getFrameThickness();
		tilePos = tile->pos + v2u32(frameThickness);
		tileSize = tile->size - 2 * v2u32(frameThickness);
	}

	PreMeshBuffer &p = findBuffer(layer, layernum, numVertices);

	f32 scale = 1.0f;
	if (use_scale)
		scale = 1.0f / layer.scale;


	u32 vertex_count = p.vertices.size();
	for (u32 i = 0; i < numVertices; i++) {
		// Pack the tile color as 24 bits in the red channel
		u32 tc_u = 0;
		tc_u |= ((u32)layer.color.getRed() << 24);
		tc_u |= ((u32)layer.color.getGreen() << 16);
		tc_u |= ((u32)layer.color.getBlue() << 8);

		auto uv = scale * vertices[i].TCoords;

		// Convert the uv coords from the tile space to the atlas one
		u32 relPosX = core::round32(uv.X * tileSize.X);
		u32 relPosY = core::round32(uv.Y * tileSize.Y);

		uv.X = tilePos.X + relPosX;
		uv.Y = tilePos.Y + relPosY;

		// Pack the crack frame pixel coords in the green channel
		u32 wh_u = 0;
		wh_u |= (relPosX << 16);
		wh_u |= (u16)relPosY;

		float tc_f, wh_f;
		std::memcpy(&tc_f, &tc_u, sizeof(tc_f));
		std::memcpy(&wh_f, &wh_u, sizeof(wh_f));

		v3f hw_color = {tc_f, wh_f, 0.0f};

		p.vertices.push_back({{vertices[i].Pos + offset, vertices[i].Normal,
			vertices[i].Color, uv}, hw_color});
		bounding_radius_sq = std::max(bounding_radius_sq,
				(vertices[i].Pos - center_pos).getLengthSQ());
	}

	for (u32 i = 0; i < numIndices; i++)
		p.indices.push_back(indices[i] + vertex_count);
}

PreMeshBuffer &MeshCollector::findBuffer(
		const TileLayer &layer, u8 layernum, u32 numVertices)
{
	if (numVertices > U16_MAX)
		throw std::invalid_argument(
				"Mesh can't contain more than 65536 vertices");
	std::vector<PreMeshBuffer> &buffers = prebuffers[layernum];
	for (PreMeshBuffer &p : buffers)
		if (p.layer == layer && p.vertices.size() + numVertices <= U16_MAX)
			return p;
	buffers.emplace_back(layer);
	return buffers.back();
}
