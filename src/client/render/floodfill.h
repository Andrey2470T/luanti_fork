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

class BlockLightPropagator
{
	struct MapBlockLightInfo
	{
		MapBlock *block = nullptr;
		std::array<NodeLight, MapBlock::nodecount> light = {};

		NodeLight &getLight(v3s16 pos)
		{
			u32 pos_n = pos.Z * MapBlock::zstride + pos.Y * MapBlock::ystride + pos.X;
			return light[pos_n];
		}
		const NodeLight &getLight(v3s16 pos) const
		{
			u32 pos_n = pos.Z * MapBlock::zstride + pos.Y * MapBlock::ystride + pos.X;
			return light[pos_n];
		}
	};

	struct LightNodeEntry
	{
		v3s16 mapblockPos;
		v3s16 nodePos;
		NodeLight lightColor;
	};

	typedef std::unordered_map<v3s16, MapBlockLightInfo> MapBlockLightGrid;
	typedef std::queue<LightNodeEntry> LightNodeEntryQueue;

	const NodeDefManager *nodedef;
	MapBlockLightGrid mapblocks_light_grid;
	LightNodeEntryQueue light_propagation_queue;

	std::mutex mapblocks_light_grid_mutex;

public:
	BlockLightPropagator(const NodeDefManager *_nodedef)
		: nodedef(_nodedef) {}
	
	void addMapBlock(v3s16 blockpos, MapBlock *block);
	void removeMapBlocks(const std::vector<v3s16> &blocks_positions);

	u16 getLight(v3s16 nodePos) const;
	video::SColor getLightColor(v3s16 nodePos) const;
	static u16 maxLight(u16 light1, u16 light2);
	
	void propagateLight();
private:
	void addLightNodesInQueue(MapBlock *block);
};