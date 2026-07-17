#pragma once

#include "irr_v3d.h"
#include "mapblock.h"
#include "constants.h"
#include <queue>

enum class BlockLightChannel : u8
{
	RED,
	GREEN,
	BLUE
};

struct NodeLightChannel
{
	u8 ch : 5;

	bool lightFalloff(u8 propagateLight);
	operator u16() const { return ch; }
};

struct NodeLight
{
	NodeLightChannel r;
	NodeLightChannel g;
	NodeLightChannel b;

	bool lightFalloff(const NodeLight &propagateLight);

	operator u16() const
	{
		return (u16)r << 10 | (u16)g << 5 | (u16)b;
	}
};

class LightSourceGrid
{
	NodeLight source_color;
	u32 cube_size;
	std::vector<NodeLight> light;

public:
	LightSourceGrid(u32 r, u32 g, u32 b);

	bool operator==(const LightSourceGrid &other) const
	{
		return source_color == other.source_color;
	}

	const NodeLight &getLight(v3s16 absSourcePos, v3s16 absNodePos) const;
	std::vector<v3s16> getCoveringMapblocks(v3s16 absSourcePos) const;

	void propagateLight();

	size_t hash() const
	{
		return std::hash<u16>{}((u16)source_color);
	}
private:
	NodeLight &getLight(v3s16 relNodePos);
};

namespace std 
{
    template <>
    struct hash<LightSourceGrid> 
    {
        std::size_t operator()(const LightSourceGrid& n) const noexcept 
        {
            return n.hash();
        }
    };
}

class BlockLightPropagator
{
	struct LeveledNodeLight
	{
		NodeLight actualLight;

		void mixLight(
			BlockLightPropagator *propagator, v3s16 absNodePos,
			const std::vector<v3s16> &sources_positions);
	};

	struct MapBlockLightInfo
	{
		MapBlock *block = nullptr;
		std::vector<v3s16> light_sources = {}; // all light sources propagating over this mapblock
		std::array<LeveledNodeLight, MapBlock::nodecount> light = {};

		LeveledNodeLight &getLight(v3s16 pos)
		{
			u32 pos_n = pos.Z * MapBlock::zstride + pos.Y * MapBlock::ystride + pos.X;
			return light[pos_n];
		}
		const LeveledNodeLight &getLight(v3s16 pos) const
		{
			u32 pos_n = pos.Z * MapBlock::zstride + pos.Y * MapBlock::ystride + pos.X;
			return light[pos_n];
		}

		void propagateLight(BlockLightPropagator *propagator);
	};

	struct LightNodeEntry
	{
		v3s16 mapblockPos;
		v3s16 nodePos;
		NodeLight lightColor;
	};

	typedef std::unordered_map<v3s16, MapBlockLightInfo> MapBlockLightGrid;

	const NodeDefManager *nodedef;
	Map *map;
	MapBlockLightGrid mapblocks_light_grid;

	std::unordered_set<LightSourceGrid> light_sources_grids;
	std::unordered_map<v3s16, const LightSourceGrid*> light_sources;

	std::unordered_map<v3s16, std::pair<MapBlockLightInfo, bool>> pending_mapblocks_lights;

public:
	BlockLightPropagator(const NodeDefManager *_nodedef, Map *_map)
		: nodedef(_nodedef), map(_map) {}
	
	void addMapBlock(v3s16 blockpos, MapBlock *block);
	void removeMapBlocks(const std::vector<v3s16> &blocks_positions);

	u16 getLight(v3s16 nodePos);
	video::SColor getLightColor(v3s16 nodePos);
	static u16 maxLight(u16 light1, u16 light2);
	
	void propagateLight();
private:
	void scanForLightSources(MapBlock *block);
};