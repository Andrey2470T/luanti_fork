// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "inventory.h"
#include "util/numeric.h"
#include "client/player/localplayer.h"
#include "client/render/camera.h"
#include <list>
#include <optional>

class LocalPlayer;
struct DrawControl;
class Client;
//class WieldMeshSceneNode;

enum CameraMode {
    CAMERA_MODE_FIRST,
    CAMERA_MODE_THIRD,
    CAMERA_MODE_THIRD_FRONT
};

/*
	Client camera class, manages the player and camera scene nodes, the viewing distance
	and performs view bobbing etc. It also displays the wielded tool in front of the
	first-person camera.
*/
class PlayerCamera : public Camera
{
public:
    PlayerCamera(Client *client);
	~PlayerCamera();

    v2f getOrientation() const
    {
        return m_orientation;
    }

	// Returns the absolute position of the head SceneNode in the world
    v3f getHeadPosition() const
	{
        return m_playerbase_pos + m_head_offset;
	}

	// Notify about new server-sent FOV and initialize smooth FOV transition
	void notifyFovChange();

	// Step the camera: updates the viewing range and view bobbing.
	void step(f32 dtime);

	// Update the camera from the local player's position.
    void update(f32 dtime);

	// Update render distance
	void updateViewingRange();
    f32 getSensitivityScaleFactor() const;
    void updateOrientation(bool invert_mouse, f32 mouse_sensitivity,
        bool enable_joysticks, f32 joystick_frustum_sensitivity, f32 dtime);

	// Start digging animation
	// Pass 0 for left click, 1 for right click
	//void setDigging(s32 button);

	// Replace the wielded item mesh
	//void wield(const ItemStack &item);

	// Draw the wielded tool.
	// This has to happen *after* the main scene is drawn.
	// Warning: This clears the Z buffer.
    //void drawWieldedTool(matrix4* translation=nullptr);

	// Toggle the current camera mode
	void toggleCameraMode() {
		if (m_camera_mode == CAMERA_MODE_FIRST)
			m_camera_mode = CAMERA_MODE_THIRD;
		else if (m_camera_mode == CAMERA_MODE_THIRD)
			m_camera_mode = CAMERA_MODE_THIRD_FRONT;
		else
			m_camera_mode = CAMERA_MODE_FIRST;
	}

    void setOrientation(const v2f new_orientation)
    {
        m_orientation = new_orientation;
    }
	// Set the current camera mode
    void setCameraMode(CameraMode mode)
	{
		m_camera_mode = mode;
	}

	//read the current camera mode
    CameraMode getCameraMode()
	{
		return m_camera_mode;
	}

    //void addArmInertia(f32 player_yaw);

private:
    //scene::ISceneManager *m_wieldmgr = nullptr;
    //WieldMeshSceneNode *m_wieldnode = nullptr;

	// draw control
    DrawControl& m_draw_control;

	Client *m_client;

    v2f m_orientation; // {pitch, yaw}

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
};
