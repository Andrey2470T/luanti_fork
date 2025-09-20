// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2014 sapier
// Copyright (C) 2024 grorp, Gregor Parzefall

#pragma once

#include "IGUIElement.h"
#include <Core/IEventReceiver.h>
#include <Core/Events.h>
#include <Core/MainWindow.h>
#include <memory>
#include <optional>
#include <unordered_map>
#include "itemdef.h"
#include "touchscreenlayout.h"
#include "util/basic_macros.h"

namespace gui
{
	class IGUIEnvironment;
	class IGUIImage;
	class IGUIStaticText;
}

using namespace gui;

class ResourceCache;
class Camera;

enum class TapState
{
    No,
	ShortTap,
	LongTap,
};


#define BUTTON_REPEAT_DELAY 0.5f
#define BUTTON_REPEAT_INTERVAL 0.333f

// Our simulated clicks last some milliseconds so that server-side mods have a
// chance to detect them via l_get_player_control.
// If you tap faster than this value, the simulated clicks are of course shorter.
#define SIMULATED_CLICK_DURATION_MS 50


struct button_info
{
	float repeat_counter;
    KEY_CODE keycode;
	std::vector<size_t> pointer_ids;
    IGUIImage *gui_button = nullptr;

	enum {
		NOT_TOGGLEABLE,
		FIRST_TEXTURE,
		SECOND_TEXTURE
	} toggleable = NOT_TOGGLEABLE;
	std::string toggle_textures[2];
};


class TouchControls
{
public:
    TouchControls(RenderSystem *rndsys, ResourceCache *cache,
        IEventReceiver *receiver);
	~TouchControls();
	DISABLE_CLASS_COPY(TouchControls);

	void translateEvent(const core::Event &event);
	void applyContextControls(const TouchInteractionMode &mode);

	double getYawChange()
	{
		double res = m_camera_yaw_change;
		m_camera_yaw_change = 0;
		return res;
	}

	double getPitchChange() {
		double res = m_camera_pitch_change;
		m_camera_pitch_change = 0;
		return res;
	}

	/**
	 * Returns a line which describes what the player is pointing at.
	 * The starting point and looking direction are significant,
	 * the line should be scaled to match its length to the actual distance
	 * the player can reach.
	 * The line starts at the camera and ends on the camera's far plane.
	 * The coordinates do not contain the camera offset.
	 */
	line3f getShootline() { return m_shootline; }

	float getJoystickDirection() { return m_joystick_direction; }
	float getJoystickSpeed() { return m_joystick_speed; }

    void step(float dtime, Camera *camera);
	inline void setUseCrosshair(bool use_crosshair) { m_draw_crosshair = use_crosshair; }

	void setVisible(bool visible);
	void hide();
	void show();

	void resetHotbarRects();
	void registerHotbarRect(u16 index, const recti &rect);
	std::optional<u16> getHotbarSelection();

	bool isStatusTextOverriden() { return m_overflow_open; }
    IGUIStaticText *getStatusText() { return m_status_text; }

	ButtonLayout getLayout() { return m_layout; }
	void applyLayout(const ButtonLayout &layout);

private:
    RenderSystem *m_rndsys = nullptr;
    core::MainWindow *m_window = nullptr;
	IGUIEnvironment *m_guienv = nullptr;
	IEventReceiver *m_receiver = nullptr;
	ResourceCache *m_cache = nullptr;
    v2u m_wndsize;
	s32 m_button_size;
	double m_touchscreen_threshold;
	u16 m_long_tap_delay;
	bool m_visible = true;

	std::unordered_map<u16, recti> m_hotbar_rects;
	std::optional<u16> m_hotbar_selection = std::nullopt;

	// value in degree
	double m_camera_yaw_change = 0.0;
	double m_camera_pitch_change = 0.0;

	/**
	 * A line starting at the camera and pointing towards the selected object.
	 * The line ends on the camera's far plane.
	 * The coordinates do not contain the camera offset.
	 */
	line3f m_shootline;

	bool m_has_move_id = false;
	size_t m_move_id;
	bool m_move_has_really_moved = false;
	u64 m_move_downtime = 0;
	// m_move_pos stays valid even after m_move_id has been released.
	v2i m_move_pos;
	// This is needed so that we don't miss if m_has_move_id is true for less
	// than one client step, i.e. press and release happen in the same step.
	bool m_had_move_id = false;
	bool m_move_prevent_short_tap = false;

	bool m_has_joystick_id = false;
	size_t m_joystick_id;
	bool m_joystick_has_really_moved = false;
	float m_joystick_direction = 0.0f; // assume forward
	float m_joystick_speed = 0.0f; // no movement
	bool m_joystick_status_aux1 = false;
	bool m_fixed_joystick = false;
	bool m_joystick_triggers_aux1 = false;
	bool m_draw_crosshair = false;
    IGUIImage *m_joystick_btn_off;
    IGUIImage *m_joystick_btn_bg;
    IGUIImage *m_joystick_btn_center;

	std::vector<button_info> m_buttons;
    IGUIImage *m_overflow_btn;

	bool m_overflow_open = false;
    IGUIStaticText *m_overflow_bg;
	std::vector<button_info> m_overflow_buttons;
    std::vector<IGUIStaticText *> m_overflow_button_titles;
	std::vector<recti> m_overflow_button_rects;

    IGUIStaticText *m_status_text;

	// Note: TouchControls intentionally uses IGUIImage instead of IGUIButton
	// for its buttons. We only want static image display, not interactivity,
	// from Irrlicht.

    void emitKeyboardEvent(KEY_CODE keycode, bool pressed);

	void loadButtonTexture(IGUIImage *gui_button, const std::string &path);
	void buttonEmitAction(button_info &btn, bool action);

	bool buttonsHandlePress(std::vector<button_info> &buttons, size_t pointer_id, IGUIElement *element);
	bool buttonsHandleRelease(std::vector<button_info> &buttons, size_t pointer_id);
	bool buttonsStep(std::vector<button_info> &buttons, float dtime);

	void toggleOverflowMenu();
	void updateVisibility();
	void releaseAll();

	// initialize a button
	bool mayAddButton(touch_gui_button_id id);
	void addButton(std::vector<button_info> &buttons,
			touch_gui_button_id id, const std::string &image,
			const recti &rect, bool visible);
	void addToggleButton(std::vector<button_info> &buttons,
			touch_gui_button_id id,
			const std::string &image_1, const std::string &image_2,
			const recti &rect, bool visible);

	IGUIImage *makeButtonDirect(touch_gui_button_id id,
			const recti &rect, bool visible);

	// handle pressing hotbar items
	bool isHotbarButton(const core::Event &event);

	// handle release event
	void handleReleaseEvent(size_t pointer_id);

	// apply joystick status
	void applyJoystickStatus();

	// map to store the IDs and original positions of currently pressed pointers
	std::unordered_map<size_t, v2i> m_pointer_downpos;
	// map to store the IDs and positions of currently pressed pointers
	std::unordered_map<size_t, v2i> m_pointer_pos;

	v2i getPointerPos();
    void emitMouseEvent(MouseInputEventType type);
	TouchInteractionMode m_last_mode = TouchInteractionMode_END;
    TapState m_tap_state = TapState::No;

	bool m_dig_pressed = false;
	u64 m_dig_pressed_until = 0;

	bool m_place_pressed = false;
	u64 m_place_pressed_until = 0;

	ButtonLayout m_layout;
};

extern TouchControls *g_touchcontrols;
