// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include "map.h"
#include "client/player/playercamera.h"

class Client;
class DistanceSortedDrawList;
class MeshUpdateManager;
class LayeredMesh;

/*
	ClientMap

	This is the only map class that is able to render itself on screen.
*/

class ClientMap : public Map
{
public:
    ClientMap(Client *client, DistanceSortedDrawList *drawlist);
    ~ClientMap();

	bool maySaveBlocks() override
	{
		return false;
	}

	/*
		Forcefully get a sector from somewhere
	*/
    MapSector *emergeSector(v2s16 p) override;

    void getBlocksInViewRange(v3s16 cam_pos_nodes,
        v3s16 *p_blocks_min, v3s16 *p_blocks_max, float range=-1.0f);

    void update();
    //void updateShadowBlocks(const v3f &shadow_light_pos, const v3f &shadow_light_dir, f32 radius);

    bool addActiveObject(u16 id);
    void updateMapBlocksActiveObjects();
    void removeActiveObject(u16 id);

    int getBackgroundBrightness(float max_d, u32 daylight_factor,
            int oldvalue, bool *sunlight_seen_result);

    MeshGrid getMeshGrid()
    {
        return m_mesh_grid;
    }

    void pushToDeletedMeshes(LayeredMesh *mesh)
    {
        m_delete_meshes.emplace(mesh);
    }

    void startMeshUpdate();
    void stopMeshUpdate();
    bool isMeshUpdateRunning() const;

    void step(f32 dtime);

    void addUpdateMeshTask(v3s16 blockpos, bool ack_to_server=false, bool urgent=false);
    // Including blocks at appropriate edges
    void addUpdateMeshTaskWithEdge(v3s16 blockpos, bool ack_to_server=false, bool urgent=false);
    void addUpdateMeshTaskForNode(v3s16 nodepos, bool ack_to_server=false, bool urgent=false);

    void addNode(v3s16 p, MapNode n, bool remove_metadata = true);
    // Causes urgent mesh updates (unlike Map::add/removeNodeWithEvent)
    void removeNode(v3s16 p);

	// For debug printing
    void PrintInfo(std::ostream &out) override
    {
        out<<"ClientMap: ";
    }

	void onSettingChanged(std::string_view name, bool all);

protected:
	void reportMetrics(u64 save_time_us, u32 saved_blocks, u32 all_blocks) override;
private:
	Client *m_client;

    std::unique_ptr<MeshUpdateManager> m_mesh_update_manager;

    std::set<u16> m_pending_to_add_caos;

    // The number of blocks the client will combine for mesh generation.
    MeshGrid m_mesh_grid;

    IntervalLimiter m_map_timer_and_unload_interval;

    f32 m_unload_unused_data_timeout;
    s32 m_mapblock_limit;

    const f32 update_draw_list_delta = 0.2f;
    f32 update_draw_list_timer = 0.0f;

    std::set<LayeredMesh *> m_add_meshes;
    std::set<LayeredMesh *> m_delete_meshes;
};
