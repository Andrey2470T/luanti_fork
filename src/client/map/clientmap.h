// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include "map.h"
#include "client/player/playercamera.h"

class Client;
class DistanceSortedDrawList;

/*
	ClientMap

	This is the only map class that is able to render itself on screen.
*/

class ClientMap : public Map
{
public:
    ClientMap(Client *client, DistanceSortedDrawList *drawlist);

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
    void updateShadowBlocks(const v3f &shadow_light_pos, const v3f &shadow_light_dir, f32 radius);
	// @brief Calculate statistics about the map and keep the blocks alive
	void touchMapBlocks();

    bool addActiveObject(u16 id);
    void updateMapBlocksActiveObjects();
    void removeActiveObject(u16 id);

    int getBackgroundBrightness(float max_d, u32 daylight_factor,
            int oldvalue, bool *sunlight_seen_result);

	// For debug printing
	void PrintInfo(std::ostream &out) override;

	void onSettingChanged(std::string_view name, bool all);

protected:
	void reportMetrics(u64 save_time_us, u32 saved_blocks, u32 all_blocks) override;
private:
	Client *m_client;

    DistanceSortedDrawList *m_drawlist;

    v3f m_camera_pos = v3f(0,0,0);
    v3s16 m_camera_offset;

    std::list<MapBlock*> m_visible_mapblocks;
    std::list<MapBlock*> m_visible_shadow_mapblocks;

    std::list<u16> m_pending_to_add_caos;
};
