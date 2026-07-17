#include "floodfill.h"
#include "nodedef.h"
#include "util/directiontables.h"
#include "threading/mutex_auto_lock.h"
#include "map.h"
#include <queue>

bool NodeLightChannel::lightFalloff(u8 propagateLight)
{
	if (propagateLight == 0)
		return false;
	if (propagateLight <= 2)
		propagateLight = 0;
	else
		propagateLight -= 2;

	ch = propagateLight;
	return true;
}

bool NodeLight::lightFalloff(const NodeLight &propagateLight)
{
	bool falloff_r = r.lightFalloff(propagateLight.r);
	bool falloff_g = g.lightFalloff(propagateLight.g);
	bool falloff_b = b.lightFalloff(propagateLight.b);
	return falloff_r || falloff_g || falloff_b;
}

void NodeLight::mixLight(const NodeLight &propagateLight)
{
	r.ch = std::max(propagateLight.r.ch, r.ch);
	g.ch = std::max(propagateLight.g.ch, g.ch);
	b.ch = std::max(propagateLight.b.ch, b.ch);
}

LightSourceGrid::LightSourceGrid(u32 r, u32 g, u32 b)
{
	source_color.r.ch = r;
	source_color.g.ch = g;
	source_color.b.ch = b;

	cube_size = std::max(std::max(r / 2 + 1, g / 2 + 1), b / 2 + 1) * 2 + 1;
	light.resize(cube_size * cube_size * cube_size);

	propagateLight();
}

const NodeLight &LightSourceGrid::getLight(v3s16 absSourcePos, v3s16 absNodePos) const
{
	v3s16 rel_source_pos((cube_size - 1) / 2);
	v3s16 node_pos = absNodePos - absSourcePos + rel_source_pos;
    u32 pos = node_pos.Z * cube_size * cube_size + node_pos.Y * cube_size + node_pos.X;

	if (pos >= light.size()) {
		static NodeLight fallback;
		return fallback;
	}

	return light[pos];
}

std::vector<v3s16> LightSourceGrid::getCoveringMapblocks(v3s16 absSourcePos, bool self_include) const
{
	std::vector<v3s16> positions;

	v3s16 source_blocks_pos = getContainerPos(absSourcePos, MAP_BLOCKSIZE);
	v3s16 corner_nodes = absSourcePos - v3s16((cube_size - 1) / 2);
	v3s16 corner_block_pos1 = getContainerPos(corner_nodes, MAP_BLOCKSIZE);
	v3s16 corner_block_pos2 = getContainerPos(corner_nodes + v3s16(cube_size), MAP_BLOCKSIZE);

	for (s16 x = corner_block_pos1.X; x <= corner_block_pos2.X; ++x)
		for (s16 y = corner_block_pos1.Y; y <= corner_block_pos2.Y; ++y)
			for (s16 z = corner_block_pos1.Z; z <= corner_block_pos2.Z; ++z) {
				v3s16 cur_pos(x, y, z);
				if (!self_include && cur_pos == source_blocks_pos)
					continue;
				positions.emplace_back(x, y, z);
			}

	return positions;
}

void LightSourceGrid::propagateLight()
{
	std::queue<std::pair<v3s16, NodeLight>> nodes_queue;
	v3s16 source_pos((cube_size - 1) / 2);
	
	auto &source_light = getLight(source_pos);
	source_light = source_color;
	nodes_queue.push({source_pos, source_color});

	std::unordered_map<v3s16, bool> visited_nodes;
    visited_nodes.reserve(4096);
	visited_nodes[source_pos] = true;

	while (!nodes_queue.empty()) {
		auto light_p = nodes_queue.front();
		nodes_queue.pop();

		auto &current_light = getLight(light_p.first);

		for (u8 side = 0; side < 6; ++side) {
			v3s16 next_pos = light_p.first + g_6dirs[side];

			if (visited_nodes[next_pos])
				continue;
			auto &next_light = getLight(next_pos);

        	if (!next_light.lightFalloff(current_light))
            	continue;
			
			visited_nodes[next_pos] = true;

			nodes_queue.push({next_pos, next_light});
		}
	}
}

NodeLight &LightSourceGrid::getLight(v3s16 relNodePos)
{
	u32 pos = relNodePos.Z * cube_size * cube_size + relNodePos.Y * cube_size + relNodePos.X;

	if (pos >= light.size()) {
		static NodeLight fallback;
		return fallback;
	}
	return light[pos];
}

void BlockLightPropagator::LeveledNodeLight::mixLight(
	BlockLightPropagator *propagator, v3s16 absNodePos,
	const std::vector<v3s16> &sources_positions)
{
	for (auto &pos : sources_positions) {
		auto light_grid = propagator->light_sources[pos];

		if (!light_grid) {
			propagator->light_sources.erase(pos);
			continue;
		}
		auto &source_light = light_grid->getLight(pos, absNodePos);
		actualLight.mixLight(source_light);
	}
}

void BlockLightPropagator::MapBlockLightInfo::propagateLight(BlockLightPropagator *propagator)
{
	for (s16 z = 0; z < MAP_BLOCKSIZE; ++z) {
		for (s16 y = 0; y < MAP_BLOCKSIZE; ++y) {
			for (s16 x = 0; x < MAP_BLOCKSIZE; ++x) {
				v3s16 pos(x, y, z);
				auto &light = getLight(pos);

				u16 content = block->getNodeNoCheck(pos).getContent();

        		if (content == CONTENT_IGNORE)
            		continue;
        		auto &cf = propagator->nodedef->get(content);

       			if (!cf.light_propagates || cf.solidness == 2)
            		continue;

				v3s16 abs_pos = block->getPosRelative() + pos;
				light.mixLight(propagator, abs_pos, light_sources);
			}
		}
	}
}

void BlockLightPropagator::addMapBlock(v3s16 blockpos, MapBlock *block)
{
	if (!block)
		return;

	scanForLightSources(block);
}

void BlockLightPropagator::removeMapBlocks(const std::vector<v3s16> &blocks_positions)
{
	for (auto &pos : blocks_positions)
		pending_mapblocks_lights[pos] = {MapBlockLightInfo{nullptr}, false};
}

u16 BlockLightPropagator::getLight(v3s16 nodePos)
{
    v3s16 mapblock_pos = getContainerPos(nodePos, MAP_BLOCKSIZE);
	auto &mapblock = mapblocks_light_grid[mapblock_pos];

	if (!mapblock.block)
		return 0;
	
	v3s16 rel_nodepos = nodePos - mapblock_pos * MAP_BLOCKSIZE;
	return (u16)(mapblock.getLight(rel_nodepos).actualLight);
}

video::SColor BlockLightPropagator::getLightColor(v3s16 nodePos)
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
	for (auto &light_mb_p : pending_mapblocks_lights) {
		auto &mb = mapblocks_light_grid[light_mb_p.first];

		if (light_mb_p.second.second) {
			auto &pending_mb = light_mb_p.second.first;
			mb.block = pending_mb.block;
			mb.light_sources = pending_mb.light_sources;

			mb.propagateLight(this);
		}
		else {
			std::set<v3s16> update_mapblocks;
			for (auto &source_pos : mb.light_sources) {
				auto light_grid = light_sources[source_pos];

				if (light_grid) {
					auto update_cover_mapblocks = light_grid->getCoveringMapblocks(source_pos, false);

					for (auto &update_mb : update_cover_mapblocks) {
						if (map->getBlockNoCreateNoEx(update_mb))
							update_mapblocks.emplace(update_mb);
					}
				}
				light_sources.erase(source_pos);
			}
			mapblocks_light_grid.erase(light_mb_p.first);

			for (auto &update_mb : update_mapblocks)
				mapblocks_light_grid[update_mb].propagateLight(this);
		}
	}

	pending_mapblocks_lights.clear();
}

void BlockLightPropagator::scanForLightSources(MapBlock *block)
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

					auto *inserted = &*(light_sources_grids.emplace(red, green, blue).first);
					v3s16 source_pos = block->getPosRelative() + pos;

					light_sources[source_pos] = inserted;
					auto update_mapblocks = inserted->getCoveringMapblocks(source_pos);

					for (auto &pos : update_mapblocks) {
						auto update_block = map->getBlockNoCreateNoEx(pos);

						if (!update_block)
							continue;

						if (pending_mapblocks_lights[pos].first.block)
							pending_mapblocks_lights[pos].first.light_sources.emplace_back(source_pos);
						else
							pending_mapblocks_lights[pos] = {MapBlockLightInfo{update_block, {source_pos}}, true};
					}
				}
			}
		}
	}
}
