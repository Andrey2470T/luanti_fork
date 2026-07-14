#include "floodfill.h"
#include "nodedef.h"
#include "util/directiontables.h"

u16 getLightFromColor(const video::SColor &color)
{
	u16 light = 0;

	u16 light_r = ((u16)color.getRed() & 0x1fu);
	u16 light_g = ((u16)color.getGreen() & 0x1fu);
	u16 light_b = ((u16)color.getBlue() & 0x1fu);

	light |= light_r << 10;
	light |= light_g << 5;
	light |= light_b;

	return light;
}

video::SColor getColorFromLight(u16 light)
{
	return video::SColor(255, light >> 10, light >> 5 & 0x1fu, light & 0x1fu);
}

u16 BlockLightFloodFill::MapBlockLightInfo::getLight(v3s16 nodePos) const
{
	return light[nodePos.Z * MapBlock::zstride + nodePos.Y * MapBlock::ystride + nodePos.X];
}

video::SColor BlockLightFloodFill::MapBlockLightInfo::getLightColor(v3s16 nodePos) const
{
	return getColorFromLight(getLight(nodePos));
}

u16 channelFalloff(u16 ch)
{
	if (ch <= 2)
		return 0;
	return ch-2;
}

bool BlockLightFloodFill::MapBlockLightInfo::lightFalloff(v3s16 nodePos, video::SColor &color)
{
	u16 color_light_v = getLightFromColor(color);
	if (color_light_v == 0)
		return false;

	u16 color_light_r = channelFalloff(color_light_v >> 10);
	u16 color_light_g = channelFalloff(color_light_v >> 5 & 0x1fu);
	u16 color_light_b = channelFalloff(color_light_v & 0x1fu);

	u16 &light_v = light[nodePos.Z * MapBlock::zstride + nodePos.Y * MapBlock::ystride + nodePos.X];

	light_v = 0;
	light_v |= std::max<u16>(light_v >> 10, color_light_r) << 10;
	light_v |= std::max<u16>(light_v >> 5 & 0x1fu, color_light_g) << 5;
	light_v |= std::max<u16>(light_v & 0x1fu, color_light_b);

	color = getColorFromLight(color_light_r << 10 | color_light_g << 5 | color_light_b);

	return true;
}

void BlockLightFloodFill::addMapBlock(v3s16 blockpos, MapBlock *block)
{
	if (!block)
		return;
	
	m_mapblocks_light.insert_or_assign(blockpos, MapBlockLightInfo{block});
	addLightNodesInQueue(block);
}

void BlockLightFloodFill::removeMapBlocks(const std::vector<v3s16> &blocks_positions)
{
	for (auto &pos : blocks_positions) {
		auto it = m_mapblocks_light.find(pos);
		if (it != m_mapblocks_light.end())
			m_mapblocks_light.erase(it);
	}
}

u16 BlockLightFloodFill::getLight(v3s16 nodePos) const
{
	v3s16 mapblock_pos = getContainerPos(nodePos, MAP_BLOCKSIZE);
	auto it = m_mapblocks_light.find(mapblock_pos);

	if (it == m_mapblocks_light.end())
		return 0;
	
	v3s16 rel_nodepos = nodePos - mapblock_pos * MAP_BLOCKSIZE;
	return it->second.getLight(rel_nodepos);
}

video::SColor BlockLightFloodFill::getLightColor(v3s16 nodePos) const
{
    return getColorFromLight(getLight(nodePos));
}

u16 BlockLightFloodFill::maxLight(u16 light1, u16 light2)
{
    auto color1 = getColorFromLight(light1);
	auto color2 = getColorFromLight(light2);

	video::SColor max_color(
		std::max(color1.getAlpha(), color2.getAlpha()),
		std::max(color1.getRed(), color2.getRed()),
		std::max(color1.getGreen(), color2.getGreen()),
		std::max(color1.getBlue(), color2.getBlue()));
	
	return getLightFromColor(max_color);
}

void BlockLightFloodFill::addLightNodesInQueue(MapBlock *block)
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
					f32 delim = cf.light_source / 15.0f;

					u8 red = (u8)(light_emission.getRed() * delim) / 8 & 0x1fu;
					u8 green = (u8)(light_emission.getGreen() * delim) / 8 & 0x1fu;
					u8 blue = (u8)(light_emission.getBlue() * delim) / 8 & 0x1fu;
					light_emission.setRed(red);
					light_emission.setGreen(green);
					light_emission.setBlue(blue);

					LightNodeEntry light_node = {block->getPos(), pos, light_emission};
					m_light_propagation_queue.push(light_node);
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
		recurseFill(light_node, passed_nodes, 0);
    }
}

void BlockLightFloodFill::recurseFill(const LightNodeEntry &cur_lightnode, std::unordered_map<v3s16, bool> &passed_lightnodes, u8 level)
{
	if (level == 5)
		return;

	auto &mapblock = m_mapblocks_light[cur_lightnode.mapblockPos];

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

		passed_lightnodes[neighbor_node_pos] = true;

		recurseFill({
			neighbor_mapblock_pos, neighbor_rel_node_pos,
			new_lightcolor}, passed_lightnodes, level+1);
    }
}
