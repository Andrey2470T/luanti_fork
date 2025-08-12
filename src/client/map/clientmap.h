// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include "map.h"
#include "client/player/playercamera.h"
#include <set>
#include <map>

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
	MapSector * emergeSector(v2s16 p) override;

	void getBlocksInViewRange(v3s16 cam_pos_nodes,
		v3s16 *p_blocks_min, v3s16 *p_blocks_max, float range=-1.0f);

    //void updateCamera(v3f pos, v3f dir, f32 fov, v3s16 offset, img::color8 light_color);

    void update();
	// @brief Calculate statistics about the map and keep the blocks alive
	void touchMapBlocks();
    //void updateDrawListShadow(v3f shadow_light_pos, v3f shadow_light_dir, float radius, float length);
	// Returns true if draw list needs updating before drawing the next frame.
    //bool needsUpdateDrawList() { return m_needs_update_drawlist; }

    //void renderMapShadows(video::IVideoDriver *driver,
    //		const video::SMaterial &material, s32 pass, int frame, int total_frames);

    //int getBackgroundBrightness(float max_d, u32 daylight_factor,
    //		int oldvalue, bool *sunlight_seen_result);

    //void renderPostFx(CameraMode cam_mode);

	void invalidateMapBlockMesh(MapBlockMesh *mesh);

	// For debug printing
	void PrintInfo(std::ostream &out) override;

    //const MapDrawControl & getControl() const { return m_control; }
    //f32 getWantedRange() const { return m_control.wanted_range; }
    //f32 getCameraFov() const { return m_camera_fov; }

	void onSettingChanged(std::string_view name, bool all);

protected:
	// use drop() instead
	virtual ~ClientMap();

	void reportMetrics(u64 save_time_us, u32 saved_blocks, u32 all_blocks) override;
private:
	Client *m_client;

    DistanceSortedDrawList *m_drawlist;

    v3f m_camera_pos = v3f(0,0,0);
    //v3f m_camera_dir = v3f(0,0,1);
    //f32 m_camera_fov = M_PI;
    v3s16 m_camera_offset;
    //img::color8 m_camera_light_color = img::white;

    std::list<MapBlock*> m_visible_mapblocks;
    //std::map<v3s16, MapBlock*> m_drawlist_shadow;
};
