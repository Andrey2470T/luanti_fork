#include "floodfill.h"
#include "nodedef.h"
#include "util/directiontables.h"

u16 getLightFromColor(const video::SColor &color)
{
	u16 light = 0;
	light |= (u32)color.getRed() << 10;
	light |= (u32)color.getGreen() << 5;
	light |= (u32)color.getBlue();
	return light;
}

video::SColor getColorFromLight(u16 light)
{
	return video::SColor(255, light >> 10, light >> 5 & 0x1f, light & 0x1f);
}

u16 BlockLightFloodFill::MapBlockLightInfo::getLight(v3s16 nodePos)
{
	return light[nodePos.Z * MapBlock::zstride + nodePos.Y * MapBlock::ystride + nodePos.X];
}

video::SColor BlockLightFloodFill::MapBlockLightInfo::getLightColor(v3s16 nodePos)
{
	return getColorFromLight(getLight(nodePos));
}

u16 channelFalloff(u16 ch)
{
	if (ch == 0)
		return ch;
	return ch-1;
}

bool BlockLightFloodFill::MapBlockLightInfo::lightFalloff(v3s16 nodePos, video::SColor &color)
{
	u16 color_light_v = getLightFromColor(color) ;
	if (color_light_v == 0)
		return false;

	color_light_v |= channelFalloff(color_light_v >> 10) << 10;
	color_light_v |= channelFalloff(color_light_v >> 5 & 0x1f) << 5;
	color_light_v |= channelFalloff(color_light_v & 0x1f);

	u16 &light_v = light[nodePos.Z * MapBlock::zstride + nodePos.Y * MapBlock::ystride + nodePos.X];
	light_v |= std::max(light_v >> 10, color_light_v >> 10) << 10;
	light_v |= std::max(light_v >> 5 & 0x1f, color_light_v >> 5 & 0x1f) << 5;
	light_v |= std::max(light_v & 0x1f, color_light_v & 0x1f);

	color = getColorFromLight(color_light_v);

	return true;
}

void BlockLightFloodFill::addMapBlock(v3s16 blockpos, MapBlock *block)
{
	if (!block)
		return;
	block->refGrab();
	m_mapblocks_light.emplace(blockpos, MapBlockLightInfo{block});
	addLightNodesInQueue(block, true);
}
void BlockLightFloodFill::removeMapBlock(v3s16 blockpos)
{
	auto it = m_mapblocks_light.find(blockpos);
	if (it != m_mapblocks_light.end()) {
		addLightNodesInQueue(it->second.block, false);
		/*if (it->second.block)
			it->second.block->refDrop();
		m_mapblocks_light.erase(it);*/
	}
}

void BlockLightFloodFill::addLightNodesInQueue(MapBlock *block, bool propagate)
{
	for (s16 z = 0; z < MAP_BLOCKSIZE; ++z) {
		for (s16 y = 0; y < MAP_BLOCKSIZE; ++y) {
			for (s16 x = 0; x < MAP_BLOCKSIZE; ++x) {
				v3s16 pos(x, y, z);
				u16 content = block->getNodeNoCheck(pos).getContent();

				if (content == CONTENT_IGNORE)
					continue;
				auto &cf = m_nodedef->get(content);

				if (cf.light_source > 0) {
					auto light_emission = cf.light_color;
					light_emission.setRed(light_emission.getRed() * cf.light_source / 15.0f);
					light_emission.setGreen(light_emission.getGreen() * cf.light_source / 15.0f);
					light_emission.setBlue(light_emission.getBlue() * cf.light_source / 15.0f);

					LightNodeEntry light_node = {block->getPos(), pos, light_emission};
					if (propagate) {
						m_light_propagation_queue.push(light_node);
					} else {
						m_light_removal_queue.push(light_node);
					}
				}
			}
		}
	}
}

void BlockLightFloodFill::updateFill()
{
	while (!m_light_propagation_queue.empty()) {
		auto light_node = m_light_propagation_queue.front();
		m_light_propagation_queue.pop();

		std::unordered_map<v3s16, bool> passed_nodes;
		passed_nodes.emplace(light_node.mapblockPos * MAP_BLOCKSIZE + light_node.nodePos, true);
		recurseFill(light_node, passed_nodes);
	}
}

void BlockLightFloodFill::recurseFill(const LightNodeEntry &cur_lightnode, std::unordered_map<v3s16, bool> &passed_lightnodes)
{
	auto mapblock = m_mapblocks_light[cur_lightnode.mapblockPos];

	if (!mapblock.block)
		return;
	
	for (u8 side = 0; side < 6; ++side) {
		v3s16 neighbor_node_pos = cur_lightnode.mapblockPos * MAP_BLOCKSIZE + cur_lightnode.nodePos + g_6dirs[side];

		if (passed_lightnodes[neighbor_node_pos])
			continue;

		v3s16 neighbor_mapblock_pos = getContainerPos(neighbor_node_pos, MAP_BLOCKSIZE);
		v3s16 neighbor_rel_node_pos = neighbor_node_pos - neighbor_mapblock_pos * MAP_BLOCKSIZE;

		auto &neighbor_mapblock = m_mapblocks_light[neighbor_mapblock_pos];

		if (!neighbor_mapblock.block)
			continue;

		u16 content = neighbor_mapblock.block->getNodeNoCheck(neighbor_rel_node_pos).getContent();

		if (content == CONTENT_IGNORE)
			continue;
		auto &cf = m_nodedef->get(content);

		if (!cf.light_propagates || cf.solidness == 2)
			continue;
		
		video::SColor new_lightcolor = cur_lightnode.lightColor;
		if (!neighbor_mapblock.lightFalloff(neighbor_rel_node_pos, new_lightcolor))
			continue;
		
		passed_lightnodes.emplace(neighbor_node_pos, true);

		recurseFill({
			neighbor_mapblock_pos, neighbor_rel_node_pos,
			new_lightcolor}, passed_lightnodes);
	}
}
