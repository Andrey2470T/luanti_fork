// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "client/mesh/lighting.h"
#include "nodedef.h"

class MeshMakeData;
class LayeredMesh;

enum class QuadDiagonal {
	Diag02,
	Diag13,
};

class MapblockMeshGenerator
{
public:
    MapblockMeshGenerator(MeshMakeData *input, LayeredMesh *output);
	void generate();

private:
	MeshMakeData *const data;
    LayeredMesh *const collector;

	const NodeDefManager *const nodedef;
	const v3s16 blockpos_nodes;

// current node
	struct {
		v3s16 p; // relative to blockpos_nodes
		v3f origin; // p in BS space
		MapNode n;
		const ContentFeatures *f;
		LightFrame lframe; // smooth lighting
        img::color8 lcolor; // unsmooth lighting
        v3f translation;
        v3f rotation; // in degrees
        v3f scale;
	} cur_node;

    void useTile(TileSpec *tile_ret, int index = 0, u8 set_flags = 0,
		u8 reset_flags = 0, bool special = false);
	void getTile(int index, TileSpec *tile_ret);
	void getTile(v3s16 direction, TileSpec *tile_ret);
    void getSpecialTile(int index, TileSpec *tile_ret);

    void prepareDrawing(const TileSpec &tile);
// face drawing
    void drawQuad(const TileSpec &tile, const std::array<v3f, 4> &coords,
        const std::array<v3f, 4> &normals={}, const rectf *uv = nullptr);

// cuboid drawing!
    void drawCuboid(const aabbf &box, const std::array<TileSpec, 6> &tiles, int tilecount, u8 mask=0xff,
        const std::array<rectf, 6> *uvs=nullptr);
    //void generateCuboidTextureCoords(const aabbf &box, f32 *coords);
    void drawAutoLightedCuboid(aabbf box, const TileSpec &tile, const std::array<rectf, 6> *uvs	= nullptr, u8 mask = 0);
    void drawAutoLightedCuboid(aabbf box, const std::array<TileSpec, 6> &tiles, int tile_count,
        const std::array<rectf, 6> *uvs = nullptr, u8 mask = 0);
    u8 getNodeBoxMask(aabbf box, u8 solid_neighbors, u8 sametype_neighbors) const;

// liquid-specific
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
        img::color8 color_top;
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

    // Retrieves the TileSpec of a face of a node
    // Adds MATERIAL_FLAG_CRACK if the node is cracked
    // TileSpec should be passed as reference due to the underlying TileFrame and its vector
    // TileFrame vector copy cost very much to client
    void getNodeTileN(MapNode mn, const v3s16 &p, u8 tileindex, MeshMakeData *data, TileSpec &tile);
    void getNodeTile(MapNode mn, const v3s16 &p, const v3s16 &dir, MeshMakeData *data, TileSpec &tile);

// raillike-specific
	// name of the group that enables connecting to raillike nodes of different kind
	static const std::string raillike_groupname;
	struct RaillikeData {
		int raillike_group;
	};
	RaillikeData cur_rail;
	bool isSameRail(v3s16 dir);

// plantlike-specific
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

	void drawPlantlikeQuad(const TileSpec &tile, float rotation, float quad_offset = 0,
		bool offset_top_only = false);
	void drawPlantlike(const TileSpec &tile, bool is_rooted = false);

// firelike-specific
	void drawFirelikeQuad(const TileSpec &tile, float rotation, float opening_angle,
		float offset_h, float offset_v = 0.0);

// drawtypes
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

// common
	void errorUnknownDrawtype();
	void drawNode();
};
