// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 grorp, Gregor Parzefall <grorp@posteo.de>

#pragma once

#include "touchscreenlayout.h"
#include "modalMenu.h"

#include <memory>
#include <unordered_map>

namespace gui
{
	class IGUIButton;
	class IGUIImage;
}

class UIRects;

class GUITouchscreenLayout : public GUIModalMenu
{
public:
	GUITouchscreenLayout(gui::IGUIEnvironment* env,
            gui::IGUIElement* parent, s32 id,
            IMenuManager *menumgr);
	~GUITouchscreenLayout();

	void regenerateGui(v2u screensize);
	void drawMenu();
	bool OnEvent(const core::Event& event);

protected:
	std::wstring getLabelByID(s32 id) { return L""; }
	std::string getNameByID(s32 id) { return ""; }

private:
	ButtonLayout m_layout;
	v2u m_last_screensize;
	s32 m_button_size;

	enum class Mode {
		Default,
		Dragging,
		Add,
	};
	Mode m_mode = Mode::Default;

    std::unordered_map<touch_gui_button_id, gui::IGUIImage *> m_gui_images;
	// unused if m_mode == Mode::Add
	std::unordered_map<touch_gui_button_id, v2i> m_gui_images_target_pos;
	void clearGUIImages();
	void regenerateGUIImagesRegular(v2u screensize);
	void regenerateGUIImagesAddMode(v2u screensize);
	void interpolateGUIImages();

	// interaction state
	bool m_mouse_down = false;
	v2i m_last_mouse_pos;
	touch_gui_button_id m_selected_btn = touch_gui_button_id_END;

	// dragging
	ButtonLayout m_last_good_layout;
	std::vector<recti> m_error_rects;
	void updateDragState(v2u screensize, v2i mouse_movement);

	// add mode
	ButtonLayout m_add_layout;
    std::vector<gui::IGUIStaticText *> m_add_button_titles;

	// Menu GUI elements
    gui::IGUIStaticText *m_gui_help_text;

    gui::IGUIButton *m_gui_add_btn;
    gui::IGUIButton *m_gui_reset_btn;
    gui::IGUIButton *m_gui_done_btn;

    gui::IGUIButton *m_gui_remove_btn;

    std::unique_ptr<UIRects> m_menu;

	void regenerateMenu(v2u screensize);
};
