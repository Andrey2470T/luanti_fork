// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "inventory.h"
#include "util/numeric.h"
#include "client/player/localplayer.h"
#include "client/render/camera.h"
#include <array>
#include <list>
#include <optional>

class LocalPlayer;
struct MapDrawControl;
class Client;
class RenderingEngine;
class WieldMeshSceneNode;

struct Nametag
{
	std::string text;
    img::color8 textcolor;
    std::optional<img::color8> bgcolor;
	v3f pos;

    Nametag(
            const std::string &text,
            const img::color8 &textcolor,
            const std::optional<img::color8> &bgcolor,
            const v3f &pos):
		text(text),
		textcolor(textcolor),
		bgcolor(bgcolor),
		pos(pos)
	{
	}

    img::color8 getBgColor(bool use_fallback) const
	{
		if (bgcolor)
			return bgcolor.value();
		else if (!use_fallback)
            return img::color8();
        else if (textcolor.getLuminance() > 186)
			// Dark background for light text
            return img::color8(img::PF_RGBA8, 50, 50, 50, 50);
		else
			// Light background for dark text
            return img::color8(img::PF_RGBA8, 50, 255, 255, 255);
	}
};

enum CameraMode {CAMERA_MODE_FIRST, CAMERA_MODE_THIRD, CAMERA_MODE_THIRD_FRONT};

/*
	Client camera class, manages the player and camera scene nodes, the viewing distance
	and performs view bobbing etc. It also displays the wielded tool in front of the
	first-person camera.
*/
class PlayerCamera : public Camera
{
public:
	PlayerCamera(MapDrawControl &draw_control, Client *client, RenderingEngine *rendering_engine);
	~PlayerCamera();

	// Returns the absolute position of the head SceneNode in the world
	inline v3f getHeadPosition() const
	{
        return m_playerbase_pos + m_head_offset;
	}

	// Returns a lambda that when called with an object's position and bounding-sphere
	// radius (both in BS space) returns true if, and only if the object should be
	// frustum-culled.
    bool doFrustumCull(const v3f &position, f32 radius) const
	{
        return m_frustum.frustumCull(position-intToFloat(m_offset, BS), radius);
	}

	// Notify about new server-sent FOV and initialize smooth FOV transition
	void notifyFovChange();

	// Step the camera: updates the viewing range and view bobbing.
	void step(f32 dtime);

	// Update the camera from the local player's position.
	void update(LocalPlayer* player, f32 frametime, f32 tool_reload_ratio);

	// Update render distance
	void updateViewingRange();

	// Start digging animation
	// Pass 0 for left click, 1 for right click
	void setDigging(s32 button);

	// Replace the wielded item mesh
	void wield(const ItemStack &item);

	// Draw the wielded tool.
	// This has to happen *after* the main scene is drawn.
	// Warning: This clears the Z buffer.
    void drawWieldedTool(matrix4* translation=nullptr);

	// Toggle the current camera mode
	void toggleCameraMode() {
		if (m_camera_mode == CAMERA_MODE_FIRST)
			m_camera_mode = CAMERA_MODE_THIRD;
		else if (m_camera_mode == CAMERA_MODE_THIRD)
			m_camera_mode = CAMERA_MODE_THIRD_FRONT;
		else
			m_camera_mode = CAMERA_MODE_FIRST;
	}

	// Set the current camera mode
	inline void setCameraMode(CameraMode mode)
	{
		m_camera_mode = mode;
	}

	//read the current camera mode
	inline CameraMode getCameraMode()
	{
		return m_camera_mode;
	}

    Nametag *addNametag(
        const std::string &text, img::color8 textcolor,
        std::optional<img::color8> bgcolor, const v3f &pos);

	void removeNametag(Nametag *nametag);

	void drawNametags();

	inline void addArmInertia(f32 player_yaw);

private:
    //scene::ISceneManager *m_wieldmgr = nullptr;
    WieldMeshSceneNode *m_wieldnode = nullptr;

	// draw control
	MapDrawControl& m_draw_control;

	Client *m_client;

	// Default Client FOV (as defined by the "fov" setting)
	f32 m_cache_fov;

    v3f m_playerbase_pos;
    v3f m_head_offset;
    v3f m_head_rotation;

	bool m_stepheight_smooth_active = false;

	// Server-sent FOV variables
	bool m_server_sent_fov = false;
	f32 m_curr_fov_degrees, m_target_fov_degrees;

	// FOV transition variables
	bool m_fov_transition_active = false;
	f32 m_fov_diff, m_transition_time;

	v2f m_wieldmesh_offset = v2f(55.0f, -35.0f);
	v2f m_arm_dir;
	v2f m_cam_vel;
	v2f m_cam_vel_old;
	v2f m_last_cam_pos;

	// View bobbing animation frame (0 <= m_view_bobbing_anim < 1)
	f32 m_view_bobbing_anim = 0.0f;
	// If 0, view bobbing is off (e.g. player is standing).
	// If 1, view bobbing is on (player is walking).
	// If 2, view bobbing is getting switched off.
	s32 m_view_bobbing_state = 0;
	// Speed of view bobbing animation
	f32 m_view_bobbing_speed = 0.0f;
	// Fall view bobbing
	f32 m_view_bobbing_fall = 0.0f;

	// Digging animation frame (0 <= m_digging_anim < 1)
	f32 m_digging_anim = 0.0f;
	// If -1, no digging animation
	// If 0, left-click digging animation
	// If 1, right-click digging animation
	s32 m_digging_button = -1;

	// Animation when changing wielded item
	f32 m_wield_change_timer = 0.125f;
	ItemStack m_wield_item_next;

	CameraMode m_camera_mode = CAMERA_MODE_FIRST;

	f32 m_cache_fall_bobbing_amount;
	f32 m_cache_view_bobbing_amount;
	bool m_arm_inertia;

	std::list<Nametag *> m_nametags;
	bool m_show_nametag_backgrounds;

	// Last known light color of the player
    img::color8 m_player_light_color;
};
