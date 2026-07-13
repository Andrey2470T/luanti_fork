#pragma once

#include "irr_v3d.h"
#include "mapblock.h"
#include "constants.h"
#include <queue>

class BlockLightFloodFill
{
	struct MapBlockLightInfo
	{
		MapBlock *block = nullptr;
		std::array<u16, MapBlock::nodecount> light = {};

		u16 getLight(v3s16 nodePos) const;
		video::SColor getLightColor(v3s16 nodePos) const;

		bool lightFalloff(v3s16 nodePos, video::SColor &color);
	};

	struct LightNodeEntry
	{
		v3s16 mapblockPos;
		v3s16 nodePos;
		video::SColor lightColor;
	};

	const NodeDefManager *m_nodedef;
	std::unordered_map<v3s16, MapBlockLightInfo> m_mapblocks_light;
	std::queue<LightNodeEntry> m_light_propagation_queue;
	std::queue<LightNodeEntry> m_light_removal_queue;

public:
	BlockLightFloodFill(const NodeDefManager *nodedef) : m_nodedef(nodedef) {}

	void addMapBlock(v3s16 blockpos, MapBlock *block);
	void removeMapBlock(v3s16 blockpos);

	u16 getLight(v3s16 nodePos) const;
	video::SColor getLightColor(v3s16 nodePos) const;
	static u16 maxLight(u16 light1, u16 light2);

	void updateFill();

private:
	void addLightNodesInQueue(MapBlock *block, bool propagate=true);
	void recurseFill(const LightNodeEntry &cur_lightnode, std::unordered_map<v3s16, bool> &passed_lightnodes);
};