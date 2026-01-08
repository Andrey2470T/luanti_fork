// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2015 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include <Image/Image.h>
#include <Render/Texture2D.h>
#include <Render/Shader.h>
#include "sprite.h"

#include "../hud.h"
#include "mapnode.h"
#include "util/thread.h"

class Client;
class NodeDefManager;
class RenderSystem;
class ResourceCache;
class Renderer;
class VoxelManipulator;

#define MINIMAP_MAX_SX 512
#define MINIMAP_MAX_SY 512

enum MinimapShape {
	MINIMAP_SHAPE_SQUARE,
	MINIMAP_SHAPE_ROUND,
};

struct MinimapModeDef {
	MinimapType type;
	std::string label;
	u16 scan_height;
	u16 map_size;
	std::string texture;
	u16 scale;
};

struct MinimapPixel {
	//! The topmost node that the minimap displays.
	MapNode n;
	u16 height;
	u16 air_count;
};

struct MinimapMapblock {
	void getMinimapNodes(VoxelManipulator *vmanip, const v3s16 &pos);

    std::array<MinimapPixel, MAP_BLOCKSIZE * MAP_BLOCKSIZE> data;
};

struct MinimapData {
	MinimapModeDef mode;
	v3s16 pos;
	v3s16 old_pos;
    std::array<MinimapPixel, MINIMAP_MAX_SX * MINIMAP_MAX_SY> minimap_scan;
    bool map_invalidated = true;
	bool minimap_shape_round;
    img::Image *minimap_mask_round = nullptr;
    img::Image *minimap_mask_square = nullptr;
    render::Texture2D *texture = nullptr;
    render::Texture2D *heightmap_texture = nullptr;
	bool textures_initialised = false; // True if the following textures are not nullptrs.
    img::Image *minimap_overlay_round = nullptr;
    img::Image *minimap_overlay_square = nullptr;
    img::Image *player_marker = nullptr;
    img::Image *object_marker_red = nullptr;
};

struct QueuedMinimapUpdate {
	v3s16 pos;
	MinimapMapblock *data = nullptr;
};

class MinimapUpdateThread : public UpdateThread {
public:
	MinimapUpdateThread() : UpdateThread("Minimap") {}
	virtual ~MinimapUpdateThread();

	void getMap(v3s16 pos, s16 size, s16 height);
	void enqueueBlock(v3s16 pos, MinimapMapblock *data);
	bool pushBlockUpdate(v3s16 pos, MinimapMapblock *data);
	bool popBlockUpdate(QueuedMinimapUpdate *update);

	MinimapData *data = nullptr;

protected:
	virtual void doUpdate();

private:
	std::mutex m_queue_mutex;
	std::deque<QueuedMinimapUpdate> m_update_queue;
	std::map<v3s16, MinimapMapblock *> m_blocks_cache;
};

class Minimap
{
public:
    Minimap(Client *_client, UIRects *_rect);
	~Minimap();

	void addBlock(v3s16 pos, MinimapMapblock *data);

	v3f getYawVec();

	void setPos(v3s16 pos);
	v3s16 getPos() const { return data->pos; }
	void setAngle(f32 angle);
	f32 getAngle() const { return m_angle; }
	void toggleMinimapShape();
	void setMinimapShape(MinimapShape shape);
	MinimapShape getMinimapShape();

	void clearModes() { m_modes.clear(); };
	void addMode(MinimapModeDef mode);
	void addMode(MinimapType type, u16 size = 0, const std::string &label = "",
			const std::string &texture = "", u16 scale = 1);

	void setModeIndex(size_t index);
	size_t getModeIndex() const { return m_current_mode_index; };
	size_t getMaxModeIndex() const { return m_modes.size() - 1; };
	void nextMode();

	MinimapModeDef getModeDef() const { return data->mode; }

    img::Image *getMinimapMask();
    render::Texture2D *getMinimapTexture();

    void blitMinimapPixelsToImageRadar(img::Image *map_image);
    void blitMinimapPixelsToImageSurface(img::Image *map_image,
        img::Image *heightmap_image);

    void addMarker(v3f pos);
    void removeMarker(v3f pos);

    void updateActiveMarkers(recti rect);
    void drawMinimap(recti rect);

	Client* client;
	std::unique_ptr<MinimapData> data;

private:
    void updateUVs(const rectf &srcRect);
    Renderer *m_renderer;
    ResourceCache *m_cache;

    UIRects *m_rect;
    std::unique_ptr<MeshBuffer> m_buffer;
    render::Shader *m_minimap_shader;

	const NodeDefManager *m_ndef;
	std::unique_ptr<MinimapUpdateThread> m_minimap_update_thread;
	std::vector<MinimapModeDef> m_modes;
    size_t m_current_mode_index = 0;
	u16 m_surface_mode_scan_height;
    f32 m_angle = 0.0f;
	std::mutex m_mutex;
    std::list<v3f> m_markers;
	std::list<v2f> m_active_markers;
};
