// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "playercamera.h"
#include "debug.h"
#include "config.h"
#include "map.h"
#include "client/map/clientmap.h"     // MapDrawControl
#include <cmath>
#include "client/render/renderingengine.h"
#include "client/ao/genericcao.h"
#include "settings.h"
#include "client/render/wieldmesh.h"
#include "noise.h"         // easeCurve
#include "client/event/eventsystem.h"
#include "nodedef.h"
#include "util/numeric.h"
#include "constants.h"
#include "client/ui/fontengine.h"
#include "script/scripting_client.h"
#include "gettext.h"
#include "Image/Converting.h"
#include "Utils/Quaternion.h"
#include "Utils/MathFuncs.h"

#define CAMERA_OFFSET_STEP 200
#define WIELDMESH_OFFSET_X 55.0f
#define WIELDMESH_OFFSET_Y -35.0f
#define WIELDMESH_AMPLITUDE_X 7.0f
#define WIELDMESH_AMPLITUDE_Y 10.0f

PlayerCamera::PlayerCamera(MapDrawControl &draw_control, Client *client, RenderingEngine *rendering_engine):
	Camera(), m_draw_control(draw_control),
    m_client(client), m_player_light_color(img::colorU32NumberToObject(0xFFFFFFFF))
{
    //auto smgr = rendering_engine->get_scene_manager();

    //m_playerbase_pos = client->getLocalPlayer()->getPosition();
	// This needs to be in its own scene manager. It is drawn after
	// all other 3D scene nodes and before the GUI.
    //m_wieldmgr = smgr->createNewSceneManager();
    //m_wieldmgr->addCameraSceneNode();
    //m_wieldnode = new WieldMeshSceneNode(m_wieldmgr, -1);
    //m_wieldnode->setItem(ItemStack(), m_client);
    //m_wieldnode->drop(); // m_wieldmgr grabbed it

	/* TODO: Add a callback function so these can be updated when a setting
	 *       changes.  At this point in time it doesn't matter (e.g. /set
	 *       is documented to change server settings only)
	 *
	 * TODO: Local caching of settings is not optimal and should at some stage
	 *       be updated to use a global settings object for getting thse values
	 *       (as opposed to the this local caching). This can be addressed in
	 *       a later release.
	 */
	m_cache_fall_bobbing_amount = g_settings->getFloat("fall_bobbing_amount", 0.0f, 100.0f);
	m_cache_view_bobbing_amount = g_settings->getFloat("view_bobbing_amount", 0.0f, 7.9f);
	// 45 degrees is the lowest FOV that doesn't cause the server to treat this
	// as a zoom FOV and load world beyond the set server limits.
	m_cache_fov                 = g_settings->getFloat("fov", 45.0f, 160.0f);
	m_arm_inertia               = g_settings->getBool("arm_inertia");
	m_nametags.clear();
	m_show_nametag_backgrounds  = g_settings->getBool("show_nametag_backgrounds");
}

PlayerCamera::~PlayerCamera()
{
	m_wieldmgr->drop();
}

void PlayerCamera::notifyFovChange()
{
	LocalPlayer *player = m_client->getEnv().getLocalPlayer();
	assert(player);

	PlayerFovSpec spec = player->getFov();

	// Remember old FOV in case a transition is wanted
	f32 m_old_fov_degrees = m_fov_transition_active
		? m_curr_fov_degrees // FOV is overridden with transition
		: m_server_sent_fov
			? m_target_fov_degrees // FOV is overridden without transition
			: m_cache_fov; // FOV is not overridden

	m_server_sent_fov = spec.fov > 0.0f;
	m_target_fov_degrees = m_server_sent_fov
		? spec.is_multiplier
			? m_cache_fov * spec.fov // apply multiplier to client-set FOV
			: spec.fov // absolute override
		: m_cache_fov; // reset to client-set FOV

	m_fov_transition_active = spec.transition_time > 0.0f;
	if (m_fov_transition_active) {
		m_transition_time = spec.transition_time;
		m_fov_diff = m_target_fov_degrees - m_old_fov_degrees;
	}
}

// Returns the fractional part of x
inline f32 my_modf(f32 x)
{
	float dummy;
	return std::modf(x, &dummy);
}

void PlayerCamera::step(f32 dtime)
{
	if(m_view_bobbing_fall > 0)
	{
		m_view_bobbing_fall -= 3 * dtime;
		if(m_view_bobbing_fall <= 0)
			m_view_bobbing_fall = -1; // Mark the effect as finished
	}

	bool was_under_zero = m_wield_change_timer < 0;
	m_wield_change_timer = MYMIN(m_wield_change_timer + dtime, 0.125);

	if (m_wield_change_timer >= 0 && was_under_zero) {
		m_wieldnode->setItem(m_wield_item_next, m_client);
		m_wieldnode->setNodeLightColor(m_player_light_color);
	}

	if (m_view_bobbing_state != 0)
	{
		//f32 offset = dtime * m_view_bobbing_speed * 0.035;
		f32 offset = dtime * m_view_bobbing_speed * 0.030;
		if (m_view_bobbing_state == 2) {
			// Animation is getting turned off
			if (m_view_bobbing_anim < 0.25) {
				m_view_bobbing_anim -= offset;
			} else if (m_view_bobbing_anim > 0.75) {
				m_view_bobbing_anim += offset;
			} else if (m_view_bobbing_anim < 0.5) {
				m_view_bobbing_anim += offset;
				if (m_view_bobbing_anim > 0.5)
					m_view_bobbing_anim = 0.5;
			} else {
				m_view_bobbing_anim -= offset;
				if (m_view_bobbing_anim < 0.5)
					m_view_bobbing_anim = 0.5;
			}

			if (m_view_bobbing_anim <= 0 || m_view_bobbing_anim >= 1 ||
					fabs(m_view_bobbing_anim - 0.5) < 0.01) {
				m_view_bobbing_anim = 0;
				m_view_bobbing_state = 0;
			}
		}
		else {
			float was = m_view_bobbing_anim;
			m_view_bobbing_anim = my_modf(m_view_bobbing_anim + offset);
			bool step = (was == 0 ||
					(was < 0.5f && m_view_bobbing_anim >= 0.5f) ||
					(was > 0.5f && m_view_bobbing_anim <= 0.5f));
			if(step) {
				m_client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::VIEW_BOBBING_STEP));
			}
		}
	}

	if (m_digging_button != -1) {
		f32 offset = dtime * 3.5f;
		float m_digging_anim_was = m_digging_anim;
		m_digging_anim += offset;
		if (m_digging_anim >= 1)
		{
			m_digging_anim = 0;
			m_digging_button = -1;
		}
		float lim = 0.15;
		if(m_digging_anim_was < lim && m_digging_anim >= lim)
		{
			if (m_digging_button == 0) {
				m_client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::CAMERA_PUNCH_LEFT));
			} else if(m_digging_button == 1) {
				m_client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::CAMERA_PUNCH_RIGHT));
			}
		}
	}
}

static inline v2f dir(const v2f &pos_dist)
{
	f32 x = pos_dist.X - WIELDMESH_OFFSET_X;
	f32 y = pos_dist.Y - WIELDMESH_OFFSET_Y;

	f32 x_abs = std::fabs(x);
	f32 y_abs = std::fabs(y);

	if (x_abs >= y_abs) {
		y *= (1.0f / x_abs);
		x /= x_abs;
	}

	if (y_abs >= x_abs) {
		x *= (1.0f / y_abs);
		y /= y_abs;
	}

	return v2f(std::fabs(x), std::fabs(y));
}

void PlayerCamera::addArmInertia(f32 player_yaw)
{
	m_cam_vel.X = std::fabs(rangelim(m_last_cam_pos.X - player_yaw,
		-100.0f, 100.0f) / 0.016f) * 0.01f;
    m_cam_vel.Y = std::fabs((m_last_cam_pos.Y - m_direction.Y) / 0.016f);
	f32 gap_X = std::fabs(WIELDMESH_OFFSET_X - m_wieldmesh_offset.X);
	f32 gap_Y = std::fabs(WIELDMESH_OFFSET_Y - m_wieldmesh_offset.Y);

	if (m_cam_vel.X > 1.0f || m_cam_vel.Y > 1.0f) {
		/*
		    The arm moves relative to the camera speed,
		    with an acceleration factor.
		*/

		if (m_cam_vel.X > 1.0f) {
			if (m_cam_vel.X > m_cam_vel_old.X)
				m_cam_vel_old.X = m_cam_vel.X;

			f32 acc_X = 0.12f * (m_cam_vel.X - (gap_X * 0.1f));
			m_wieldmesh_offset.X += m_last_cam_pos.X < player_yaw ? acc_X : -acc_X;

			if (m_last_cam_pos.X != player_yaw)
				m_last_cam_pos.X = player_yaw;

			m_wieldmesh_offset.X = rangelim(m_wieldmesh_offset.X,
				WIELDMESH_OFFSET_X - (WIELDMESH_AMPLITUDE_X * 0.5f),
				WIELDMESH_OFFSET_X + (WIELDMESH_AMPLITUDE_X * 0.5f));
		}

		if (m_cam_vel.Y > 1.0f) {
			if (m_cam_vel.Y > m_cam_vel_old.Y)
				m_cam_vel_old.Y = m_cam_vel.Y;

			f32 acc_Y = 0.12f * (m_cam_vel.Y - (gap_Y * 0.1f));
			m_wieldmesh_offset.Y +=
                m_last_cam_pos.Y > m_direction.Y ? acc_Y : -acc_Y;

            if (m_last_cam_pos.Y != m_direction.Y)
                m_last_cam_pos.Y = m_direction.Y;

			m_wieldmesh_offset.Y = rangelim(m_wieldmesh_offset.Y,
				WIELDMESH_OFFSET_Y - (WIELDMESH_AMPLITUDE_Y * 0.5f),
				WIELDMESH_OFFSET_Y + (WIELDMESH_AMPLITUDE_Y * 0.5f));
		}

		m_arm_dir = dir(m_wieldmesh_offset);
	} else {
		/*
		    Now the arm gets back to its default position when the camera stops,
		    following a vector, with a smooth deceleration factor.
		*/

		f32 dec_X = 0.35f * (std::min(15.0f, m_cam_vel_old.X) * (1.0f +
			(1.0f - m_arm_dir.X))) * (gap_X / 20.0f);

		f32 dec_Y = 0.25f * (std::min(15.0f, m_cam_vel_old.Y) * (1.0f +
			(1.0f - m_arm_dir.Y))) * (gap_Y / 15.0f);

		if (gap_X < 0.1f)
			m_cam_vel_old.X = 0.0f;

		m_wieldmesh_offset.X -=
			m_wieldmesh_offset.X > WIELDMESH_OFFSET_X ? dec_X : -dec_X;

		if (gap_Y < 0.1f)
			m_cam_vel_old.Y = 0.0f;

		m_wieldmesh_offset.Y -=
			m_wieldmesh_offset.Y > WIELDMESH_OFFSET_Y ? dec_Y : -dec_Y;
	}
}

void PlayerCamera::update(LocalPlayer* player, f32 frametime, f32 tool_reload_ratio)
{
	// Get player position
	// Smooth the movement when walking up stairs
    v3f old_player_position = m_playerbase_pos;
	v3f player_position = player->getPosition();

	f32 yaw = player->getYaw();
	f32 pitch = player->getPitch();

	// This is worse than `LocalPlayer::getPosition()` but
	// mods expect the player head to be at the parent's position
	// plus eye height.
	if (player->getParent())
		player_position = player->getParent()->getPosition();

	// Smooth the camera movement after the player instantly moves upward due to stepheight.
	// The smoothing usually continues until the camera position reaches the player position.
	float player_stepheight = player->getCAO() ? player->getCAO()->getStepHeight() : HUGE_VALF;
	float upward_movement = player_position.Y - old_player_position.Y;
	if (upward_movement < 0.01f || upward_movement > player_stepheight) {
		m_stepheight_smooth_active = false;
	} else if (player->touching_ground) {
		m_stepheight_smooth_active = true;
	}
	if (m_stepheight_smooth_active) {
		f32 oldy = old_player_position.Y;
		f32 newy = player_position.Y;
		f32 t = std::exp(-23 * frametime);
		player_position.Y = oldy * t + newy * (1-t);
	}

    m_playerbase_pos = player_position;

	// Get camera tilt timer (hurt animation)
	float cameratilt = fabs(fabs(player->hurt_tilt_timer-0.75)-0.75);

	// Fall bobbing animation
	float fall_bobbing = 0;
	if(player->camera_impact >= 1 && m_camera_mode < CAMERA_MODE_THIRD)
	{
		if(m_view_bobbing_fall == -1) // Effect took place and has finished
			player->camera_impact = m_view_bobbing_fall = 0;
		else if(m_view_bobbing_fall == 0) // Initialize effect
			m_view_bobbing_fall = 1;

		// Convert 0 -> 1 to 0 -> 1 -> 0
		fall_bobbing = m_view_bobbing_fall < 0.5 ? m_view_bobbing_fall * 2 : -(m_view_bobbing_fall - 0.5) * 2 + 1;
		// Smoothen and invert the above
		fall_bobbing = sin(fall_bobbing * 0.5 * M_PI) * -1;
		// Amplify according to the intensity of the impact
		if (player->camera_impact > 0.0f)
			fall_bobbing *= (1 - rangelim(50 / player->camera_impact, 0, 1)) * 5;

		fall_bobbing *= m_cache_fall_bobbing_amount;
	}

	// Calculate and translate the head SceneNode offsets
    v3f eye_offset = player->getEyeOffset();
    switch(m_camera_mode) {
    case CAMERA_MODE_FIRST:
        eye_offset += player->eye_offset_first;
        break;
    case CAMERA_MODE_THIRD:
        eye_offset += player->eye_offset_third;
        break;
    case CAMERA_MODE_THIRD_FRONT:
        eye_offset.X += player->eye_offset_third_front.X;
        eye_offset.Y += player->eye_offset_third_front.Y;
        eye_offset.Z -= player->eye_offset_third_front.Z;
        break;
    }

    // Set head node transformation
    eye_offset.Y += cameratilt * -player->hurt_tilt_strength + fall_bobbing;
    m_head_offset = eye_offset;
    m_head_rotation = v3f(pitch, 0,
           cameratilt * player->hurt_tilt_strength);

	// Compute relative camera position and target
	v3f rel_cam_pos = v3f(0,0,0);
	v3f rel_cam_target = v3f(0,0,1);
	v3f rel_cam_up = v3f(0,1,0);

	if (m_cache_view_bobbing_amount != 0.0f && m_view_bobbing_anim != 0.0f &&
		m_camera_mode < CAMERA_MODE_THIRD) {
		f32 bobfrac = my_modf(m_view_bobbing_anim * 2);
		f32 bobdir = (m_view_bobbing_anim < 0.5) ? 1.0 : -1.0;

		f32 bobknob = 1.2;
		f32 bobtmp = std::sin(std::pow(bobfrac, bobknob) * M_PI);

		v3f bobvec = v3f(
			0.3 * bobdir * std::sin(bobfrac * M_PI),
			-0.28 * bobtmp * bobtmp,
			0.);

		rel_cam_pos += bobvec * m_cache_view_bobbing_amount;
		rel_cam_target += bobvec * m_cache_view_bobbing_amount;
		rel_cam_up.rotateXYBy(-0.03 * bobdir * bobtmp * M_PI * m_cache_view_bobbing_amount);
	}

	// Compute absolute camera position and target
    m_position = m_playerbase_pos + m_head_offset + rel_cam_pos;
    //m_headnode->getAbsoluteTransformation().transformVect(m_camera_position, rel_cam_pos);
    matrix4 rot_transform;
    rot_transform.buildRotateFromTo(v3f(0,0,1), m_direction);
    rot_transform.transformVect(m_direction, rel_cam_target - rel_cam_pos);
    //m_camera_direction = m_headnode->getAbsoluteTransformation()
    //		.rotateAndScaleVect(rel_cam_target - rel_cam_pos);

    v3f abs_cam_up;
    rot_transform.transformVect(abs_cam_up, rel_cam_up);
    //v3f abs_cam_up = m_headnode->getAbsoluteTransformation()
    //		.rotateAndScaleVect(rel_cam_up);

	// Separate camera position for calculation
    v3f my_cp = m_position;

	// Reposition the camera for third person view
	if (m_camera_mode > CAMERA_MODE_FIRST)
	{
		if (m_camera_mode == CAMERA_MODE_THIRD_FRONT)
            m_direction *= -1;

		my_cp.Y += 2;

		// Calculate new position
		bool abort = false;
		for (int i = BS; i <= BS * 2.75; i++) {
            my_cp.X = m_position.X + m_direction.X * -i;
            my_cp.Z = m_position.Z + m_direction.Z * -i;
			if (i > 12)
                my_cp.Y = m_position.Y + (m_direction.Y * -i);

			// Prevent camera positioned inside nodes
			const NodeDefManager *nodemgr = m_client->ndef();
			MapNode n = m_client->getEnv().getClientMap()
				.getNode(floatToInt(my_cp, BS));

			const ContentFeatures& features = nodemgr->get(n);
			if (features.walkable) {
                my_cp.X += m_direction.X*-1*-BS/2;
                my_cp.Z += m_direction.Z*-1*-BS/2;
                my_cp.Y += m_direction.Y*-1*-BS/2;
				abort = true;
				break;
			}
		}

		// If node blocks camera position don't move y to heigh
		if (abort && my_cp.Y > player_position.Y+BS*2)
			my_cp.Y = player_position.Y+BS*2;
	}

	// Update offset if too far away from the center of the map
    m_offset.X += CAMERA_OFFSET_STEP*
            (((s16)(my_cp.X/BS) - m_offset.X)/CAMERA_OFFSET_STEP);
    m_offset.Y += CAMERA_OFFSET_STEP*
            (((s16)(my_cp.Y/BS) - m_offset.Y)/CAMERA_OFFSET_STEP);
    m_offset.Z += CAMERA_OFFSET_STEP*
            (((s16)(my_cp.Z/BS) - m_offset.Z)/CAMERA_OFFSET_STEP);

    // Set camera node transformation
    m_position -= intToFloat(m_offset, BS);
    //m_cameranode->setPosition(my_cp-intToFloat(m_camera_offset, BS));
    //m_cameranode->updateAbsolutePosition();
    m_up_vector = abs_cam_up;
    //m_cameranode->setUpVector(abs_cam_up);
	// *100.0 helps in large map coordinates
    setDirection(my_cp-intToFloat(m_offset, BS) + m_direction * 100);
    //m_cameranode->setTarget(my_cp-intToFloat(m_camera_offset, BS) + 100 * m_camera_direction);

	// update the camera position in third-person mode to render blocks behind player
	// and correctly apply liquid post FX.
	if (m_camera_mode != CAMERA_MODE_FIRST)
        m_position = my_cp;

	/*
	 * Apply server-sent FOV, instantaneous or smooth transition.
	 * If not, check for zoom and set to zoom FOV.
	 * Otherwise, default to m_cache_fov.
	 */
	if (m_fov_transition_active) {
		// Smooth FOV transition
		// Dynamically calculate FOV delta based on frametimes
		f32 delta = (frametime / m_transition_time) * m_fov_diff;
		m_curr_fov_degrees += delta;

		// Mark transition as complete if target FOV has been reached
		if ((m_fov_diff > 0.0f && m_curr_fov_degrees >= m_target_fov_degrees) ||
				(m_fov_diff < 0.0f && m_curr_fov_degrees <= m_target_fov_degrees)) {
			m_fov_transition_active = false;
			m_curr_fov_degrees = m_target_fov_degrees;
		}
	} else if (m_server_sent_fov) {
		// Instantaneous FOV change
		m_curr_fov_degrees = m_target_fov_degrees;
	} else if (player->getPlayerControl().zoom && player->getZoomFOV() > 0.001f) {
		// Player requests zoom, apply zoom FOV
		m_curr_fov_degrees = player->getZoomFOV();
	} else {
		// Set to client's selected FOV
		m_curr_fov_degrees = m_cache_fov;
	}
	m_curr_fov_degrees = rangelim(m_curr_fov_degrees, 1.0f, 160.0f);

	// FOV and aspect ratio
    const v2u &window_size = RenderingEngine::getWindowSize();
    m_frustum.Aspect = (f32) window_size.X / (f32) window_size.Y;
    m_frustum.Fovy = m_curr_fov_degrees * M_PI / 180.0;
	// Increase vertical FOV on lower aspect ratios (<16:10)
    m_frustum.Fovy *= std::clamp(sqrt(16./10. / m_frustum.Aspect), 1.0, 1.4);
    m_frustum.Fovx = 2 * atan(m_frustum.Aspect * tan(0.5 * m_frustum.Fovy));

	// Make new matrices and frustum
    updateMatrices();

	if (m_arm_inertia)
		addArmInertia(yaw);

	// Position the wielded item
	v3f wield_position = v3f(m_wieldmesh_offset.X, m_wieldmesh_offset.Y, 65);
	v3f wield_rotation = v3f(-100, 120, -100);
	wield_position.Y += std::abs(m_wield_change_timer)*320 - 40;
	if(m_digging_anim < 0.05 || m_digging_anim > 0.5)
	{
		f32 frac = 1.0;
		if(m_digging_anim > 0.5)
			frac = 2.0 * (m_digging_anim - 0.5);
		// This value starts from 1 and settles to 0
		f32 ratiothing = std::pow((1.0f - tool_reload_ratio), 0.5f);
		f32 ratiothing2 = (easeCurve(ratiothing*0.5))*2.0;
		wield_position.Y -= frac * 25.0f * std::pow(ratiothing2, 1.7f);
		wield_position.X -= frac * 35.0f * std::pow(ratiothing2, 1.1f);
		wield_rotation.Y += frac * 70.0f * std::pow(ratiothing2, 1.4f);
	}
	if (m_digging_button != -1)
	{
		f32 digfrac = m_digging_anim;
		wield_position.X -= 50 * std::sin(std::pow(digfrac, 0.8f) * M_PI);
		wield_position.Y += 24 * std::sin(digfrac * 1.8 * M_PI);
		wield_position.Z += 25 * 0.5;

		// Euler angles are PURE EVIL, so why not use quaternions?
        Quaternion quat_begin(wield_rotation * DEGTORAD);
        Quaternion quat_end(v3f(80, 30, 100) * DEGTORAD);
        Quaternion quat_slerp;
		quat_slerp.slerp(quat_begin, quat_end, std::sin(digfrac * M_PI));
		quat_slerp.toEuler(wield_rotation);
        wield_rotation *= RADTODEG;
	} else {
		f32 bobfrac = my_modf(m_view_bobbing_anim);
		wield_position.X -= std::sin(bobfrac*M_PI*2.0) * 3.0;
		wield_position.Y += std::sin(my_modf(bobfrac*2.0)*M_PI) * 3.0;
	}
	m_wieldnode->setPosition(wield_position);
	m_wieldnode->setRotation(wield_rotation);

	m_player_light_color = player->light_color;
	m_wieldnode->setNodeLightColor(m_player_light_color);

	// Set render distance
	updateViewingRange();

	// If the player is walking, swimming, or climbing,
	// view bobbing is enabled and free_move is off,
	// start (or continue) the view bobbing animation.
	const v3f &speed = player->getSpeed();
	const bool movement_XZ = std::hypot(speed.X, speed.Z) > BS;
	const bool movement_Y = std::abs(speed.Y) > BS;

	const bool walking = movement_XZ && player->touching_ground;
	const bool swimming = (movement_XZ || player->swimming_vertical) && player->in_liquid;
	const bool climbing = movement_Y && player->is_climbing;
	const bool flying = g_settings->getBool("free_move")
		&& m_client->checkLocalPrivilege("fly");
	if ((walking || swimming || climbing) && !flying) {
		// Start animation
		m_view_bobbing_state = 1;
		m_view_bobbing_speed = MYMIN(speed.getLength(), 70);
	} else if (m_view_bobbing_state == 1) {
		// Stop animation
		m_view_bobbing_state = 2;
		m_view_bobbing_speed = 60;
	}
}

void PlayerCamera::updateViewingRange()
{
	f32 viewing_range = g_settings->getFloat("viewing_range");

    setNearValue(0.1f * BS);

	m_draw_control.wanted_range = std::fmin(adjustDist(viewing_range, getFovMax()), 4000);
	if (m_draw_control.range_all) {
        setFarValue(100000.0);
		return;
	}
    setFarValue((viewing_range < 2000) ? 2000 * BS : viewing_range * BS);
}

void PlayerCamera::setDigging(s32 button)
{
	if (m_digging_button == -1)
		m_digging_button = button;
}

void PlayerCamera::wield(const ItemStack &item)
{
	if (item.name != m_wield_item_next.name ||
			item.metadata != m_wield_item_next.metadata) {
		m_wield_item_next = item;
		if (m_wield_change_timer > 0)
			m_wield_change_timer = -m_wield_change_timer;
		else if (m_wield_change_timer == 0)
			m_wield_change_timer = -0.001;
	}
}

void PlayerCamera::drawWieldedTool(matrix4* translation)
{
	// Clear Z buffer so that the wielded tool stays in front of world geometry
    /*m_wieldmgr->getVideoDriver()->clearBuffers(video::ECBF_DEPTH);

	// Draw the wielded node (in a separate scene manager)
	scene::ICameraSceneNode* cam = m_wieldmgr->getActiveCamera();
	cam->setAspectRatio(m_cameranode->getAspectRatio());
	cam->setFOV(72.0*M_PI/180.0);
	cam->setNearValue(10);
	cam->setFarValue(1000);
	if (translation != NULL)
	{
		irr::core::matrix4 startMatrix = cam->getAbsoluteTransformation();
		irr::core::vector3df focusPoint = (cam->getTarget()
				- cam->getAbsolutePosition()).setLength(1)
				+ cam->getAbsolutePosition();

		irr::core::vector3df camera_pos =
				(startMatrix * *translation).getTranslation();
		cam->setPosition(camera_pos);
		cam->updateAbsolutePosition();
		cam->setTarget(focusPoint);
	}
    m_wieldmgr->drawAll();*/
}

void PlayerCamera::drawNametags()
{
    matrix4 trans = getProjectionMatrix();
    trans *= getViewMatrix();

	gui::IGUIFont *font = g_fontengine->getFont();
	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
    v2u screensize = driver->getScreenSize();

	for (const Nametag *nametag : m_nametags) {
		// Nametags are hidden in GenericCAO::updateNametag()

        v3f pos = m_position + nametag->pos * BS;
		f32 transformed_pos[4] = { pos.X, pos.Y, pos.Z, 1.0f };
		trans.multiplyWith1x4Matrix(transformed_pos);
		if (transformed_pos[3] > 0) {
			std::wstring nametag_colorless =
				unescape_translate(utf8_to_wide(nametag->text));
			core::dimension2d<u32> textsize = font->getDimension(
				nametag_colorless.c_str());
			f32 zDiv = transformed_pos[3] == 0.0f ? 1.0f :
				core::reciprocal(transformed_pos[3]);
            v2i screen_pos;
			screen_pos.X = screensize.X *
				(0.5 * transformed_pos[0] * zDiv + 0.5) - textsize.Width / 2;
			screen_pos.Y = screensize.Y *
				(0.5 - transformed_pos[1] * zDiv * 0.5) - textsize.Height / 2;
            recti size(0, 0, textsize.Width, textsize.Height);

			auto bgcolor = nametag->getBgColor(m_show_nametag_backgrounds);
            if (bgcolor.A() != 0) {
                recti bg_size(-2, 0, textsize.Width + 2, textsize.Height);
				driver->draw2DRectangle(bgcolor, bg_size + screen_pos);
			}

			font->draw(
				translate_string(utf8_to_wide(nametag->text)).c_str(),
				size + screen_pos, nametag->textcolor);
		}
	}
}

Nametag *PlayerCamera::addNametag(
        const std::string &text, img::color8 textcolor,
        std::optional<img::color8> bgcolor, const v3f &pos)
{
    Nametag *nametag = new Nametag(text, textcolor, bgcolor, pos);
	m_nametags.push_back(nametag);
	return nametag;
}

void PlayerCamera::removeNametag(Nametag *nametag)
{
	m_nametags.remove(nametag);
	delete nametag;
}
