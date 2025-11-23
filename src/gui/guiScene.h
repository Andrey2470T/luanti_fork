// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2020 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>

#pragma once

#include "IGUIElement.h"
#include "StyleSpec.h"
#include "client/render/camera.h"

class Client;
class Model;
struct TileLayer;

class GUIScene : public gui::IGUIElement
{
public:
    GUIScene(Client *client, gui::IGUIEnvironment *env,
		 gui::IGUIElement *parent, recti rect, s32 id = -1);

    void setModel(Model *model);
	void setTexture(u32 idx, img::Image *texture);
	void setBackgroundColor(const img::color8 &color) noexcept { m_bgcolor = color; };
    void setFrameLoop(s32 begin, s32 end);
	void setAnimationSpeed(f32 speed);
	void enableMouseControl(bool enable) noexcept { m_mouse_ctrl = enable; };
	void setRotation(v2f rot) noexcept { m_custom_rot = rot; };
	void enableContinuousRotation(bool enable) noexcept { m_inf_rot = enable; };
	void setStyles(const std::array<StyleSpec, StyleSpec::NUM_STATES> &styles);
    v3f getCameraRotation() const { return m_cam->getRotation(); };

	virtual void draw();
	virtual bool OnEvent(const core::Event &event);

private:
	void calcOptimalDistance();
    void updateTargetPos(v3f target);
	void setCameraRotation(v3f rot);
	/// @return true indicates that the rotation was corrected
	bool correctBounds(v3f &rot);
    //void cameraLoop();

	void rotateCamera(const v3f &delta) { setCameraRotation(getCameraRotation() + delta); };

    void updateCamera();

    Client *m_client;
    Camera *m_cam = nullptr;
    Model *m_model = nullptr;

	f32 m_cam_distance = 50.f;

	u64 m_last_time = 0;

	v3f m_cam_pos;
	v3f m_target_pos;
	v3f m_last_target_pos;
	// Cursor positions
	v2f m_curr_pos;
	v2f m_last_pos;
	// Initial rotation
	v2f m_custom_rot;

	bool m_mouse_ctrl = true;
	bool m_update_cam = false;
	bool m_inf_rot    = false;
	bool m_initial_rotation = true;

    img::color8 m_bgcolor;

    std::shared_ptr<TileLayer> m_layer;
    std::unique_ptr<UISprite> m_background;
 };
