// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "clientmap.h"
#include "client/core/client.h"
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
#include "client/map/meshgeneratorthread.h"
#include "client/network/packethandler.h"
#include "client/render/rendersystem.h"

/*
	ClientMap
*/

ClientMap::ClientMap(Client *client, DistanceSortedDrawList *drawlist)
    : Map(client->idef()), m_client(client), m_drawlist(drawlist),
      m_mesh_update_manager(std::make_unique<MeshUpdateManager>(client)),
      m_mesh_grid({g_settings->getU16("client_mesh_chunk")}),
      m_unload_unused_data_timeout(std::max(g_settings->getFloat("client_unload_unused_data_timeout"), 0.0f)),
      m_mapblock_limit(g_settings->getS32("client_mapblock_limit"))
{}

ClientMap::~ClientMap()
{
    m_mesh_update_manager->stop();
    m_mesh_update_manager->wait();

    MeshUpdateResult r;
    while (m_mesh_update_manager->getNextResult(r)) {
        for (auto block : r.map_blocks)
            if (block)
                block->refDrop();
        delete r.mesh;
    }
}

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
    ScopeProfiler sp(g_profiler, "CM::update()", SPT_AVG);

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
    ScopeProfiler sp(g_profiler, "CM::updateShadowBlocks()", SPT_AVG);

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

bool ClientMap::addActiveObject(u16 id)
{
    auto cao = m_client->getEnv().getActiveObject(id);

    v3s16 blockpos = getContainerPos(floatToInt(cao->getPosition(), BS), BS);
    MapBlock *block = getBlockNoCreate(blockpos);

    if (!block || !block->mesh) {
        m_pending_to_add_caos.push_back(id);
        return false;
    }

    block->mesh->addActiveObject(id);

    return true;
}

void ClientMap::updateMapBlocksActiveObjects()
{
    auto it = m_pending_to_add_caos.begin();
    while (it != m_pending_to_add_caos.end()) {
        if (addActiveObject(*it))
            m_pending_to_add_caos.erase(it);
    }
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

    if (!block || !block->mesh)
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

    auto camera = m_client->getEnv().getLocalPlayer()->getCamera();
    v3f camera_dir = camera->getDirection();
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
        bool ok = getVisibleBrightness(this, camera->getPosition(), dir,
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
        MapNode n = getNode(floatToInt(camera->getPosition(), BS));
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

void ClientMap::startMeshUpdate()
{
    m_mesh_update_manager->start();
}

void ClientMap::stopMeshUpdate()
{
    m_mesh_update_manager->stop();
}

bool ClientMap::isMeshUpdateRunning() const
{
    return m_mesh_update_manager->isRunning();
}

void ClientMap::step(f32 dtime)
{
    /*
        Run Map's timers and unload unused data
    */
    auto pkt_handler = m_client->getPacketHandler();

    const float map_timer_and_unload_dtime = 5.25;
    if(m_map_timer_and_unload_interval.step(dtime, map_timer_and_unload_dtime)) {
        std::vector<v3s16> deleted_blocks;
        timerUpdate(map_timer_and_unload_dtime,
            m_unload_unused_data_timeout,
            m_mapblock_limit,
            &deleted_blocks);

        /*
            Send info to server
            NOTE: This loop is intentionally iterated the way it is.
        */

        std::vector<v3s16>::iterator i = deleted_blocks.begin();
        std::vector<v3s16> sendlist;
        for(;;) {
            if(sendlist.size() == 255 || i == deleted_blocks.end()) {
                if(sendlist.empty())
                    break;
                /*
                    [0] u16 command
                    [2] u8 count
                    [3] v3s16 pos_0
                    [3+6] v3s16 pos_1
                    ...
                */

                pkt_handler->sendDeletedBlocks(sendlist);

                if(i == deleted_blocks.end())
                    break;

                sendlist.clear();
            }

            sendlist.push_back(*i);
            ++i;
        }
    }


    auto minimap = m_client->getRenderSystem()->getDefaultMinimap();
    /*
        Replace updated meshes
    */
    {
        int num_processed_meshes = 0;
        std::vector<v3s16> blocks_to_ack;
        //bool force_update_shadows = false;
        MeshUpdateResult r;
        while (m_mesh_update_manager->getNextResult(r))
        {
            num_processed_meshes++;

            std::vector<std::shared_ptr<MinimapMapblock>> minimap_mapblocks;
            bool do_mapper_update = true;

            MapSector *sector = emergeSector(v2s16(r.p.X, r.p.Z));

            MapBlock *block = sector->getBlockNoCreateNoEx(r.p.Y);

            // The block in question is not visible (perhaps it is culled at the server),
            // create a blank block just to hold the chunk's mesh.
            // If the block becomes visible later it will replace the blank block.
            if (!block && r.mesh)
                block = sector->createBlankBlock(r.p.Y);

            if (block) {
                // Delete the old mesh
                delete block->mesh;
                block->mesh = nullptr;
                block->solid_sides = r.solid_sides;

                if (r.mesh) {
                    minimap_mapblocks = r.mesh->moveMinimapMapblocks();
                    if (minimap_mapblocks.empty())
                        do_mapper_update = false;

                    if (r.mesh->getMesh()->getBuffersCount() == 0) {
                        delete r.mesh;
                    } else {
                        // Replace with the new mesh
                        block->mesh = r.mesh;
                        //if (r.urgent)
                        //    force_update_shadows = true;
                    }
                }
            } else {
                delete r.mesh;
            }

            if (minimap && do_mapper_update) {
                v3s16 ofs;

                // See also mapblock_mesh.cpp for the code that creates the array of minimap blocks.
                for (ofs.Z = 0; ofs.Z < m_mesh_grid.cell_size; ofs.Z++)
                for (ofs.Y = 0; ofs.Y < m_mesh_grid.cell_size; ofs.Y++)
                for (ofs.X = 0; ofs.X < m_mesh_grid.cell_size; ofs.X++) {
                    size_t i = m_mesh_grid.getOffsetIndex(ofs);
                    if (i < minimap_mapblocks.size() && minimap_mapblocks[i])
                        minimap->addBlock(r.p + ofs, minimap_mapblocks[i].get());
                }
            }

            for (auto p : r.ack_list) {
                if (blocks_to_ack.size() == 255) {
                    pkt_handler->sendGotBlocks(blocks_to_ack);
                    blocks_to_ack.clear();
                }

                blocks_to_ack.emplace_back(p);
            }

            for (auto block : r.map_blocks)
                if (block)
                    block->refDrop();
        }
        if (blocks_to_ack.size() > 0) {
                // Acknowledge block(s)
                pkt_handler->sendGotBlocks(blocks_to_ack);
        }

        if (num_processed_meshes > 0)
            g_profiler->graphAdd("num_processed_meshes", num_processed_meshes);

        /*auto shadow_renderer = RenderingEngine::get_shadow_renderer();
        if (shadow_renderer && force_update_shadows)
            shadow_renderer->setForceUpdateShadowMap();*/
    }


    updateMapBlocksActiveObjects();

    /*
        Update block draw list every 200ms or when camera direction has
        changed much
    */
    update_draw_list_timer += dtime;
    touch_blocks_timer += dtime;

    auto camera = m_client->getEnv().getLocalPlayer()->getCamera();

    // call only one of updateDrawList, touchMapBlocks, or updateShadow per frame
    // (the else-ifs below are intentional)
    if (update_draw_list_timer >= update_draw_list_delta
            || camera->isNecessaryUpdateDrawList()
    ) {
        update_draw_list_timer = 0;
        update();
    } else if (touch_blocks_timer > update_draw_list_delta) {
        touchMapBlocks();
        touch_blocks_timer = 0;
    }/* else if (RenderingEngine::get_shadow_renderer()) {
        updateShadows();
    }*/
}

void ClientMap::addUpdateMeshTask(v3s16 p, bool ack_to_server, bool urgent)
{
    // Check if the block exists to begin with. In the case when a non-existing
    // neighbor is automatically added, it may not. In that case we don't want
    // to tell the mesh update thread about it.
    MapBlock *b = getBlockNoCreateNoEx(p);
    if (!b)
        return;

    m_mesh_update_manager->updateBlock(this, p, ack_to_server, urgent);
}

void ClientMap::addUpdateMeshTaskWithEdge(v3s16 blockpos, bool ack_to_server, bool urgent)
{
    m_mesh_update_manager->updateBlock(this, blockpos, ack_to_server, urgent, true);
}

void ClientMap::addUpdateMeshTaskForNode(v3s16 nodepos, bool ack_to_server, bool urgent)
{
    {
        v3s16 p = nodepos;
        infostream<<"Client::addUpdateMeshTaskForNode(): "
                <<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
                <<std::endl;
    }

    v3s16 blockpos = getNodeBlockPos(nodepos);
    v3s16 blockpos_relative = blockpos * MAP_BLOCKSIZE;
    m_mesh_update_manager->updateBlock(this, blockpos, ack_to_server, urgent, false);
    // Leading edge
    if (nodepos.X == blockpos_relative.X)
        addUpdateMeshTask(blockpos + v3s16(-1, 0, 0), false, urgent);
    if (nodepos.Y == blockpos_relative.Y)
        addUpdateMeshTask(blockpos + v3s16(0, -1, 0), false, urgent);
    if (nodepos.Z == blockpos_relative.Z)
        addUpdateMeshTask(blockpos + v3s16(0, 0, -1), false, urgent);
}

void ClientMap::addNode(v3s16 p, MapNode n, bool remove_metadata)
{
    //TimeTaker timer1("Client::addNode()");

    std::map<v3s16, MapBlock*> modified_blocks;

    try {
        //TimeTaker timer3("Client::addNode(): addNodeAndUpdate");
        addNodeAndUpdate(p, n, modified_blocks, remove_metadata);
    }
    catch(InvalidPositionException &e) {
    }

    for (const auto &modified_block : modified_blocks) {
        addUpdateMeshTaskWithEdge(modified_block.first, false, true);
    }
}

void ClientMap::removeNode(v3s16 p)
{
    std::map<v3s16, MapBlock*> modified_blocks;

    try {
        removeNodeAndUpdate(p, modified_blocks);
    }
    catch(InvalidPositionException &e) {
    }

    for (const auto &modified_block : modified_blocks) {
        addUpdateMeshTaskWithEdge(modified_block.first, false, true);
    }
}

void ClientMap::reportMetrics(u64 save_time_us, u32 saved_blocks, u32 all_blocks)
{
	g_profiler->avg("CM::reportMetrics loaded blocks [#]", all_blocks);
}
