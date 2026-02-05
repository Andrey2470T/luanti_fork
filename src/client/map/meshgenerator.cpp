// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2025 Unified Mesh Generator Refactor

#include <cmath>
#include "meshgenerator.h"
#include "util/basic_macros.h"
#include "util/numeric.h"
#include "util/directiontables.h"
#include "util/tracy_wrapper.h"
#include "mapblockmesh.h"
#include "settings.h"
#include "nodedef.h"
#include "client/render/tilelayer.h"
#include "client/mesh/meshoperations.h"
#include "client/core/client.h"
#include "noise.h"
#include "client/mesh/layeredmesh.h"
#include "client/render/batcher3d.h"
#include "client/mesh/defaultVertexTypes.h"
#include "client/mesh/model.h"

// Constants
#define FRAMED_EDGE_COUNT 12
#define FRAMED_NEIGHBOR_COUNT 18

const std::string MeshGenerator::raillike_groupname = "connect_to_raillike";

MeshGenerator::MeshGenerator(MeshMakeData *input, LayeredMesh *output):
	data(input),
	collector(output),
	nodedef(data->m_nodedef),
	blockpos_nodes(data->m_blockpos * MAP_BLOCKSIZE)
{
}

// ============================================================================
// TILE MANAGEMENT
// ============================================================================

void MeshGenerator::useTile(TileSpec *tile_ret, int index, u8 set_flags,
		u8 reset_flags, bool special)
{
	if (special)
		getSpecialTile(index, tile_ret);
	else
		getTile(index, tile_ret);

	for (auto &layer : *tile_ret) {
        layer.material_flags |= set_flags;
        layer.material_flags &= ~reset_flags;
	}
}

void MeshGenerator::getTile(int index, TileSpec *tile_ret)
{
	getNodeTileN(cur_node.n, cur_node.p, index, data, *tile_ret);
}

void MeshGenerator::getTile(v3s16 direction, TileSpec *tile_ret)
{
	getNodeTile(cur_node.n, cur_node.p, direction, data, *tile_ret);
}

void MeshGenerator::getSpecialTile(int index, TileSpec *tile_ret)
{
	*tile_ret = cur_node.f->special_tiles[index];
    TileLayer top_layer;

	for (auto &layer : *tile_ret) {
        if (!layer.tile_ref)
			continue;
		top_layer = layer;
        if (!(layer.material_flags & MATERIAL_FLAG_HARDWARE_COLORIZED))
            cur_node.n.getColor(*cur_node.f, &layer.color);
	}
}

// ============================================================================
// UNIFIED LIGHTING HELPERS
// ============================================================================

std::array<img::color8, 4> MeshGenerator::calculateFaceColors(
	int face_index,
	const NodeLighting &lighting,
	u8 light_source)
{
	std::array<img::color8, 4> colors;
	
	// Face normals for sunlight boost
	static const v3f face_normals[6] = {
		v3f(0, 1, 0), v3f(0, -1, 0),
		v3f(1, 0, 0), v3f(-1, 0, 0),
		v3f(0, 0, 1), v3f(0, 0, -1)
	};
	v3f normal = face_normals[face_index];

	for (int i = 0; i < 4; i++) {
		const VertexLight &vl = lighting.faces[face_index][i];
		colors[i] = encodeVertexLightWithSun(vl, light_source, normal);
	}

	return colors;
}

img::color8 MeshGenerator::calculateVertexColor(
	const v3f &position,
	const v3f &normal,
	const NodeLighting &lighting,
	u8 light_source)
{
	VertexLight vl = interpolateLight(lighting, position);
	return encodeVertexLightWithSun(vl, light_source, normal);
}

// ============================================================================
// CORE DRAWING PRIMITIVES
// ============================================================================

#define SELECT_VERTEXTYPE(layer) \
	layer.material_flags & MATERIAL_FLAG_HARDWARE_COLORIZED ? \
	TwoColorNodeVType : NodeVType

void MeshGenerator::appendQuad(
	const std::array<v3f, 4> &positions,
	const std::array<v3f, 4> &normals,
	const TileSpec &tile,
	const rectf *uv)
{
    if (first_stage) {
        mergeMeshPart(tile[0], NodeVType, 4, 6);
        mergeMeshPart(tile[1], TwoColorNodeVType, 4, 6);
    }
    else {
        std::array<v3f, 4> transformed_positions = positions;
        std::array<img::color8, 4> colors;
        for (int i = 0; i < 4; i++) {
            transformed_positions[i] += cur_node.origin;
            colors[i] = calculateVertexColor(transformed_positions[i], normals[i],
                cur_node.lighting, cur_node.f->light_source);
        }
        rectf uvs = uv ? *uv : rectf(v2f(0.0, 1.0), v2f(1.0, 0.0));

        auto buf1 = findBuffer(tile[0], SELECT_VERTEXTYPE(tile[0]), 4, 6);
        Batcher3D::face(buf1, transformed_positions, colors, uvs, normals);

        auto buf2 = findBuffer(tile[1], SELECT_VERTEXTYPE(tile[1]), 4, 6);
        Batcher3D::face(buf2, transformed_positions, colors, uvs, normals);
    }
}

void MeshGenerator::appendCuboid(
	const aabbf &box,
	const std::array<TileSpec, 6> &tiles,
	const NodeLighting &lighting,
    u8 face_mask,
    const std::array<rectf, 6> *uvs)
{
	// Apply visual scale
	aabbf transformed_box = box;
	bool scale = std::fabs(cur_node.f->visual_scale - 1.0f) > 1e-3f;
	if (scale) {
		transformed_box.MinEdge *= cur_node.f->visual_scale;
		transformed_box.MaxEdge *= cur_node.f->visual_scale;
	}
	transformed_box.MinEdge += cur_node.origin;
	transformed_box.MaxEdge += cur_node.origin;

    std::array<img::color8, 24> colors;

	for (int face = 0; face < 6; face++) {
        if (!(face_mask & (1 << face)))
			continue;  // Face is culled

        if (first_stage) {
            mergeMeshPart(tiles[face][0], SELECT_VERTEXTYPE(tiles[face][0]), 4, 6);
            mergeMeshPart(tiles[face][1], SELECT_VERTEXTYPE(tiles[face][1]), 4, 6);
        }
        else {
            // Get face colors from pre-calculated lighting
            std::array<img::color8, 4> face_colors = calculateFaceColors(
                face, lighting, cur_node.f->light_source);

            colors[face*4] = face_colors[0];
            colors[face*4+1] = face_colors[1];
            colors[face*4+2] = face_colors[2];
            colors[face*4+3] = face_colors[3];

            auto buf1 = findBuffer(tiles[face][0], NodeVType, 4, 6);
            Batcher3D::boxFace(buf1, (BoxFaces)face, transformed_box, colors, uvs, face_mask);

            auto buf2 = findBuffer(tiles[face][1], TwoColorNodeVType, 4, 6);
            Batcher3D::boxFace(buf2, (BoxFaces)face, transformed_box, colors, uvs, face_mask);

        }
	}
}

// ============================================================================
// DRAWTYPE IMPLEMENTATIONS
// ============================================================================

void MeshGenerator::appendSolidNode()
{
	u8 visible_faces = 0;
	std::array<TileSpec, 6> tiles;

	static const v3s16 tile_dirs[6] = {
		v3s16(0, 1, 0), v3s16(0, -1, 0),
		v3s16(1, 0, 0), v3s16(-1, 0, 0),
		v3s16(0, 0, 1), v3s16(0, 0, -1)
	};

	content_t n1 = cur_node.n.getContent();
	for (int face = 0; face < 6; face++) {
		v3s16 p2 = blockpos_nodes + cur_node.p + tile_dirs[face];
		MapNode neighbor = data->m_vmanip.getNodeNoEx(p2);
		content_t n2 = neighbor.getContent();
		
		bool backface_culling = cur_node.f->drawtype == NDT_NORMAL;
		
		// Culling logic
		if (n2 == n1)
			continue;
		if (n2 == CONTENT_IGNORE)
			continue;
		if (n2 != CONTENT_AIR) {
			const ContentFeatures &f2 = nodedef->get(n2);
			if (f2.solidness == 2)
				continue;
			if (cur_node.f->drawtype == NDT_LIQUID) {
				if (cur_node.f->sameLiquidRender(f2))
					continue;
				backface_culling = f2.solidness || f2.visual_solidness;
			}
		}
		
		visible_faces |= 1 << face;
		getTile(tile_dirs[face], &tiles[face]);
		
		for (auto &layer : tiles[face]) {
			if (backface_culling)
                layer.material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
		}
	}

	if (!visible_faces)
		return;  // All faces culled

	aabbf box(v3f(-0.5 * BS), v3f(0.5 * BS));
	
    appendCuboid(box, tiles, cur_node.lighting, visible_faces);
}

void MeshGenerator::appendLiquidNode()
{
	prepareLiquidNodeDrawing();
	getLiquidNeighborhood();
	calculateCornerLevels();
	
    appendLiquidSides();
	if (!cur_liquid.top_is_same_liquid)
        appendLiquidTop();
	if (cur_liquid.draw_bottom)
        appendLiquidBottom();
}

void MeshGenerator::appendGlasslikeNode()
{
	TileSpec tile;
	useTile(&tile, 0, 0, 0);

	for (int face = 0; face < 6; face++) {
		v3s16 dir = g_6dirs[face];
		v3s16 neighbor_pos = blockpos_nodes + cur_node.p + dir;
		MapNode neighbor = data->m_vmanip.getNodeNoExNoEmerge(neighbor_pos);
		
		if (neighbor.getContent() == cur_node.n.getContent())
			continue;

		// Create face vertices
		std::array<v3f, 4> vertices = {
			v3f(-BS/2,  BS/2, -BS/2),
			v3f( BS/2,  BS/2, -BS/2),
			v3f( BS/2, -BS/2, -BS/2),
			v3f(-BS/2, -BS/2, -BS/2),
		};

        // Rotate based on face
        for (v3f &vertex : vertices) {
            switch (face) {
            case D6D_ZP: vertex.rotateXZBy(180); break;
            case D6D_YP: vertex.rotateYZBy( 90); break;
            case D6D_XP: vertex.rotateXZBy( 90); break;
            case D6D_ZN: vertex.rotateXZBy(  0); break;
            case D6D_YN: vertex.rotateYZBy(-90); break;
            case D6D_XN: vertex.rotateXZBy(-90); break;
            }
		}

		// Calculate colors
		v3f dir_f = v3f(dir.X, dir.Y, dir.Z);
		std::array<v3f, 4> normals = {dir_f, dir_f, dir_f, dir_f};

        appendQuad(vertices, normals, tile);
	}
}

void MeshGenerator::appendAllfacesNode()
{
	static const aabbf box(-BS/2, -BS/2, -BS/2, BS/2, BS/2, BS/2);
	std::array<TileSpec, 6> tiles;
	
	static const v3s16 nodebox_tile_dirs[6] = {
		v3s16(0, 1, 0), v3s16(0, -1, 0),
		v3s16(1, 0, 0), v3s16(-1, 0, 0),
		v3s16(0, 0, 1), v3s16(0, 0, -1)
	};
	
	for (int face = 0; face < 6; face++)
		getTile(nodebox_tile_dirs[face], &tiles[face]);

    appendCuboid(box, tiles, cur_node.lighting, 0xff);
}

void MeshGenerator::appendTorchlikeNode()
{
	u8 wall = cur_node.n.getWallMounted(nodedef);
	u8 tileindex = 0;
	switch (wall) {
	case DWM_YP: tileindex = 1; break;
	case DWM_YN: tileindex = 0; break;
	case DWM_S1: tileindex = 1; break;
	case DWM_S2: tileindex = 0; break;
	default: tileindex = 2;
	}
	
	TileSpec tile;
	useTile(&tile, tileindex, 0, MATERIAL_FLAG_BACKFACE_CULLING);

	float size = BS / 2 * cur_node.f->visual_scale;
	std::array<v3f, 4> vertices = {
		v3f(-size,  size, 0),
		v3f( size,  size, 0),
		v3f( size, -size, 0),
		v3f(-size, -size, 0),
	};

    for (v3f &vertex : vertices) {
        switch (wall) {
        case DWM_YP:
            vertex.Y += -size + BS/2;
            vertex.rotateXZBy(-45);
            break;
        case DWM_YN:
            vertex.Y += size - BS/2;
            vertex.rotateXZBy(45);
            break;
        case DWM_XP:
            vertex.X += -size + BS/2;
            break;
        case DWM_XN:
            vertex.X += -size + BS/2;
            vertex.rotateXZBy(180);
            break;
        case DWM_ZP:
            vertex.X += -size + BS/2;
            vertex.rotateXZBy(90);
            break;
        case DWM_ZN:
            vertex.X += -size + BS/2;
            vertex.rotateXZBy(-90);
            break;
        case DWM_S1:
            vertex.Y += -size + BS/2;
            vertex.rotateXZBy(45);
            break;
        case DWM_S2:
            vertex.Y += size - BS/2;
            vertex.rotateXZBy(-45);
            break;
        }
	}

	// Calculate colors with interpolation
	std::array<v3f, 4> normals = {v3f(0,0,-1), v3f(0,0,-1), v3f(0,0,-1), v3f(0,0,-1)};

    appendQuad(vertices, normals, tile);
}

void MeshGenerator::appendSignlikeNode()
{
	u8 wall = cur_node.n.getWallMounted(nodedef);
	TileSpec tile;
	useTile(&tile, 0, 0, MATERIAL_FLAG_BACKFACE_CULLING);
	
	static const float offset = BS / 16;
	float size = BS / 2 * cur_node.f->visual_scale;
	
	std::array<v3f, 4> vertices = {
		v3f(BS/2 - offset,  size,  size),
		v3f(BS/2 - offset,  size, -size),
		v3f(BS/2 - offset, -size, -size),
		v3f(BS/2 - offset, -size,  size),
	};

    for (v3f &vertex : vertices) {
        switch (wall) {
        case DWM_YP: vertex.rotateXYBy( 90); break;
        case DWM_YN: vertex.rotateXYBy(-90); break;
        case DWM_XP: vertex.rotateXZBy(  0); break;
        case DWM_XN: vertex.rotateXZBy(180); break;
        case DWM_ZP: vertex.rotateXZBy( 90); break;
        case DWM_ZN: vertex.rotateXZBy(-90); break;
        case DWM_S1:
            vertex.rotateXYBy( 90);
            vertex.rotateXZBy(90);
            break;
        case DWM_S2:
            vertex.rotateXYBy(-90);
            vertex.rotateXZBy(-90);
            break;
        }
	}

	std::array<v3f, 4> normals = {v3f(1,0,0), v3f(1,0,0), v3f(1,0,0), v3f(1,0,0)};

    appendQuad(vertices, normals, tile);
}

void MeshGenerator::appendPlantlikeNode()
{
	TileSpec tile;
	useTile(&tile);
    appendPlantlike(tile, false);
}

void MeshGenerator::appendPlantlikeRootedNode()
{
    appendSolidNode();
	
	TileSpec tile;
	useTile(&tile, 0, 0, 0, true);
	cur_node.origin += v3f(0.0, BS, 0.0);
	cur_node.p.Y++;
	
	// Recalculate lighting for upper node
	calculateNodeLighting(
		cur_node.lighting,
		blockpos_nodes + cur_node.p,
		data->m_smooth_lighting,
		data
	);
	
    appendPlantlike(tile, true);
	cur_node.p.Y--;
}

void MeshGenerator::appendFirelikeNode()
{
	TileSpec tile;
	useTile(&tile);

	// Check for adjacent nodes
	bool neighbors = false;
	bool neighbor[6] = {0, 0, 0, 0, 0, 0};
	content_t current = cur_node.n.getContent();
	
	for (int i = 0; i < 6; i++) {
		v3s16 n2p = blockpos_nodes + cur_node.p + g_6dirs[i];
		MapNode n2 = data->m_vmanip.getNodeNoEx(n2p);
		content_t n2c = n2.getContent();
		if (n2c != CONTENT_IGNORE && n2c != CONTENT_AIR && n2c != current) {
			neighbor[i] = true;
			neighbors = true;
		}
	}
	
	bool drawBasicFire = neighbor[D6D_YN] || !neighbors;
	bool drawBottomFire = neighbor[D6D_YP];

	if (drawBasicFire || neighbor[D6D_ZP])
        appendFirelikeQuad(tile, 0, -10, 0.4 * BS);
	else if (drawBottomFire)
        appendFirelikeQuad(tile, 0, 70, 0.47 * BS, 0.484 * BS);

	if (drawBasicFire || neighbor[D6D_XN])
        appendFirelikeQuad(tile, 90, -10, 0.4 * BS);
	else if (drawBottomFire)
        appendFirelikeQuad(tile, 90, 70, 0.47 * BS, 0.484 * BS);

	if (drawBasicFire || neighbor[D6D_ZN])
        appendFirelikeQuad(tile, 180, -10, 0.4 * BS);
	else if (drawBottomFire)
        appendFirelikeQuad(tile, 180, 70, 0.47 * BS, 0.484 * BS);

	if (drawBasicFire || neighbor[D6D_XP])
        appendFirelikeQuad(tile, 270, -10, 0.4 * BS);
	else if (drawBottomFire)
        appendFirelikeQuad(tile, 270, 70, 0.47 * BS, 0.484 * BS);

	if (drawBasicFire) {
        appendFirelikeQuad(tile, 45, 0, 0.0);
        appendFirelikeQuad(tile, -45, 0, 0.0);
	}
}

void MeshGenerator::appendFencelikeNode()
{
	TileSpec tile_nocrack;
	useTile(&tile_nocrack, 0, 0, 0);

	TileSpec tile_rot = tile_nocrack;
    tile_rot[0].rotation = TileRotation::R90;
    tile_rot[1].rotation = TileRotation::R90;

	static const f32 post_rad = BS / 8;
	static const f32 bar_rad  = BS / 16;
	static const f32 bar_len  = BS / 2 - post_rad;

	// The post - always present
	static const aabbf post(-post_rad, -BS/2, -post_rad, post_rad, BS/2, post_rad);
	static const std::array<rectf, 6> postuv = {
		rectf(v2f(0.375, 0.375), v2f(0.625, 0.625)),
		rectf(v2f(0.375, 0.375), v2f(0.625, 0.625)),
		rectf(v2f(0.000, 0.000), v2f(0.250, 1.000)),
		rectf(v2f(0.250, 0.000), v2f(0.500, 1.000)),
		rectf(v2f(0.500, 0.000), v2f(0.750, 1.000)),
		rectf(v2f(0.750, 0.000), v2f(1.000, 1.000))
	};

    appendCuboid(post, {tile_rot}, cur_node.lighting, 0xff, &postuv);

	// Check +X neighbor
	v3s16 p2 = cur_node.p;
	p2.X++;
	MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
	const ContentFeatures *f2 = &nodedef->get(n2);
	
	if (f2->drawtype == NDT_FENCELIKE) {
		static const aabbf bar_x1(BS/2 - bar_len,  BS/4 - bar_rad, -bar_rad,
								   BS/2 + bar_len,  BS/4 + bar_rad,  bar_rad);
		static const aabbf bar_x2(BS/2 - bar_len, -BS/4 - bar_rad, -bar_rad,
								   BS/2 + bar_len, -BS/4 + bar_rad,  bar_rad);
        static const std::array<rectf, 6> xrailuv = {
            rectf(v2f(0.000, 0.125), v2f(1.000, 0.250)),
            rectf(v2f(0.000, 0.250), v2f(1.000, 0.375)),
            rectf(v2f(0.375, 0.375), v2f(0.500, 0.500)),
            rectf(v2f(0.625, 0.625), v2f(0.750, 0.750)),
            rectf(v2f(0.000, 0.500), v2f(1.000, 0.625)),
            rectf(v2f(0.000, 0.875), v2f(1.000, 1.000)),
        };

        appendCuboid(bar_x1, {tile_nocrack}, cur_node.lighting, 0xff, &xrailuv);
        appendCuboid(bar_x2, {tile_nocrack}, cur_node.lighting, 0xff, &xrailuv);
	}

	// Check +Z neighbor
	p2 = cur_node.p;
	p2.Z++;
	n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
	f2 = &nodedef->get(n2);
	
	if (f2->drawtype == NDT_FENCELIKE) {
		static const aabbf bar_z1(-bar_rad,  BS/4 - bar_rad, BS/2 - bar_len,
									bar_rad,  BS/4 + bar_rad, BS/2 + bar_len);
		static const aabbf bar_z2(-bar_rad, -BS/4 - bar_rad, BS/2 - bar_len,
									bar_rad, -BS/4 + bar_rad, BS/2 + bar_len);
        static const std::array<rectf, 6> zrailuv = {
            rectf(v2f(0.1875, 0.0625), v2f(0.3125, 0.3125)),
            rectf(v2f(0.2500, 0.0625), v2f(0.3750, 0.3125)),
            rectf(v2f(0.0000, 0.5625), v2f(1.0000, 0.6875)),
            rectf(v2f(0.0000, 0.3750), v2f(1.0000, 0.5000)),
            rectf(v2f(0.3750, 0.3750), v2f(0.5000, 0.5000)),
            rectf(v2f(0.6250, 0.6250), v2f(0.7500, 0.7500))
        };

        appendCuboid(bar_z1, {tile_nocrack}, cur_node.lighting, 0xff, &zrailuv);
        appendCuboid(bar_z2, {tile_nocrack}, cur_node.lighting, 0xff, &zrailuv);
	}
}

void MeshGenerator::appendRaillikeNode()
{
	cur_rail.raillike_group = cur_node.f->getGroup(raillike_groupname);

	static const v3s16 rail_direction[4] = {
		v3s16( 0, 0,  1), v3s16( 0, 0, -1),
		v3s16(-1, 0,  0), v3s16( 1, 0,  0),
	};
	static const int rail_slope_angle[4] = {0, 180, 90, -90};

	enum RailTile { straight, curved, junction, cross };
	struct RailDesc { int tile_index; int angle; };
	static const RailDesc rail_kinds[16] = {
		{straight,   0}, {straight,   0}, {straight,   0}, {straight,   0},
		{straight,  90}, {  curved, 180}, {  curved, 270}, {junction, 180},
		{straight,  90}, {  curved,  90}, {  curved,   0}, {junction,   0},
		{straight,  90}, {junction,  90}, {junction, 270}, {   cross,   0},
	};

	int code = 0;
	int angle;
	int tile_index;
	bool sloped = false;
	
	for (int dir = 0; dir < 4; dir++) {
		bool rail_above = isSameRail(rail_direction[dir] + v3s16(0, 1, 0));
		if (rail_above) {
			sloped = true;
			angle = rail_slope_angle[dir];
		}
		if (rail_above ||
				isSameRail(rail_direction[dir]) ||
				isSameRail(rail_direction[dir] + v3s16(0, -1, 0)))
			code |= 1 << dir;
	}

	if (sloped) {
		tile_index = straight;
	} else {
		tile_index = rail_kinds[code].tile_index;
		angle = rail_kinds[code].angle;
	}

	TileSpec tile;
	useTile(&tile, tile_index, 0, MATERIAL_FLAG_BACKFACE_CULLING);

	static const float offset = BS / 64;
	static const float size   = BS / 2;
	float y2 = sloped ? size : -size;
	
	std::array<v3f, 4> vertices = {
		v3f(-size,    y2 + offset,  size),
		v3f( size,    y2 + offset,  size),
		v3f( size, -size + offset, -size),
		v3f(-size, -size + offset, -size),
	};
	
	if (angle)
		for (v3f &vertex : vertices)
			vertex.rotateXZBy(angle);

	std::array<v3f, 4> normals = {v3f(0,1,0), v3f(0,1,0), v3f(0,1,0), v3f(0,1,0)};

    appendQuad(vertices, normals, tile);
}

void MeshGenerator::appendNodeboxNode()
{
	static const v3s16 nodebox_tile_dirs[6] = {
		v3s16(0, 1, 0), v3s16(0, -1, 0),
		v3s16(1, 0, 0), v3s16(-1, 0, 0),
		v3s16(0, 0, 1), v3s16(0, 0, -1)
	};

	std::array<TileSpec, 6> tiles;
    for (int face = 0; face < 6; face++)
		getTile(nodebox_tile_dirs[face], &tiles[face]);

	bool param2_is_rotation =
		cur_node.f->param_type_2 == CPT2_COLORED_FACEDIR ||
		cur_node.f->param_type_2 == CPT2_COLORED_WALLMOUNTED ||
		cur_node.f->param_type_2 == CPT2_COLORED_4DIR ||
		cur_node.f->param_type_2 == CPT2_FACEDIR ||
		cur_node.f->param_type_2 == CPT2_WALLMOUNTED ||
		cur_node.f->param_type_2 == CPT2_4DIR;

	bool param2_is_level = cur_node.f->param_type_2 == CPT2_LEVELED;

	// Determine neighbors
	u8 neighbors_set = 0;
	u8 solid_neighbors = 0;
	u8 sametype_neighbors = 0;
	
	static const v3s16 nodebox_connection_dirs[6] = {
		v3s16( 0,  1,  0), v3s16( 0, -1,  0),
		v3s16( 0,  0, -1), v3s16(-1,  0,  0),
		v3s16( 0,  0,  1), v3s16( 1,  0,  0),
	};

	for (int dir = 0; dir != 6; dir++) {
		u8 flag = 1 << dir;
		v3s16 p2 = blockpos_nodes + cur_node.p + nodebox_tile_dirs[dir];
		MapNode n2 = data->m_vmanip.getNodeNoEx(p2);

		if (n2.param0 == cur_node.n.param0 &&
				(!param2_is_rotation || cur_node.n.param2 == n2.param2) &&
				(!param2_is_level || cur_node.n.param2 <= n2.param2))
			sametype_neighbors |= flag;

		if (nodedef->get(n2).drawtype == NDT_NORMAL)
			solid_neighbors |= flag;

		if (cur_node.f->node_box.type == NODEBOX_CONNECTED) {
			p2 = blockpos_nodes + cur_node.p + nodebox_connection_dirs[dir];
			n2 = data->m_vmanip.getNodeNoEx(p2);
			if (nodedef->nodeboxConnects(cur_node.n, n2, flag))
				neighbors_set |= flag;
		}
	}

	std::vector<aabbf> boxes;
	cur_node.n.getNodeBoxes(nodedef, &boxes, neighbors_set);

	// Box splitting for transparent nodes (same as original)
	bool isTransparent = false;
	for (const TileSpec &tile : tiles) {
        if (tile[0].material_flags & MATERIAL_FLAG_TRANSPARENT) {
			isTransparent = true;
			break;
		}
	}

	if (isTransparent) {
		std::vector<float> sections;
		sections.reserve(8 + 2 * boxes.size());

		for (u8 axis = 0; axis < 3; axis++) {
			if (axis == 0) {
				for (float s = -3.5f * BS; s < 4.0f * BS; s += 1.0f * BS)
					sections.push_back(s);
			} else {
				sections.resize(8);
			}

			for (size_t i = 0; i < boxes.size(); i++) {
				sections.push_back(std::floor(boxes[i].MinEdge[axis] * 1E3) * 1E-3);
				sections.push_back(std::floor(boxes[i].MaxEdge[axis] * 1E3) * 1E-3);
			}

			for (size_t i = 0; i < boxes.size() && i < 100; i++) {
				aabbf *box = &boxes[i];
				for (float section : sections) {
					if (box->MinEdge[axis] < section && box->MaxEdge[axis] > section) {
						aabbf copy(*box);
						copy.MinEdge[axis] = section;
						box->MaxEdge[axis] = section;
						boxes.push_back(copy);
						box = &boxes[i];
					}
				}
			}
		}
	}

	// Draw all boxes
	for (auto &box : boxes) {
		u8 mask = getNodeBoxMask(box, solid_neighbors, sametype_neighbors);
        appendCuboid(box, tiles, cur_node.lighting, mask);
	}
}

void MeshGenerator::appendMeshNode()
{
	u8 facedir = 0;
	int degrotate = 0;

	if (cur_node.f->param_type_2 == CPT2_FACEDIR ||
			cur_node.f->param_type_2 == CPT2_COLORED_FACEDIR ||
			cur_node.f->param_type_2 == CPT2_4DIR ||
			cur_node.f->param_type_2 == CPT2_COLORED_4DIR) {
		facedir = cur_node.n.getFaceDir(nodedef);
	} else if (cur_node.f->param_type_2 == CPT2_WALLMOUNTED ||
			cur_node.f->param_type_2 == CPT2_COLORED_WALLMOUNTED) {
		facedir = cur_node.n.getWallMounted(nodedef);
		facedir = wallmounted_to_facedir[facedir];
	} else if (cur_node.f->param_type_2 == CPT2_DEGROTATE ||
			cur_node.f->param_type_2 == CPT2_COLORED_DEGROTATE) {
		degrotate = cur_node.n.getDegRotate(nodedef);
	}

	if (cur_node.f->mesh_ptr) {
		auto layered_mesh = cur_node.f->mesh_ptr->getMesh();

        MeshBuffer *buf = nullptr;
        if (!first_stage) {
            buf = layered_mesh->getBuffer(0)->copy();

            bool modified = true;
            if (facedir)
                MeshOperations::rotateMeshBy6dFacedir(buf, facedir);
            else if (degrotate)
                MeshOperations::rotateMeshXZby(buf, 1.5f * degrotate);
            else
                modified = false;

            if (modified)
                buf->recalculateBoundingBox();
        }

        auto buffer_layers = layered_mesh->getBufferLayers(layered_mesh->getBuffer(0));
        u8 k = 0;
        for (const auto &buffer_layer : buffer_layers) {
            TileSpec tile;
            useTile(&tile, MYMIN(k, 5));

            const auto &mesh_part = buffer_layer.second;
            if (first_stage)
                mergeMeshPart(tile[0], SELECT_VERTEXTYPE(tile[0]), mesh_part.vertex_count, mesh_part.count);
            else {
                auto collector_buf = findBuffer(tile[0], NodeVType, mesh_part.vertex_count, mesh_part.count);

                for (u32 j = mesh_part.vertex_offset; j < mesh_part.vertex_offset + mesh_part.vertex_count; j++) {
                    v3f pos = svtGetPos(buf, j);
                    pos += cur_node.origin;

                    img::color8 color = svtGetColor(buf, j);
                    v3f normal = svtGetNormal(buf, j);
                    color = calculateVertexColor(pos, normal,
                         cur_node.lighting, cur_node.f->light_source);

                    Batcher3D::vertex(collector_buf, pos, color, normal, svtGetUV(buf, j));
                }

                for (u32 j = mesh_part.offset; j < mesh_part.offset + mesh_part.count; j++)
                    Batcher3D::index(collector_buf, buf->getIndexAt(j));
            }

            ++k;
        }

        if (buf)
            delete buf;
	} else {
		warningstream << "drawMeshNode(): missing mesh" << std::endl;
		return;
	}
}

// ============================================================================
// HELPER FUNCTIONS (mostly same as original)
// ============================================================================

void MeshGenerator::appendPlantlikeQuad(const TileSpec &tile,
		float rotation, float quad_offset, bool offset_top_only)
{
	const f32 scale = cur_plant.scale;
	std::array<v3f, 4> vertices = {
		v3f(-scale, -BS/2 + 2.0*scale*cur_plant.plant_height, 0),
		v3f( scale, -BS/2 + 2.0*scale*cur_plant.plant_height, 0),
		v3f( scale, -BS/2, 0),
		v3f(-scale, -BS/2, 0),
	};
	
	if (cur_plant.random_offset_Y) {
		PseudoRandom yrng(cur_plant.face_num++
				| cur_node.p.X << 16
				| cur_node.p.Z << 8
				| cur_node.p.Y << 24);
		cur_plant.offset.Y = -BS * ((yrng.next() % 16 / 16.0) * 0.125);
	}
	
	int offset_count = offset_top_only ? 2 : 4;
	for (int i = 0; i < offset_count; i++)
		vertices[i].Z += quad_offset;

	for (v3f &vertex : vertices) {
		vertex.rotateXZBy(rotation + cur_plant.rotate_degree);
		vertex += cur_plant.offset;
	}

	u8 wall = cur_node.n.getWallMounted(nodedef);
	if (wall != DWM_YN) {
		for (v3f &vertex : vertices) {
			switch (wall) {
			case DWM_YP:
				vertex.rotateYZBy(180);
				vertex.rotateXZBy(180);
				break;
			case DWM_XP: vertex.rotateXYBy(90); break;
			case DWM_XN:
				vertex.rotateXYBy(-90);
				vertex.rotateYZBy(180);
				break;
			case DWM_ZP:
				vertex.rotateYZBy(-90);
				vertex.rotateXYBy(90);
				break;
			case DWM_ZN:
				vertex.rotateYZBy(90);
				vertex.rotateXYBy(90);
				break;
			}
		}
	}

	// Calculate colors with interpolation
	std::array<v3f, 4> normals = {v3f(0,0,-1), v3f(0,0,-1), v3f(0,0,-1), v3f(0,0,-1)};

    appendQuad(vertices, normals, tile);
}

void MeshGenerator::appendPlantlike(const TileSpec &tile, bool is_rooted)
{
	cur_plant.draw_style = PLANT_STYLE_CROSS;
	cur_plant.offset = v3f(0, 0, 0);
	cur_plant.scale = BS / 2 * cur_node.f->visual_scale;
	cur_plant.rotate_degree = 0.0f;
	cur_plant.random_offset_Y = false;
	cur_plant.face_num = 0;
	cur_plant.plant_height = 1.0;

	switch (cur_node.f->param_type_2) {
	case CPT2_MESHOPTIONS:
		cur_plant.draw_style = PlantlikeStyle(cur_node.n.param2 & MO_MASK_STYLE);
		if (cur_node.n.param2 & MO_BIT_SCALE_SQRT2)
			cur_plant.scale *= 1.41421;
		if (cur_node.n.param2 & MO_BIT_RANDOM_OFFSET) {
			PseudoRandom rng(cur_node.p.X << 8 | cur_node.p.Z | cur_node.p.Y << 16);
			cur_plant.offset.X = BS * ((rng.next() % 16 / 16.0) * 0.29 - 0.145);
			cur_plant.offset.Z = BS * ((rng.next() % 16 / 16.0) * 0.29 - 0.145);
		}
		if (cur_node.n.param2 & MO_BIT_RANDOM_OFFSET_Y)
			cur_plant.random_offset_Y = true;
		break;
	case CPT2_DEGROTATE:
	case CPT2_COLORED_DEGROTATE:
		cur_plant.rotate_degree = 1.5f * cur_node.n.getDegRotate(nodedef);
		break;
	case CPT2_LEVELED:
		cur_plant.plant_height = cur_node.n.param2 / 16.0;
		break;
	default:
		break;
	}

	if (is_rooted) {
		u8 wall = cur_node.n.getWallMounted(nodedef);
		switch (wall) {
		case DWM_YP:
			cur_plant.offset.Y += BS*2;
			break;
		case DWM_XN:
		case DWM_XP:
		case DWM_ZN:
		case DWM_ZP:
			cur_plant.offset.X += -BS;
			cur_plant.offset.Y +=  BS;
			break;
		}
	}

	switch (cur_plant.draw_style) {
	case PLANT_STYLE_CROSS:
        appendPlantlikeQuad(tile, 46);
        appendPlantlikeQuad(tile, -44);
		break;
	case PLANT_STYLE_CROSS2:
        appendPlantlikeQuad(tile, 91);
        appendPlantlikeQuad(tile, 1);
		break;
	case PLANT_STYLE_STAR:
        appendPlantlikeQuad(tile, 121);
        appendPlantlikeQuad(tile, 241);
        appendPlantlikeQuad(tile, 1);
		break;
	case PLANT_STYLE_HASH:
        appendPlantlikeQuad(tile,   1, BS/4);
        appendPlantlikeQuad(tile,  91, BS/4);
        appendPlantlikeQuad(tile, 181, BS/4);
        appendPlantlikeQuad(tile, 271, BS/4);
		break;
	case PLANT_STYLE_HASH2:
        appendPlantlikeQuad(tile,   1, -BS/2, true);
        appendPlantlikeQuad(tile,  91, -BS/2, true);
        appendPlantlikeQuad(tile, 181, -BS/2, true);
        appendPlantlikeQuad(tile, 271, -BS/2, true);
		break;
	}
}

void MeshGenerator::appendFirelikeQuad(const TileSpec &tile,
		float rotation, float opening_angle, float offset_h, float offset_v)
{
	const f32 scale = BS / 2 * cur_node.f->visual_scale;
	std::array<v3f, 4> vertices = {
		v3f(-scale, -BS/2 + scale*2, 0),
		v3f( scale, -BS/2 + scale*2, 0),
		v3f( scale, -BS/2, 0),
		v3f(-scale, -BS/2, 0),
	};

	for (v3f &vertex : vertices) {
		vertex.rotateYZBy(opening_angle);
		vertex.Z += offset_h;
		vertex.rotateXZBy(rotation);
		vertex.Y += offset_v;
	}

	std::array<v3f, 4> normals = {v3f(0,0,-1), v3f(0,0,-1), v3f(0,0,-1), v3f(0,0,-1)};

    appendQuad(vertices, normals, tile);
}

// Liquid functions (keep as-is, already use unified lighting via cur_node.lighting)

void MeshGenerator::prepareLiquidNodeDrawing()
{
	getSpecialTile(0, &cur_liquid.tile_top);
	getSpecialTile(1, &cur_liquid.tile);

	MapNode ntop    = data->m_vmanip.getNodeNoEx(blockpos_nodes + cur_node.p + v3s16(0,  1, 0));
	MapNode nbottom = data->m_vmanip.getNodeNoEx(blockpos_nodes + cur_node.p + v3s16(0, -1, 0));
	
	cur_liquid.c_flowing = cur_node.f->liquid_alternative_flowing_id;
	cur_liquid.c_source = cur_node.f->liquid_alternative_source_id;
	cur_liquid.top_is_same_liquid = (ntop.getContent() == cur_liquid.c_flowing)
			|| (ntop.getContent() == cur_liquid.c_source);
	cur_liquid.draw_bottom = (nbottom.getContent() != cur_liquid.c_flowing)
			&& (nbottom.getContent() != cur_liquid.c_source);
	
	if (cur_liquid.draw_bottom) {
		const ContentFeatures &f2 = nodedef->get(nbottom.getContent());
		if (f2.solidness > 1)
			cur_liquid.draw_bottom = false;
	}
}

void MeshGenerator::getLiquidNeighborhood()
{
	u8 range = rangelim(nodedef->get(cur_liquid.c_flowing).liquid_range, 1, 8);

	for (int w = -1; w <= 1; w++)
	for (int u = -1; u <= 1; u++) {
		LiquidData::NeighborData &neighbor = cur_liquid.neighbors[w + 1][u + 1];
		v3s16 p2 = cur_node.p + v3s16(u, 0, w);
		MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
		neighbor.content = n2.getContent();
		neighbor.level = -0.5f;
		neighbor.is_same_liquid = false;
		neighbor.top_is_same_liquid = false;

		if (neighbor.content == CONTENT_IGNORE)
			continue;

		if (neighbor.content == cur_liquid.c_source) {
			neighbor.is_same_liquid = true;
			neighbor.level = 0.5f;
		} else if (neighbor.content == cur_liquid.c_flowing) {
			neighbor.is_same_liquid = true;
			u8 liquid_level = (n2.param2 & LIQUID_LEVEL_MASK);
			if (liquid_level <= LIQUID_LEVEL_MAX + 1 - range)
				liquid_level = 0;
			else
				liquid_level -= (LIQUID_LEVEL_MAX + 1 - range);
			neighbor.level = (-0.5f + (liquid_level + 0.5f) / range);
		}

		p2.Y++;
		n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
		if (n2.getContent() == cur_liquid.c_source || n2.getContent() == cur_liquid.c_flowing)
			neighbor.top_is_same_liquid = true;
	}
}

void MeshGenerator::calculateCornerLevels()
{
	for (int k = 0; k < 2; k++)
	for (int i = 0; i < 2; i++)
		cur_liquid.corner_levels[k][i] = getCornerLevel(i, k);
}

f32 MeshGenerator::getCornerLevel(int i, int k) const
{
	float sum = 0;
	int count = 0;
	int air_count = 0;
	
	for (int dk = 0; dk < 2; dk++)
	for (int di = 0; di < 2; di++) {
		const LiquidData::NeighborData &neighbor_data = cur_liquid.neighbors[k + dk][i + di];
		content_t content = neighbor_data.content;

		if (neighbor_data.top_is_same_liquid)
			return 0.5f;

		if (content == cur_liquid.c_source)
			return 0.5f;

		if (content == cur_liquid.c_flowing) {
			sum += neighbor_data.level;
			count++;
		} else if (content == CONTENT_AIR) {
			air_count++;
		}
	}
	
	if (air_count >= 2)
		return -0.5f + 0.2f / BS;
	if (count > 0)
		return sum / count;
	return 0;
}

void MeshGenerator::appendLiquidSides()
{
	struct LiquidFaceDesc {
		v3s16 dir;
		v3s16 p[2];
	};
	struct UV {
		int u, v;
	};
	static const LiquidFaceDesc liquid_base_faces[4] = {
		{v3s16( 1, 0,  0), {v3s16(1, 0, 1), v3s16(1, 0, 0)}},
		{v3s16(-1, 0,  0), {v3s16(0, 0, 0), v3s16(0, 0, 1)}},
		{v3s16( 0, 0,  1), {v3s16(0, 0, 1), v3s16(1, 0, 1)}},
		{v3s16( 0, 0, -1), {v3s16(1, 0, 0), v3s16(0, 0, 0)}},
	};
	static const UV liquid_base_vertices[4] = {
		{0, 1}, {1, 1}, {1, 0}, {0, 0}
	};

	for (const auto &face : liquid_base_faces) {
		const LiquidData::NeighborData &neighbor = cur_liquid.neighbors[face.dir.Z + 1][face.dir.X + 1];

		if (neighbor.is_same_liquid) {
			if (!cur_liquid.top_is_same_liquid)
				continue;
			if (neighbor.top_is_same_liquid)
				continue;
		}

		const ContentFeatures &neighbor_features = nodedef->get(neighbor.content);
		if (neighbor_features.solidness == 2)
			continue;

		std::array<v3f, 4> positions;
		std::array<v3f, 4> normals;
		rectf uv;
		
		for (int j = 0; j < 4; j++) {
			const UV &vertex = liquid_base_vertices[j];
			const v3s16 &base = face.p[vertex.u];
			float v = vertex.v;

			v3f pos;
			pos.X = (base.X - 0.5f) * BS;
			pos.Z = (base.Z - 0.5f) * BS;
			if (vertex.v) {
				pos.Y = (neighbor.is_same_liquid ? cur_liquid.corner_levels[base.Z][base.X] : -0.5f) * BS;
			} else if (cur_liquid.top_is_same_liquid) {
				pos.Y = 0.5f * BS;
			} else {
				pos.Y = cur_liquid.corner_levels[base.Z][base.X] * BS;
				v += 0.5f - cur_liquid.corner_levels[base.Z][base.X];
			}

			positions[j] = pos;
			normals[j] = v3f(face.dir.X, face.dir.Y, face.dir.Z);

			if (j == 1)
				uv.ULC = v2f(vertex.u, v);
			else if (j == 3)
				uv.LRC = v2f(vertex.u, v);
		}

        appendQuad(positions, normals, cur_liquid.tile, &uv);
	}
}

void MeshGenerator::appendLiquidTop()
{
	static const int corner_resolve[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};

	std::array<v3f, 4> positions = {
		v3f(-BS/2, 0, BS/2),
		v3f(BS/2, 0, BS/2),
		v3f(BS/2, 0, -BS/2),
		v3f(-BS/2, 0, -BS/2)
	};
	std::array<v3f, 4> normals;
	rectf uv(v2f(0,1), v2f(1,0));

	for (int i = 0; i < 4; i++) {
		int u = corner_resolve[i][0];
		int w = corner_resolve[i][1];

		if (data->m_enable_water_reflections) {
			int x = positions[i].X > 0;
			int z = positions[i].Z > 0;

			f32 dx = 0.5f * (cur_liquid.neighbors[z][x].level - cur_liquid.neighbors[z][x+1].level +
				cur_liquid.neighbors[z+1][x].level - cur_liquid.neighbors[z+1][x+1].level);
			f32 dz = 0.5f * (cur_liquid.neighbors[z][x].level - cur_liquid.neighbors[z+1][x].level +
				cur_liquid.neighbors[z][x+1].level - cur_liquid.neighbors[z+1][x+1].level);

			normals[i] = v3f(dx, 1., dz).normalize();
		} else {
			normals[i] = v3f(0, 1, 0);
		}

		positions[i].Y += cur_liquid.corner_levels[w][u] * BS;
	}

	// UV rotation for flow direction
	f32 dz = (cur_liquid.corner_levels[0][0] + cur_liquid.corner_levels[0][1]) -
			 (cur_liquid.corner_levels[1][0] + cur_liquid.corner_levels[1][1]);
	f32 dx = (cur_liquid.corner_levels[0][0] + cur_liquid.corner_levels[1][0]) -
			 (cur_liquid.corner_levels[0][1] + cur_liquid.corner_levels[1][1]);
	
	v2f tcoord_center(0.5, 0.5);
	v2f tcoord_translate(blockpos_nodes.Z + cur_node.p.Z,
			blockpos_nodes.X + cur_node.p.X);
	v2f dir = v2f(dx, dz).normalize();
	if (dir == v2f{0.0f, 0.0f})
		dir = v2f{1.0f, 0.0f};

	tcoord_translate = v2f(
		dir.X * tcoord_translate.X - dir.Y * tcoord_translate.Y,
		dir.Y * tcoord_translate.X + dir.X * tcoord_translate.Y);

	tcoord_translate.X -= floor(tcoord_translate.X);
	tcoord_translate.Y -= floor(tcoord_translate.Y);

	for (u32 i = 0; i < 4; i++) {
		if (i == 0 || i == 2) {
			v2f &cur_uv = i == 0 ? uv.ULC : uv.LRC;
			cur_uv -= tcoord_center;
			cur_uv.X = dir.X * cur_uv.X - dir.Y * cur_uv.Y;
			cur_uv.Y = dir.Y * cur_uv.X + dir.X * cur_uv.Y;
			cur_uv += tcoord_center;
			cur_uv += tcoord_translate;
		}
	}

	std::swap(uv.ULC, uv.LRC);

    appendQuad(positions, normals, cur_liquid.tile_top, &uv);
}

void MeshGenerator::appendLiquidBottom()
{
	std::array<v3f, 4> positions = {
		v3f(-BS/2, -BS/2, -BS/2),
		v3f(BS/2, -BS/2, -BS/2),
		v3f(BS/2, -BS/2,  BS/2),
		v3f(-BS/2, -BS/2,  BS/2)
	};
	std::array<v3f, 4> normals = {v3f(0,-1,0), v3f(0,-1,0), v3f(0,-1,0), v3f(0,-1,0)};
	rectf uv(v2f(0,1), v2f(1,0));

    appendQuad(positions, normals, cur_liquid.tile_top, &uv);
}

// Node tile functions (same as original)

void MeshGenerator::getNodeTileN(MapNode mn, const v3s16 &p, u8 tileindex, MeshMakeData *data, TileSpec &tile)
{
	const NodeDefManager *ndef = data->m_nodedef;
	const ContentFeatures &f = ndef->get(mn);
	tile = f.tiles[tileindex];
	
    for (auto &layer : tile) {
        if (!layer.tile_ref)
			continue;
        if (!(layer.material_flags & MATERIAL_FLAG_HARDWARE_COLORIZED))
            mn.getColor(f, &(layer.color));
	}
}

void MeshGenerator::getNodeTile(MapNode mn, const v3s16 &p, const v3s16 &dir, MeshMakeData *data, TileSpec &tile)
{
	const NodeDefManager *ndef = data->m_nodedef;

	assert(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z <= 1);

	u8 dir_i = (dir.X + 2 * dir.Y + 3 * dir.Z) & 7;
	u8 facedir = mn.getFaceDir(ndef, true);

	static constexpr auto
		R0 = TileRotation::RNone,
		R1 = TileRotation::R90,
		R2 = TileRotation::R180,
		R3 = TileRotation::R270;
	
	static const struct {
		u8 tile;
		TileRotation rotation;
	} dir_to_tile[24][8] = {
		{{0,R0},  {2,R0}, {0,R0}, {4,R0},  {0,R0},  {5,R0}, {1,R0}, {3,R0}},
		{{0,R0},  {4,R0}, {0,R3}, {3,R0},  {0,R0},  {2,R0}, {1,R1}, {5,R0}},
		{{0,R0},  {3,R0}, {0,R2}, {5,R0},  {0,R0},  {4,R0}, {1,R2}, {2,R0}},
		{{0,R0},  {5,R0}, {0,R1}, {2,R0},  {0,R0},  {3,R0}, {1,R3}, {4,R0}},

		{{0,R0},  {2,R3}, {5,R0}, {0,R2},  {0,R0},  {1,R0}, {4,R2}, {3,R1}},
		{{0,R0},  {4,R3}, {2,R0}, {0,R1},  {0,R0},  {1,R1}, {3,R2}, {5,R1}},
		{{0,R0},  {3,R3}, {4,R0}, {0,R0},  {0,R0},  {1,R2}, {5,R2}, {2,R1}},
		{{0,R0},  {5,R3}, {3,R0}, {0,R3},  {0,R0},  {1,R3}, {2,R2}, {4,R1}},

		{{0,R0},  {2,R1}, {4,R2}, {1,R2},  {0,R0},  {0,R0}, {5,R0}, {3,R3}},
		{{0,R0},  {4,R1}, {3,R2}, {1,R3},  {0,R0},  {0,R3}, {2,R0}, {5,R3}},
		{{0,R0},  {3,R1}, {5,R2}, {1,R0},  {0,R0},  {0,R2}, {4,R0}, {2,R3}},
		{{0,R0},  {5,R1}, {2,R2}, {1,R1},  {0,R0},  {0,R1}, {3,R0}, {4,R3}},

		{{0,R0},  {0,R3}, {3,R3}, {4,R1},  {0,R0},  {5,R3}, {2,R3}, {1,R3}},
		{{0,R0},  {0,R2}, {5,R3}, {3,R1},  {0,R0},  {2,R3}, {4,R3}, {1,R0}},
		{{0,R0},  {0,R1}, {2,R3}, {5,R1},  {0,R0},  {4,R3}, {3,R3}, {1,R1}},
		{{0,R0},  {0,R0}, {4,R3}, {2,R1},  {0,R0},  {3,R3}, {5,R3}, {1,R2}},

		{{0,R0},  {1,R1}, {2,R1}, {4,R3},  {0,R0},  {5,R1}, {3,R1}, {0,R1}},
		{{0,R0},  {1,R2}, {4,R1}, {3,R3},  {0,R0},  {2,R1}, {5,R1}, {0,R0}},
		{{0,R0},  {1,R3}, {3,R1}, {5,R3},  {0,R0},  {4,R1}, {2,R1}, {0,R3}},
		{{0,R0},  {1,R0}, {5,R1}, {2,R3},  {0,R0},  {3,R1}, {4,R1}, {0,R2}},

		{{0,R0},  {3,R2}, {1,R2}, {4,R2},  {0,R0},  {5,R2}, {0,R2}, {2,R2}},
		{{0,R0},  {5,R2}, {1,R3}, {3,R2},  {0,R0},  {2,R2}, {0,R1}, {4,R2}},
		{{0,R0},  {2,R2}, {1,R0}, {5,R2},  {0,R0},  {4,R2}, {0,R0}, {3,R2}},
		{{0,R0},  {4,R2}, {1,R1}, {2,R2},  {0,R0},  {3,R2}, {0,R3}, {5,R2}}
	};
	
	getNodeTileN(mn, p, dir_to_tile[facedir][dir_i].tile, data, tile);

    for (auto &layer : tile)
        layer.rotation = layer.material_flags & MATERIAL_FLAG_WORLD_ALIGNED ? TileRotation::RNone : dir_to_tile[facedir][dir_i].rotation;
}

bool MeshGenerator::isSameRail(v3s16 dir)
{
	MapNode node2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + cur_node.p + dir);
	if (node2.getContent() == cur_node.n.getContent())
		return true;
	const ContentFeatures &def2 = nodedef->get(node2);
	return ((def2.drawtype == NDT_RAILLIKE) &&
		(def2.getGroup(raillike_groupname) == cur_rail.raillike_group));
}

u8 MeshGenerator::getNodeBoxMask(aabbf box, u8 solid_neighbors, u8 sametype_neighbors) const
{
	const f32 NODE_BOUNDARY = 0.5 * BS;

	if (box.MaxEdge.X > NODE_BOUNDARY ||
			box.MinEdge.X < -NODE_BOUNDARY ||
			box.MaxEdge.Y >  NODE_BOUNDARY ||
			box.MinEdge.Y < -NODE_BOUNDARY ||
			box.MaxEdge.Z >  NODE_BOUNDARY ||
			box.MinEdge.Z < -NODE_BOUNDARY)
		return 0;

	u8 solid_mask =
			(box.MaxEdge.Y == NODE_BOUNDARY  ?  1 : 0) |
			(box.MinEdge.Y == -NODE_BOUNDARY ?  2 : 0) |
			(box.MaxEdge.X == NODE_BOUNDARY  ?  4 : 0) |
			(box.MinEdge.X == -NODE_BOUNDARY ?  8 : 0) |
			(box.MaxEdge.Z == NODE_BOUNDARY  ? 16 : 0) |
			(box.MinEdge.Z == -NODE_BOUNDARY ? 32 : 0);

	u8 sametype_mask = 0;
	if (cur_node.f->alpha == AlphaMode::ALPHAMODE_OPAQUE) {
		sametype_mask =
				((solid_mask & 3) == 3 ? 3 : 0) |
				((solid_mask & 12) == 12 ? 12 : 0) |
				((solid_mask & 48) == 48 ? 48 : 0);
	}

	return (solid_mask & solid_neighbors) | (sametype_mask & sametype_neighbors);
}

void MeshGenerator::errorUnknownDrawtype()
{
	infostream << "Got drawtype " << cur_node.f->drawtype << std::endl;
	FATAL_ERROR("Unknown drawtype");
}

void MeshGenerator::appendNode()
{
	if (cur_node.f->drawtype == NDT_AIRLIKE)
		return;

    cur_node.origin = intToFloat(cur_node.p, BS);

    calculateNodeLighting(
        cur_node.lighting,
        blockpos_nodes + cur_node.p,
        data->m_smooth_lighting,
        data
    );

	switch (cur_node.f->drawtype) {
	case NDT_LIQUID:
    case NDT_NORMAL:            appendSolidNode(); break;
    case NDT_FLOWINGLIQUID:     appendLiquidNode(); break;
    case NDT_GLASSLIKE:         appendGlasslikeNode(); break;
    case NDT_GLASSLIKE_FRAMED:  appendGlasslikeFramedNode(); break;
    case NDT_ALLFACES:          appendAllfacesNode(); break;
    case NDT_TORCHLIKE:         appendTorchlikeNode(); break;
    case NDT_SIGNLIKE:          appendSignlikeNode(); break;
    case NDT_PLANTLIKE:         appendPlantlikeNode(); break;
    case NDT_PLANTLIKE_ROOTED:  appendPlantlikeRootedNode(); break;
    case NDT_FIRELIKE:          appendFirelikeNode(); break;
    case NDT_FENCELIKE:         appendFencelikeNode(); break;
    case NDT_RAILLIKE:          appendRaillikeNode(); break;
    case NDT_NODEBOX:           appendNodeboxNode(); break;
    case NDT_MESH:              appendMeshNode(); break;
	default:                    errorUnknownDrawtype(); break;
	}
}

void MeshGenerator::generate()
{
	ZoneScoped;

    if (!first_stage)
        return; // forbid the regenerating

    allocate();

    first_stage = false;

	for (cur_node.p.Z = 0; cur_node.p.Z < data->m_side_length; cur_node.p.Z++)
	for (cur_node.p.Y = 0; cur_node.p.Y < data->m_side_length; cur_node.p.Y++)
	for (cur_node.p.X = 0; cur_node.p.X < data->m_side_length; cur_node.p.X++) {
		cur_node.n = data->m_vmanip.getNodeNoEx(blockpos_nodes + cur_node.p);
		cur_node.f = &nodedef->get(cur_node.n);
        appendNode();
	}
}

void MeshGenerator::mergeMeshPart(const TileLayer &layer,
    render::VertexTypeDescriptor vType,
    u32 vertexCount, u32 indexCount)
{
    for (u8 i = 0; i < interim_buffers.size(); i++) {
        auto &buffer = interim_buffers.at(i).first;
        // The buffer is overfilled or vertex type is different from the required one
        if (((u64)buffer.vertexCount + vertexCount > (u64)T_MAX(u32)) ||
            (vType.Name != buffer.vertexType.Name))
            continue;

        buffer.vertexCount += vertexCount;
        buffer.indexCount += indexCount;

        auto &buf_layers = interim_buffers.at(i).second;
        auto found_buf_layer = std::find_if(buf_layers.begin(), buf_layers.end(),
            [layer] (const std::pair<TileLayer, InterimBufferLayer> &cur_buf_layer)
        { return layer == cur_buf_layer.first; });

        if (found_buf_layer == buf_layers.end()) {
            InterimBufferLayer buf_layer = {vertexCount, indexCount};
            buf_layers.emplace_back(layer, buf_layer);
        }
        else {
            auto &buf_layer = *found_buf_layer;
            buf_layer.second.vertexCount += vertexCount;
            buf_layer.second.indexCount += indexCount;
        }

        return;
    }

    InterimBuffer buffer = {vType, vertexCount, indexCount};
    InterimBufferLayers buf_layers;

    InterimBufferLayer buf_layer = {vertexCount, indexCount};
    buf_layers.emplace_back(layer, buf_layer);
    interim_buffers.emplace_back(buffer, buf_layers);
}

MeshBuffer *MeshGenerator::findBuffer(const TileLayer &layer,
    render::VertexTypeDescriptor vType,
    u32 vertexCount, u32 indexCount)
{
    for (u8 buf_i = 0; buf_i < collector->getBuffersCount(); buf_i++) {
        auto buffer = collector->getBuffer(buf_i);

        auto &buf_layers = interim_buffers.at(buf_i).second;
        auto found_buf_layer = std::find_if(buf_layers.begin(), buf_layers.end(),
            [layer] (const std::pair<TileLayer, InterimBufferLayer> &cur_buf_layer)
        { return layer == cur_buf_layer.first; });
        auto &buf_layer = found_buf_layer->second;

        if (buffer->getVertexType().Name != vType.Name || buf_layer.vertexCount == 0)
            continue;

        buffer->setVertexOffset(buf_layer.curVertexOffset);
        buffer->setIndexOffset(buf_layer.curIndexOffset);
        buf_layer.curVertexOffset += vertexCount;
        buf_layer.curIndexOffset += indexCount;

        return buffer;
    }

    return nullptr;
}

void MeshGenerator::allocate()
{
    for (cur_node.p.Z = 0; cur_node.p.Z < data->m_side_length; cur_node.p.Z++)
    for (cur_node.p.Y = 0; cur_node.p.Y < data->m_side_length; cur_node.p.Y++)
    for (cur_node.p.X = 0; cur_node.p.X < data->m_side_length; cur_node.p.X++) {
        cur_node.n = data->m_vmanip.getNodeNoEx(blockpos_nodes + cur_node.p);
        cur_node.f = &nodedef->get(cur_node.n);

        switch (cur_node.f->drawtype) {
        case NDT_AIRLIKE:           break;
        case NDT_LIQUID:
        case NDT_NORMAL:            appendSolidNode(); break;
        case NDT_FLOWINGLIQUID:     appendLiquidNode(); break;
        case NDT_GLASSLIKE:         appendGlasslikeNode(); break;
        case NDT_GLASSLIKE_FRAMED:  appendGlasslikeFramedNode(); break;
        case NDT_ALLFACES:          appendAllfacesNode(); break;
        case NDT_TORCHLIKE:         appendTorchlikeNode(); break;
        case NDT_SIGNLIKE:          appendSignlikeNode(); break;
        case NDT_PLANTLIKE:         appendPlantlikeNode(); break;
        case NDT_PLANTLIKE_ROOTED:  appendPlantlikeRootedNode(); break;
        case NDT_FIRELIKE:          appendFirelikeNode(); break;
        case NDT_FENCELIKE:         appendFencelikeNode(); break;
        case NDT_RAILLIKE:          appendRaillikeNode(); break;
        case NDT_NODEBOX:           appendNodeboxNode(); break;
        case NDT_MESH:              appendMeshNode(); break;
        default:                    errorUnknownDrawtype(); break;
        }
    }

    for (auto &buffer : interim_buffers) {
        auto mesh_buffer = new MeshBuffer(
            buffer.first.vertexCount, buffer.first.indexCount, true, buffer.first.vertexType,
            render::MeshUsage::STATIC, true);
        collector->addNewBuffer(mesh_buffer);

        u32 vertex_offset = 0;
        u32 index_offset = 0;

        for (auto &layer : buffer.second) {
            layer.second.curVertexOffset = vertex_offset;
            layer.second.curIndexOffset = index_offset;

            LayeredMeshPart mesh_part = {
                mesh_buffer, layer.first, layer.second.curIndexOffset, layer.second.indexCount
            };
            mesh_part.vertex_offset = layer.second.curVertexOffset;
            mesh_part.vertex_count = layer.second.vertexCount;
            collector->addNewLayer(mesh_buffer, layer.first, mesh_part);

            vertex_offset += layer.second.vertexCount;
            index_offset += layer.second.indexCount;
        }
    }
}

// Glasslike framed - keep as original for now (complex, needs full rewrite)
void MeshGenerator::appendGlasslikeFramedNode()
{
	// TODO: Implement with unified lighting
	// For now, call original implementation
	warningstream << "drawGlasslikeFramedNode() not yet implemented in unified generator" << std::endl;
}
