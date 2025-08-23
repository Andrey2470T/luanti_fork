// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "clientmap.h"
#include "client/client.h"
#include "client/mesh/meshoperations.h"
#include "mapblockmesh.h"
#include "mapsector.h"
#include "mapblock.h"
#include "nodedef.h"
#include "profiler.h"
#include "settings.h"
#include "util/basic_macros.h"
#include "util/tracy_wrapper.h"
#include "client/render/drawlist.h"
#include "client/ao/clientActiveObject.h"

/*
	ClientMap
*/

ClientMap::ClientMap(Client *client, DistanceSortedDrawList *drawlist)
    : Map(client->idef()), m_client(client), m_drawlist(drawlist)
{}

MapSector * ClientMap::emergeSector(v2s16 p2d)
{
	// Check that it doesn't exist already
	MapSector *sector = getSectorNoGenerate(p2d);

	// Create it if it does not exist yet
	if (!sector) {
		sector = new MapSector(this, p2d, m_gamedef);
		m_sectors[p2d] = sector;
	}

	return sector;
}

/*void ClientMap::render()
{
    video::IVideoDriver* driver = SceneManager->getVideoDriver();
    driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
    renderMap(driver, SceneManager->getSceneNodeRenderPass());
}*/

void ClientMap::getBlocksInViewRange(v3s16 cam_pos_nodes,
		v3s16 *p_blocks_min, v3s16 *p_blocks_max, float range)
{
	if (range <= 0.0f)
        range = m_drawlist->getDrawControl().wanted_range;

    v3s16 box_nodes_d = v3s16(1, 1, 1) * range;
    // Define p_nodes_min/max as v3s32 becaudynamic_cast<ActiveObjectMgr<ClientActiveObject> *>(m_ao_mgr)->se 'cam_pos_nodes -/+ box_nodes_d'
	// can exceed the range of v3s16 when a large view range is used near the
	// world edges.
    v3i p_nodes_min(
		cam_pos_nodes.X - box_nodes_d.X,
		cam_pos_nodes.Y - box_nodes_d.Y,
		cam_pos_nodes.Z - box_nodes_d.Z);
    v3i p_nodes_max(
		cam_pos_nodes.X + box_nodes_d.X,
		cam_pos_nodes.Y + box_nodes_d.Y,
		cam_pos_nodes.Z + box_nodes_d.Z);
	// Take a fair amount as we will be dropping more out later
	// Umm... these additions are a bit strange but they are needed.
	*p_blocks_min = v3s16(
			p_nodes_min.X / MAP_BLOCKSIZE - 3,
			p_nodes_min.Y / MAP_BLOCKSIZE - 3,
			p_nodes_min.Z / MAP_BLOCKSIZE - 3);
	*p_blocks_max = v3s16(
			p_nodes_max.X / MAP_BLOCKSIZE + 1,
			p_nodes_max.Y / MAP_BLOCKSIZE + 1,
			p_nodes_max.Z / MAP_BLOCKSIZE + 1);
}

void ClientMap::update()
{
	ScopeProfiler sp(g_profiler, "CM::updateDrawList()", SPT_AVG);

    m_drawlist->lockMeshes();

    for (auto &block : m_visible_mapblocks) {
        block->mesh->removeFromDrawList(m_drawlist);
		block->refDrop();
	}
    m_visible_mapblocks.clear();

    const v3s16 cam_pos_nodes = floatToInt(m_client->getEnv().getLocalPlayer()->getCamera()->getPosition(), BS);

	v3s16 p_blocks_min;
	v3s16 p_blocks_max;
	getBlocksInViewRange(cam_pos_nodes, &p_blocks_min, &p_blocks_max);

    // Number of blocks currently loaded by the client
    u32 blocks_loaded = 0;
    // Number of blocks with mesh in rendering range
    u32 blocks_in_range_with_mesh = 0;

    MapBlockVect sectorblocks;

    for (auto &sector_it : m_sectors) {
        MapSector *sector = sector_it.second;
        v2s16 sp = sector->getPos();

        blocks_loaded += sector->size();
        if (!m_drawlist->getDrawControl().range_all) {
            if (sp.X < p_blocks_min.X || sp.X > p_blocks_max.X ||
                    sp.Y < p_blocks_min.Z || sp.Y > p_blocks_max.Z)
                continue;
        }

        // Loop through blocks in sector
        sector->getBlocks(sectorblocks);
        for (auto &block : sectorblocks) {
            MapBlockMesh *mesh = block->mesh;

            if (!mesh)
                continue;

            m_visible_mapblocks.push_back(block);

            mesh->addInDrawList(m_drawlist);

            block->resetUsageTimer();
            blocks_in_range_with_mesh++;
        }
	}

    m_drawlist->unlockMeshes();

    g_profiler->avg("MapBlocks loaded [#]", blocks_loaded);
    g_profiler->avg("MapBlocks with mesh [#]", blocks_in_range_with_mesh);
}

void ClientMap::updateShadowBlocks(const v3f &shadow_light_pos, const v3f &shadow_light_dir, f32 radius)
{
    ScopeProfiler sp(g_profiler, "CM::updateShadowDrawList()", SPT_AVG);

    m_drawlist->lockMeshes(true);

    for (auto &block : m_visible_shadow_mapblocks) {
        block->mesh->removeFromDrawList(m_drawlist, true);
        block->refDrop();
    }
    m_visible_shadow_mapblocks.clear();

    // Number of blocks currently loaded by the client
    u32 blocks_loaded = 0;
    // Number of blocks with mesh in rendering range
    u32 blocks_in_range_with_mesh = 0;

    MapBlockVect sectorblocks;

    for (auto &sector_it : m_sectors) {
        MapSector *sector = sector_it.second;

        // Loop through blocks in sector
        sector->getBlocks(sectorblocks);
        for (auto &block : sectorblocks) {
            MapBlockMesh *mesh = block->mesh;

            if (!mesh)
                continue;

            v3f block_pos = mesh->getBoundingSphereCenter();
            v3f projection = shadow_light_pos + shadow_light_dir * shadow_light_dir.dotProduct(block_pos - shadow_light_pos);
            if (projection.getDistanceFrom(block_pos) > (radius + mesh->getBoundingRadius()))
                continue;

            m_visible_shadow_mapblocks.push_back(block);

            mesh->addInDrawList(m_drawlist, true);

            block->resetUsageTimer();
            blocks_in_range_with_mesh++;
        }
    }

    m_drawlist->setLightPos(shadow_light_pos);

    m_drawlist->unlockMeshes(true);

    g_profiler->avg("MapBlocks loaded [#]", blocks_loaded);
    g_profiler->avg("MapBlocks with mesh [#]", blocks_in_range_with_mesh);
}

void ClientMap::touchMapBlocks()
{
    auto control = m_drawlist->getDrawControl();
    if (control.range_all)
		return;

	ScopeProfiler sp(g_profiler, "CM::touchMapBlocks()", SPT_AVG);

    for (auto &visible_block : m_visible_mapblocks) {
        // Keep the block alive as long as it is in range.
        visible_block->resetUsageTimer();
    }
}

void ClientMap::addActiveObject(u16 id)
{
    auto cao = m_client->getEnv().getActiveObject(id);

    v3s16 blockpos = getContainerPos(floatToInt(cao->getPosition(), BS), BS);
    MapBlock *block = getBlockNoCreate(blockpos);

    if (!block)
        return;

    block->mesh->addActiveObject(id);


}

void ClientMap::updateMapBlocksActiveObjects()
{
    for (auto &sector_it : m_sectors) {
        MapBlockVect sectorblocks;
        sector_it.second->getBlocks(sectorblocks);

        for (auto &block : sectorblocks) {
            for (u16 ao_id : block->mesh->getActiveObjects()) {
                auto cao = m_client->getEnv().getActiveObject(ao_id);
                v3s16 blockpos = getContainerPos(floatToInt(cao->getPosition(), BS), BS);

                if (blockpos != block->getPos()) {
                    block->mesh->removeActiveObject(ao_id);
                    getBlockNoCreate(blockpos)->mesh->addActiveObject(ao_id);
                }
            }
        }
    }
}

void ClientMap::removeActiveObject(u16 id)
{
    auto cao = m_client->getEnv().getActiveObject(id);

    v3s16 blockpos = getContainerPos(floatToInt(cao->getPosition(), BS), BS);
    MapBlock *block = getBlockNoCreate(blockpos);

    if (!block)
        return;

    block->mesh->removeActiveObject(id);
}
static bool getVisibleBrightness(Map *map, const v3f &p0, v3f dir, float step,
	float step_multiplier, float start_distance, float end_distance,
	const NodeDefManager *ndef, u32 daylight_factor, float sunlight_min_d,
	int *result, bool *sunlight_seen)
{
	int brightness_sum = 0;
	int brightness_count = 0;
	float distance = start_distance;
	dir.normalize();
	v3f pf = p0;
	pf += dir * distance;
	int noncount = 0;
	bool nonlight_seen = false;
	bool allow_allowing_non_sunlight_propagates = false;
    bool allow_non_sunlight_propagates = false;
	// Check content nearly at camera position
    {
        v3s16 p = floatToInt(p0 /*+ dir * 3*BS*/, BS);
        MapNode n = map->getNode(p);
		if(ndef->getLightingFlags(n).has_light &&
				!ndef->getLightingFlags(n).sunlight_propagates)
			allow_allowing_non_sunlight_propagates = true;
	}
	// If would start at CONTENT_IGNORE, start closer
	{
		v3s16 p = floatToInt(pf, BS);
		MapNode n = map->getNode(p);
		if(n.getContent() == CONTENT_IGNORE){
			float newd = 2*BS;
			pf = p0 + dir * 2*newd;
			distance = newd;
			sunlight_min_d = 0;
		}
	}
	for (int i=0; distance < end_distance; i++) {
		pf += dir * step;
		distance += step;
		step *= step_multiplier;

		v3s16 p = floatToInt(pf, BS);
		MapNode n = map->getNode(p);
		ContentLightingFlags f = ndef->getLightingFlags(n);
		if (allow_allowing_non_sunlight_propagates && i == 0 &&
				f.has_light && !f.sunlight_propagates) {
			allow_non_sunlight_propagates = true;
		}

		if (!f.has_light || (!f.sunlight_propagates && !allow_non_sunlight_propagates)){
			nonlight_seen = true;
			noncount++;
			if(noncount >= 4)
				break;
			continue;
		}

		if (distance >= sunlight_min_d && !*sunlight_seen && !nonlight_seen)
			if (n.getLight(LIGHTBANK_DAY, f) == LIGHT_SUN)
				*sunlight_seen = true;
		noncount = 0;
		brightness_sum += decode_light(n.getLightBlend(daylight_factor, f));
		brightness_count++;
	}
	*result = 0;
	if(brightness_count == 0)
		return false;
    *result = brightness_sum / brightness_count;
	/*std::cerr<<"Sampled "<<brightness_count<<" points; result="
			<<(*result)<<std::endl;*/
    return true;
}

int ClientMap::getBackgroundBrightness(float max_d, u32 daylight_factor,
		int oldvalue, bool *sunlight_seen_result)
{
	ScopeProfiler sp(g_profiler, "CM::getBackgroundBrightness", SPT_AVG);
	static v3f z_directions[50] = {
		v3f(-100, 0, 0)
	};
	static f32 z_offsets[50] = {
		-1000,
	};

	if (z_directions[0].X < -99) {
		for (u32 i = 0; i < ARRLEN(z_directions); i++) {
			// Assumes FOV of 72 and 16/9 aspect ratio
			z_directions[i] = v3f(
				0.02 * myrand_range(-100, 100),
				1.0,
				0.01 * myrand_range(-100, 100)
			).normalize();
			z_offsets[i] = 0.01 * myrand_range(0,100);
		}
	}

	int sunlight_seen_count = 0;
	float sunlight_min_d = max_d*0.8;
	if(sunlight_min_d > 35*BS)
		sunlight_min_d = 35*BS;
	std::vector<int> values;
	values.reserve(ARRLEN(z_directions));

    v3f camera_dir = m_client->getEnv().getLocalPlayer()->getCamera()->getDirection();
	for (u32 i = 0; i < ARRLEN(z_directions); i++) {
		v3f z_dir = z_directions[i];
        matrix4 a;
		a.buildRotateFromTo(v3f(0,1,0), z_dir);

        v3f dir = a.rotateAndScaleVect(camera_dir);
		int br = 0;
		float step = BS*1.5;
		if(max_d > 35*BS)
			step = max_d / 35 * 1.5;
		float off = step * z_offsets[i];
		bool sunlight_seen_now = false;
        bool ok = getVisibleBrightness(this, m_camera_pos, dir,
				step, 1.0, max_d*0.6+off, max_d, m_nodedef, daylight_factor,
				sunlight_min_d,
				&br, &sunlight_seen_now);
		if(sunlight_seen_now)
			sunlight_seen_count++;
		if(!ok)
			continue;
		values.push_back(br);
		// Don't try too much if being in the sun is clear
		if(sunlight_seen_count >= 20)
			break;
	}
	int brightness_sum = 0;
	int brightness_count = 0;
	std::sort(values.begin(), values.end());
	u32 num_values_to_use = values.size();
	if(num_values_to_use >= 10)
		num_values_to_use -= num_values_to_use/2;
	else if(num_values_to_use >= 7)
		num_values_to_use -= num_values_to_use/3;
	u32 first_value_i = (values.size() - num_values_to_use) / 2;

	for (u32 i=first_value_i; i < first_value_i + num_values_to_use; i++) {
		brightness_sum += values[i];
		brightness_count++;
	}

	int ret = 0;
	if(brightness_count == 0){
        MapNode n = getNode(floatToInt(m_camera_pos, BS));
		ContentLightingFlags f = m_nodedef->getLightingFlags(n);
		if(f.has_light){
			ret = decode_light(n.getLightBlend(daylight_factor, f));
		} else {
			ret = oldvalue;
		}
	} else {
		ret = brightness_sum / brightness_count;
	}

	*sunlight_seen_result = (sunlight_seen_count > 0);
	return ret;
}


void ClientMap::reportMetrics(u64 save_time_us, u32 saved_blocks, u32 all_blocks)
{
	g_profiler->avg("CM::reportMetrics loaded blocks [#]", all_blocks);
}
