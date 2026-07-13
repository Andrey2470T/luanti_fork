// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "collector.h"
#include <stdexcept>
#include "log.h"
#include "client/mesh/mesh.h"
#include "client/render/atlas.h"

void MeshCollector::append(TileSpec &tile, const scene::Vertex3DExt *vertices,
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

void MeshCollector::append(TileLayer &layer, const scene::Vertex3DExt *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, u8 layernum,
		bool use_scale)
{
	v2u32 tilePos, tileSize;

	auto atlas = pool ? pool->getAtlasByImage({layer.image, layer.anim_info}) : nullptr;
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
		f32 pack_r_f, pack_g_f, pack_b_f;

		// Pack the tile color as 24 bits in the red channel
		u32 pack_r = 0;
		pack_r |= ((u32)layer.color.getRed() << 24);
		pack_r |= ((u32)layer.color.getGreen() << 16);
		pack_r |= ((u32)layer.color.getBlue() << 8);

		pack_r |= ((u32)(layer.material_flags & MATERIAL_FLAG_CRACK) ? 1u : 0u);

		std::memcpy(&pack_r_f, &pack_r, sizeof(pack_r_f));

		auto uv = scale * vertices[i].TCoords;

		// Convert the uv coords from the tile space to the atlas one
		u32 relPosX = core::round32(uv.X * tileSize.X);
		u32 relPosY = core::round32(uv.Y * tileSize.Y);

		uv.X = tilePos.X + relPosX;
		uv.Y = tilePos.Y + relPosY;

		u32 pack_b = 0;
		std::memcpy(&pack_b, &vertices[i].Aux.Z, sizeof(pack_b));

		if (atlas) {
			// Pack the crack frame pixel coords and crack flag in the green channel
			u32 pack_g = 0;
			pack_g |= (relPosX << 16);
			pack_g |= (u16)relPosY;

			// Pack the tile width and height as 8-bit in the blue channel
			pack_b |= (tileSize.X << 24);
			pack_b |= (tileSize.Y << 16 & 0xff0000u);

			std::memcpy(&pack_g_f, &pack_g, sizeof(pack_g_f));
		}

		std::memcpy(&pack_b_f, &pack_b, sizeof(pack_b_f));

		v3f inAux = {pack_r_f, pack_g_f, pack_b_f};

		p.vertices.push_back({{vertices[i].Pos + offset, vertices[i].Normal,
			vertices[i].Color, uv}, inAux});
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
