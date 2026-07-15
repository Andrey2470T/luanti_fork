#include "floodfill.h"
#include "nodedef.h"
#include "util/directiontables.h"
#include "threading/mutex_auto_lock.h"

bool NodeLightChannel::lightFalloff(u8 propagateLight)
{
	if (propagateLight == 0)
		return false;
	if (propagateLight <= 2)
		propagateLight = 0;
	else
		propagateLight -= 2;

	ch = std::max(propagateLight, ch);
	return true;
}

bool NodeLight::lightFalloff(const NodeLight &propagateLight)
{
	bool falloff_r = r.lightFalloff(propagateLight.r);
	bool falloff_g = g.lightFalloff(propagateLight.g);
	bool falloff_b = b.lightFalloff(propagateLight.b);
	return falloff_r || falloff_g || falloff_b;
}

void BlockLightPropagator::addMapBlock(v3s16 blockpos, MapBlock *block)
{
	if (!block)
		return;
	
	MutexAutoLock lock(mapblocks_light_grid_mutex);
	mapblocks_light_grid.insert_or_assign(blockpos, MapBlockLightInfo{block});
	addLightNodesInQueue(block);
}

void BlockLightPropagator::removeMapBlocks(const std::vector<v3s16> &blocks_positions)
{
	MutexAutoLock lock(mapblocks_light_grid_mutex);
	for (auto &pos : blocks_positions) {
		auto it = mapblocks_light_grid.find(pos);
		if (it != mapblocks_light_grid.end())
			mapblocks_light_grid.erase(it);
	}
}

u16 BlockLightPropagator::getLight(v3s16 nodePos) const
{
    v3s16 mapblock_pos = getContainerPos(nodePos, MAP_BLOCKSIZE);
	auto it = mapblocks_light_grid.find(mapblock_pos);

	if (it == mapblocks_light_grid.end())
		return 0;
	
	v3s16 rel_nodepos = nodePos - mapblock_pos * MAP_BLOCKSIZE;
	return (u16)(it->second.getLight(rel_nodepos));
}

video::SColor BlockLightPropagator::getLightColor(v3s16 nodePos) const
{
	u16 light = getLight(nodePos);
    return video::SColor(0, light >> 10 & 0x1fu, light >> 5 & 0x1fu, light & 0x1fu);
}

u16 BlockLightPropagator::maxLight(u16 light1, u16 light2)
{
	u16 max_light = 0;
	max_light |= std::max(light1 >> 10, light2 >> 10) << 10;
	max_light |= std::max(light1 >> 5 & 0x1fu, light2 >> 5 & 0x1fu) << 5;
	max_light |= std::max(light1 & 0x1fu, light2 & 0x1fu);

    return max_light;
}

void BlockLightPropagator::propagateLight()
{
	std::unordered_map<v3s16, bool> visited_nodes;
    visited_nodes.reserve(4096);

	while (!light_propagation_queue.empty()) {
		auto light_node = light_propagation_queue.front();
		light_propagation_queue.pop();

		auto &mapblock = mapblocks_light_grid[light_node.mapblockPos];

		if (!mapblock.block)
			continue;

		auto &current_light = mapblock.getLight(light_node.nodePos);

		for (u8 side = 0; side < 6; ++side) {
			v3s16 next_node_pos = light_node.mapblockPos * MAP_BLOCKSIZE + light_node.nodePos + g_6dirs[side];

			if (visited_nodes[next_node_pos])
				continue;

        	v3s16 next_mapblock_pos = getContainerPos(next_node_pos, MAP_BLOCKSIZE);
        	v3s16 next_rel_node_pos = next_node_pos - next_mapblock_pos * MAP_BLOCKSIZE;

			auto &neighbor_mapblock = mapblocks_light_grid[next_mapblock_pos];

			if (!neighbor_mapblock.block)
            	continue;

        	u16 content = neighbor_mapblock.block->getNodeNoCheck(next_rel_node_pos).getContent();

        	if (content == CONTENT_IGNORE)
            	continue;
        	auto &cf = nodedef->get(content);

       		if (!cf.light_propagates || cf.solidness == 2)
            	continue;

			auto &next_light = neighbor_mapblock.getLight(next_rel_node_pos);
        	if (!next_light.lightFalloff(current_light))
            	continue;
			
			visited_nodes[next_node_pos] = true;

			LightNodeEntry next_light_node = {next_mapblock_pos, next_rel_node_pos, next_light};
			light_propagation_queue.push(next_light_node);
		}
	}
}

void BlockLightPropagator::addLightNodesInQueue(MapBlock *block)
{
	for (s16 z = 0; z < MAP_BLOCKSIZE; ++z) {
		for (s16 y = 0; y < MAP_BLOCKSIZE; ++y) {
			for (s16 x = 0; x < MAP_BLOCKSIZE; ++x) {
				v3s16 pos(x, y, z);
				u16 content = block->getNodeNoCheck(pos).getContent();

				if (content == CONTENT_IGNORE)
					continue;
				auto &cf = nodedef->get(content);

				if (cf.light_source > 0) {
					auto light_emission = cf.light_color;
					f32 delim = cf.light_source / 15.0f;

					u8 red = light_emission.getRed() / 8;
					u8 green = light_emission.getGreen() / 8;
					u8 blue = light_emission.getBlue() / 8;

					auto &mapblock = mapblocks_light_grid[block->getPos()];
					auto &light = mapblock.getLight(pos);
					light.r.ch = red;
					light.g.ch = green;
					light.b.ch = blue;

					LightNodeEntry light_node = {block->getPos(), pos, {red, green, blue}};
					light_propagation_queue.push(light_node);
				}
			}
		}
	}
}
