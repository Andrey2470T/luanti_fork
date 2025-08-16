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

/*void ClientMap::updateCamera(v3f pos, v3f dir, f32 fov, v3s16 offset, img::color8 light_color)
{
    v3s16 previous_node = floatToInt(m_camera_pos, BS) + m_camera_offset;
    v3s16 previous_block = getContainerPos(previous_node, MAP_BLOCKSIZE);

    m_camera_pos = pos;
    m_camera_offset = offset;
    m_camera_light_color = light_color;

    v3s16 current_node = floatToInt(m_camera_pos, BS) + m_camera_offset;
    v3s16 current_block = getContainerPos(current_node, MAP_BLOCKSIZE);

    if (previous_block != current_block) {
        update();
        m_drawlist->updateCamera(pos, dir, fov, offset);
    }
}*/

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

    for (auto &block : m_visible_mapblocks) {
        block->mesh->removeFromDrawList(m_drawlist);
		block->refDrop();
	}
    m_visible_mapblocks.clear();

    const v3s16 cam_pos_nodes = floatToInt(m_client->getCamera()->getPosition(), BS);

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

    g_profiler->avg("MapBlocks loaded [#]", blocks_loaded);
    g_profiler->avg("MapBlocks with mesh [#]", blocks_in_range_with_mesh);
}

void ClientMap::updateShadowBlocks(const v3f &shadow_light_pos, const v3f &shadow_light_dir, f32 radius, f32 length)
{
    ScopeProfiler sp(g_profiler, "CM::updateShadowDrawList()", SPT_AVG);

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
/*static bool getVisibleBrightness(Map *map, const v3f &p0, v3f dir, float step,
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
    bool allow_non_sunlight_propagates = false;*/
	// Check content nearly at camera position
//	{
//		v3s16 p = floatToInt(p0 /*+ dir * 3*BS*/, BS);
/*		MapNode n = map->getNode(p);
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
    *result = brightness_sum / brightness_count;*/
	/*std::cerr<<"Sampled "<<brightness_count<<" points; result="
			<<(*result)<<std::endl;*/
//	return true;
//}

/*int ClientMap::getBackgroundBrightness(float max_d, u32 daylight_factor,
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
	for (u32 i = 0; i < ARRLEN(z_directions); i++) {
		v3f z_dir = z_directions[i];
        matrix4 a;
		a.buildRotateFromTo(v3f(0,1,0), z_dir);
        v3f dir = a.rotateAndScaleVect(m_camera_dir);
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
}*/

/*void ClientMap::renderPostFx(CameraMode cam_mode)
{
	// Sadly ISceneManager has no "post effects" render pass, in that case we
	// could just register for that and handle it in renderMap().

    MapNode n = getNode(floatToInt(m_camera_pos, BS));

	const ContentFeatures& features = m_nodedef->get(n);
	img::color8 post_color = features.post_effect_color;

	if (features.post_effect_color_shaded) {
		auto apply_light = [] (u32 color, u32 light) {
            return std::clamp(round32(color * light / 255.0f), 0, 255);
		};
        post_color.R(apply_light(post_color.R(), m_camera_light_color.R()));
        post_color.G(apply_light(post_color.G(), m_camera_light_color.G()));
        post_color.B(apply_light(post_color.B(), m_camera_light_color.B()));
	}

	// If the camera is in a solid node, make everything black.
	// (first person mode only)
	if (features.solidness == 2 && cam_mode == CAMERA_MODE_FIRST &&
            !m_drawlist->getDrawControl().allow_noclip) {
        post_color = img::black;
	}

    if (post_color.A() != 0) {
		// Draw a full-screen rectangle
		video::IVideoDriver* driver = SceneManager->getVideoDriver();
		v2u32 ss = driver->getScreenSize();
		core::rect<s32> rect(0,0, ss.X, ss.Y);
		driver->draw2DRectangle(post_color, rect);
    }
}*/

/*void ClientMap::renderMapShadows(video::IVideoDriver *driver,
		const video::SMaterial &material, s32 pass, int frame, int total_frames)
{
	bool is_transparent_pass = pass != scene::ESNRP_SOLID;
	std::string prefix;
	if (is_transparent_pass)
		prefix = "renderMap(SHADOW TRANS): ";
	else
		prefix = "renderMap(SHADOW SOLID): ";

	const auto mesh_grid = m_client->getMeshGrid();
	// Gets world position from block map position
	const auto get_block_wpos = [&] (v3s16 pos) -> v3f {
		return intToFloat(mesh_grid.getMeshPos(pos) * MAP_BLOCKSIZE - m_camera_offset, BS);
	};

	MeshBufListMaps grouped_buffers;
	DrawDescriptorList draw_order;

	std::size_t count = 0;
	std::size_t meshes_per_frame = m_drawlist_shadow.size() / total_frames + 1;
	std::size_t low_bound = is_transparent_pass ? 0 : meshes_per_frame * frame;
	std::size_t high_bound = is_transparent_pass ? m_drawlist_shadow.size() : meshes_per_frame * (frame + 1);

	// transparent pass should be rendered in one go
	if (is_transparent_pass && frame != total_frames - 1) {
		return;
	}

	for (const auto &i : m_drawlist_shadow) {
		// only process specific part of the list & break early
		++count;
		if (count <= low_bound)
			continue;
		if (count > high_bound)
			break;

		v3s16 block_pos = i.first;
		MapBlock *block = i.second;

		// If the mesh of the block happened to get deleted, ignore it
		if (!block->mesh)
            continue;*/

		/*
			Get the meshbuffers of the block
		*/
        /*if (is_transparent_pass) {
			// In transparent pass, the mesh will give us
			// the partial buffers in the correct order
			for (auto &buffer : block->mesh->getTransparentBuffers())
				draw_order.emplace_back(get_block_wpos(block_pos), &buffer);
		} else {
			// Otherwise, group them
			grouped_buffers.addFromBlock(block_pos, block->mesh, driver);
		}
	}

	for (auto &map : grouped_buffers.maps) {
		for (auto &list : map) {
			transformBuffersToDrawOrder(
				list.second, draw_order, get_block_wpos, m_dynamic_buffers);
		}
	}

	TimeTaker draw("");

	core::matrix4 m; // Model matrix
	u32 drawcall_count = 0;
	u32 vertex_count = 0;
	u32 material_swaps = 0;

	// Render all mesh buffers in order
	drawcall_count += draw_order.size();

	bool translucent_foliage = g_settings->getBool("enable_translucent_foliage");

	video::E_MATERIAL_TYPE leaves_material = video::EMT_SOLID;

	// For translucent leaves, we want to use backface culling instead of frontface.
	if (translucent_foliage) {
		// this is the material leaves would use, compare to nodedef.cpp
		auto* shdsrc = m_client->getShaderSource();
		const u32 leaves_shader = shdsrc->getShader("nodes_shader", TILE_MATERIAL_WAVING_LEAVES, NDT_ALLFACES);
		leaves_material = shdsrc->getShaderInfo(leaves_shader).material;
	}

	for (auto &descriptor : draw_order) {
		if (!descriptor.m_reuse_material) {
			// override some material properties
			video::SMaterial local_material = descriptor.getMaterial();
			// do not override culling if the original material renders both back
			// and front faces in solid mode (e.g. plantlike)
			// Transparent plants would still render shadows only from one side,
			// but this conflicts with water which occurs much more frequently
			if (is_transparent_pass || local_material.BackfaceCulling || local_material.FrontfaceCulling) {
				local_material.BackfaceCulling = material.BackfaceCulling;
				local_material.FrontfaceCulling = material.FrontfaceCulling;
			}
			if (local_material.MaterialType == leaves_material && translucent_foliage) {
				local_material.BackfaceCulling = true;
				local_material.FrontfaceCulling = false;
			}
			local_material.MaterialType = material.MaterialType;
			local_material.BlendOperation = material.BlendOperation;
			driver->setMaterial(local_material);
			++material_swaps;
		}

		m.setTranslation(descriptor.m_pos);

		driver->setTransform(video::ETS_WORLD, m);
		vertex_count += descriptor.draw(driver);
	}

	// restore the driver material state
	video::SMaterial clean;
	clean.BlendOperation = video::EBO_ADD;
	driver->setMaterial(clean); // reset material to defaults
	// FIXME: why is this here?
	driver->draw3DLine(v3f(), v3f(), img::color8(0));

	g_profiler->avg(prefix + "draw meshes [ms]", draw.stop(true));
	g_profiler->avg(prefix + "vertices drawn [#]", vertex_count);
	g_profiler->avg(prefix + "drawcalls [#]", drawcall_count);
	g_profiler->avg(prefix + "material swaps [#]", material_swaps);
}*/

/*
	Custom update draw list for the pov of shadow light.
*/
/*void ClientMap::updateDrawListShadow(v3f shadow_light_pos, v3f shadow_light_dir, float radius, float length)
{
	ScopeProfiler sp(g_profiler, "CM::updateDrawListShadow()", SPT_AVG);

	for (auto &i : m_drawlist_shadow) {
		MapBlock *block = i.second;
		block->refDrop();
	}
	m_drawlist_shadow.clear();

	// Number of blocks currently loaded by the client
	u32 blocks_loaded = 0;
	// Number of blocks with mesh in rendering range
	u32 blocks_in_range_with_mesh = 0;

	for (auto &sector_it : m_sectors) {
		const MapSector *sector = sector_it.second;
		if (!sector)
			continue;
        blocks_loaded += sector->size();*/

		/*
			Loop through blocks in sector
		*/
        /*for (const auto &entry : sector->getBlocks()) {
			MapBlock *block = entry.second.get();
			MapBlockMesh *mesh = block->mesh;
			if (!mesh) {
				// Ignore if mesh doesn't exist
				continue;
			}

			v3f block_pos = intToFloat(block->getPosRelative(), BS) + mesh->getBoundingSphereCenter();
			v3f projection = shadow_light_pos + shadow_light_dir * shadow_light_dir.dotProduct(block_pos - shadow_light_pos);
			if (projection.getDistanceFrom(block_pos) > (radius + mesh->getBoundingRadius()))
				continue;

			blocks_in_range_with_mesh++;

			// This block is in range. Reset usage timer.
			block->resetUsageTimer();

			// Add to set
			if (m_drawlist_shadow.emplace(block->getPos(), block).second) {
				block->refGrab();
			}
		}
	}

	g_profiler->avg("SHADOW MapBlock meshes in range [#]", blocks_in_range_with_mesh);
	g_profiler->avg("SHADOW MapBlocks drawn [#]", m_drawlist_shadow.size());
	g_profiler->avg("SHADOW MapBlocks loaded [#]", blocks_loaded);
}

void ClientMap::reportMetrics(u64 save_time_us, u32 saved_blocks, u32 all_blocks)
{
	g_profiler->avg("CM::reportMetrics loaded blocks [#]", all_blocks);
}*/
