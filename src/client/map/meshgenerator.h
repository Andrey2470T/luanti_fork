// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2025 Unified Mesh Generator Refactor

#pragma once

#include "client/mesh/lighting.h"
#include "nodedef.h"

class MeshMakeData;
class LayeredMesh;

enum class QuadDiagonal {
	Diag02,
	Diag13,
};

/*
 * Unified Mesh Generator
 * 
 * Key differences from original MapblockMeshGenerator:
 * 1. Lighting is calculated ONCE per node using NodeLighting
 * 2. All drawtypes use the same lighting system
 * 3. Geometry generation is separated from lighting
 * 4. Much simpler, more readable code
 */
class MeshGenerator
{
public:
    MeshGenerator(MeshMakeData *input, LayeredMesh *output);
	void generate();

private:
	MeshMakeData *const data;
	LayeredMesh *const collector;
	const NodeDefManager *const nodedef;
	const v3s16 blockpos_nodes;

	// Current node being processed
	struct CurrentNode {
		v3s16 p;                    // Position relative to blockpos_nodes
		v3f origin;                 // Position in BS units
		MapNode n;                  // The node itself
		const ContentFeatures *f;   // Node features
		
		// UNIFIED LIGHTING - calculated once per node
		NodeLighting lighting;
		
		// Transform (for some drawtypes)
		v3f translation;
		v3f rotation;               // In degrees
		v3f scale;
	} cur_node;

	// Tile management
	void getTile(int index, TileSpec *tile_ret);
	void getTile(v3s16 direction, TileSpec *tile_ret);
	void getSpecialTile(int index, TileSpec *tile_ret);
	void useTile(TileSpec *tile_ret, int index = 0, u8 set_flags = 0,
		u8 reset_flags = 0, bool special = false);

	// Core drawing primitives
	void drawQuad(
		const std::array<v3f, 4> &positions,
		const std::array<v3f, 4> &normals,
		const std::array<img::color8, 4> &colors,
		const TileSpec &tile,
		const rectf *uv = nullptr
	);

	void drawCuboid(
		const aabbf &box,
		const std::array<TileSpec, 6> &tiles,
		const NodeLighting &lighting,
        u8 face_mask = 0xff,
        const std::array<rectf, 6> *uvs = nullptr
	);

	// Helper: calculate vertex colors from lighting
	std::array<img::color8, 4> calculateFaceColors(
		int face_index,
		const NodeLighting &lighting,
		u8 light_source = 0
	);

	img::color8 calculateVertexColor(
		const v3f &position,
		const v3f &normal,
		const NodeLighting &lighting,
		u8 light_source = 0
	);

	// Drawtype implementations (simplified with unified lighting)
	void drawSolidNode();
	void drawLiquidNode();
	void drawGlasslikeNode();
	void drawGlasslikeFramedNode();
	void drawAllfacesNode();
	void drawTorchlikeNode();
	void drawSignlikeNode();
	void drawPlantlikeNode();
	void drawPlantlikeRootedNode();
	void drawFirelikeNode();
	void drawFencelikeNode();
	void drawRaillikeNode();
	void drawNodeboxNode();
	void drawMeshNode();

	// Drawtype-specific helpers
	void drawPlantlikeQuad(const TileSpec &tile, float rotation,
		float quad_offset = 0, bool offset_top_only = false);
    void drawPlantlike(const TileSpec &tile, bool is_rooted);
	void drawFirelikeQuad(const TileSpec &tile, float rotation,
		float opening_angle, float offset_h, float offset_v = 0.0);

	// Liquid-specific
	struct LiquidData {
		struct NeighborData {
			f32 level;
			content_t content;
			bool is_same_liquid;
			bool top_is_same_liquid;
		};
		
		bool top_is_same_liquid;
		bool draw_bottom;
		TileSpec tile;
		TileSpec tile_top;
		content_t c_flowing;
		content_t c_source;
		NeighborData neighbors[3][3];
		f32 corner_levels[2][2];
	};
	LiquidData cur_liquid;

	void prepareLiquidNodeDrawing();
	void getLiquidNeighborhood();
	void calculateCornerLevels();
	f32 getCornerLevel(int i, int k) const;
	void drawLiquidSides();
	void drawLiquidTop();
	void drawLiquidBottom();

	// Raillike-specific
	static const std::string raillike_groupname;
	struct RaillikeData {
		int raillike_group;
	};
	RaillikeData cur_rail;
	bool isSameRail(v3s16 dir);

	// Plantlike-specific
	struct PlantlikeData {
		PlantlikeStyle draw_style;
		v3f offset;
		float scale;
		float rotate_degree;
		bool random_offset_Y;
		int face_num;
		float plant_height;
	};
	PlantlikeData cur_plant;

	// Nodebox helpers
	u8 getNodeBoxMask(aabbf box, u8 solid_neighbors, u8 sametype_neighbors) const;

	// Tile helpers
	void getNodeTileN(MapNode mn, const v3s16 &p, u8 tileindex, MeshMakeData *data, TileSpec &tile);
	void getNodeTile(MapNode mn, const v3s16 &p, const v3s16 &dir, MeshMakeData *data, TileSpec &tile);

	// Common
	void errorUnknownDrawtype();
	void drawNode();
};
