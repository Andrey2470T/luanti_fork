// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 grorp, Gregor Parzefall <grorp@posteo.de>

#include "touchscreeneditor.h"
#include "client/render/rendersystem.h"
#include "gui/IGUIEnvironment.h"
#include "touchcontrols.h"
#include "touchscreenlayout.h"
#include "gettext.h"
#include "settings.h"
#include "IGUIButton.h"
#include <Render/TTFont.h>
#include "IGUIImage.h"
#include "IGUIStaticText.h"
#include "client/ui/sprite.h"
#include "client/ui/extra_images.h"

GUITouchscreenLayout::GUITouchscreenLayout(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
        IMenuManager *menumgr
):
    GUIModalMenu(env, parent, id, menumgr),
    m_menu(std::make_unique<UIRects>(env->getRenderSystem(), 0))
{
	if (g_touchcontrols)
		m_layout = g_touchcontrols->getLayout();
	else
		m_layout = ButtonLayout::loadFromSettings();

    m_gui_help_text = Environment->addStaticText(
            L"", recti(), false, false, this, -1);
	m_gui_help_text->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);

    m_gui_add_btn = Environment->addButton(
            recti(), this, -1, wstrgettext("Add button").c_str());
    m_gui_reset_btn = Environment->addButton(
            recti(), this, -1, wstrgettext("Reset").c_str());
    m_gui_done_btn = Environment->addButton(
            recti(), this, -1, wstrgettext("Done").c_str());

    m_gui_remove_btn = Environment->addButton(
        recti(), this, -1, wstrgettext("Remove").c_str());
}

GUITouchscreenLayout::~GUITouchscreenLayout()
{
	ButtonLayout::clearTextureCache();
}

void GUITouchscreenLayout::regenerateGui(v2u screensize)
{
	DesiredRect = recti(0, 0, screensize.X, screensize.Y);
	recalculateAbsolutePosition(false);

    s32 button_size = ButtonLayout::getButtonSize(screensize, Environment->getRenderSystem()->getScaleFactor());
	if (m_last_screensize != screensize || m_button_size != button_size) {
		m_last_screensize = screensize;
		m_button_size = button_size;
		// Prevent interpolation when the layout scales.
		clearGUIImages();
	}

	// Discard invalid selection. May happen when...
	// 1. a button is removed.
	// 2. adding a button fails and it disappears from the layout again.
	if (m_selected_btn != touch_gui_button_id_END &&
			m_layout.layout.count(m_selected_btn) == 0)
		m_selected_btn = touch_gui_button_id_END;

	if (m_mode == Mode::Add)
		regenerateGUIImagesAddMode(screensize);
	else
		regenerateGUIImagesRegular(screensize);
	regenerateMenu(screensize);
}

void GUITouchscreenLayout::clearGUIImages()
{
	m_gui_images.clear();
	m_gui_images_target_pos.clear();

	m_add_layout.layout.clear();
	m_add_button_titles.clear();
}

void GUITouchscreenLayout::regenerateGUIImagesRegular(v2u screensize)
{
	assert(m_mode != Mode::Add);

	auto old_gui_images = m_gui_images;
	clearGUIImages();

    auto cache = Environment->getResourceCache();
	for (const auto &[btn, meta] : m_layout.layout) {
        recti rect = m_layout.getRect(btn, screensize, m_button_size, cache);
        IGUIImage *img;

		if (old_gui_images.count(btn) > 0) {
			img = old_gui_images.at(btn);
			// Update size, keep position. Position is interpolated in interpolateGUIImages.
			img->setRelativePosition(recti(
					img->getRelativePosition().ULC, rect.getSize()));
		} else {
            img = Environment->addImage(rect, this, -1);
            img->setImage(ButtonLayout::getTexture(btn, cache));
			img->setScaleImage(true);
		}

		m_gui_images[btn] = img;
		m_gui_images_target_pos[btn] = rect.ULC;
	}
}

void GUITouchscreenLayout::regenerateGUIImagesAddMode(v2u screensize)
{
	assert(m_mode == Mode::Add);

	clearGUIImages();

	auto missing_buttons = m_layout.getMissingButtons();

    auto cache = Environment->getResourceCache();
    f32 scalefactor = Environment->getRenderSystem()->getScaleFactor();
    layout_button_grid(screensize, scalefactor, cache, missing_buttons,
			[&] (touch_gui_button_id btn, v2i pos, recti rect) {
        auto img = Environment->addImage(rect, this, -1);
        img->setImage(ButtonLayout::getTexture(btn, cache));
		img->setScaleImage(true);
		m_gui_images[btn] = img;

		ButtonMeta meta;
		meta.setPos(pos, screensize, m_button_size);
		m_add_layout.layout[btn] = meta;

		IGUIStaticText *text = Environment->addStaticText(L"", recti(),
				false, false,this, -1);
		make_button_grid_title(text, btn, pos, rect);
        m_add_button_titles.push_back(text);
	});
}

void GUITouchscreenLayout::interpolateGUIImages()
{
	if (m_mode == Mode::Add)
		return;

	for (auto &[btn, gui_image] : m_gui_images) {
		bool interpolate = m_mode != Mode::Dragging || m_selected_btn != btn;

		v2i cur_pos_int = gui_image->getRelativePosition().ULC;
		v2i tgt_pos_int = m_gui_images_target_pos.at(btn);
		v2f cur_pos(cur_pos_int.X, cur_pos_int.Y);
		v2f tgt_pos(tgt_pos_int.X, tgt_pos_int.Y);

		if (interpolate && cur_pos.getDistanceFrom(tgt_pos) > 2.0f) {
            v2f pos = cur_pos.linInterp(tgt_pos, 0.5f);
            gui_image->setRelativePosition(v2i(round32(pos.X), round32(pos.Y)));
		} else {
			gui_image->setRelativePosition(tgt_pos_int);
		}
	}
}

static void layout_menu_row(v2u screensize, f32 displaydensity,
        const std::vector<IGUIButton *> &row,
        const std::vector<IGUIButton *> &full_row, bool bottom)
{
    s32 spacing = displaydensity * 4.0f;

	s32 btn_w = 0;
	s32 btn_h = 0;
	for (const auto &btn : full_row) {
		render::TTFont *font = btn->getActiveFont();
        v2u dim = font->getTextSize(btn->getText());
		btn_w = std::max(btn_w, (s32)(dim.X * 1.5f));
		btn_h = std::max(btn_h, (s32)(dim.Y * 2.5f));
	}

	const s32 row_width = ((btn_w + spacing) * row.size()) - spacing;
	s32 x = screensize.X / 2 - row_width / 2;
	const s32 y = bottom ? screensize.Y - spacing - btn_h : spacing;

	for (const auto &btn : row) {
		btn->setRelativePosition(recti(
                v2i(x, y), btn_w, btn_h));
		x += btn_w + spacing;
	}
}

void GUITouchscreenLayout::regenerateMenu(v2u screensize)
{
	bool have_selection = m_selected_btn != touch_gui_button_id_END;

	if (m_mode == Mode::Add)
		m_gui_help_text->setText(wstrgettext("Start dragging a button to add. Tap outside to cancel.").c_str());
	else if (!have_selection)
		m_gui_help_text->setText(wstrgettext("Tap a button to select it. Drag a button to move it.").c_str());
	else
		m_gui_help_text->setText(wstrgettext("Tap outside to deselect.").c_str());

	render::TTFont *font = m_gui_help_text->getActiveFont();
    v2u dim = font->getTextSize(m_gui_help_text->getText());
	s32 height = dim.Y * 2.5f;
	s32 pos_y = (m_mode == Mode::Add || have_selection) ? 0 : screensize.Y - height;
	m_gui_help_text->setRelativePosition(recti(
			v2i(0, pos_y),
            screensize.X, height));

	bool normal_buttons_visible = m_mode != Mode::Add && !have_selection;
	bool add_visible = normal_buttons_visible && !m_layout.getMissingButtons().empty();

	m_gui_add_btn->setVisible(add_visible);
	m_gui_reset_btn->setVisible(normal_buttons_visible);
	m_gui_done_btn->setVisible(normal_buttons_visible);

    f32 display_density = Environment->getRenderSystem()->getDisplayDensity();
	if (normal_buttons_visible) {
		std::vector row1{m_gui_add_btn, m_gui_reset_btn, m_gui_done_btn};
		if (add_visible) {
            layout_menu_row(screensize, display_density, row1, row1, false);
		} else {
			std::vector row1_reduced{m_gui_reset_btn, m_gui_done_btn};
            layout_menu_row(screensize, display_density, row1_reduced, row1, false);
		}
	}

	bool remove_visible = m_mode != Mode::Add && have_selection &&
			!ButtonLayout::isButtonRequired(m_selected_btn);

	m_gui_remove_btn->setVisible(remove_visible);

	if (remove_visible) {
		std::vector row2{m_gui_remove_btn};
        layout_menu_row(screensize, display_density, row2, row2, true);
	}
}

void GUITouchscreenLayout::drawMenu()
{
    img::color8 bgcolor(img::PF_RGBA8, 0, 0, 0, 140);
    img::color8 selection_color(img::gray);
    img::color8 error_color(img::red);

    m_menu->clear();

    m_menu->addRect(toRectf(AbsoluteRect), {bgcolor});

	// Done here instead of in OnPostRender to avoid drag&drop lagging behind
	// input by one frame.
	// Must be done before drawing selection rectangle.
	interpolateGUIImages();

	bool draw_selection = m_gui_images.count(m_selected_btn) > 0;
	if (draw_selection)
        m_menu->addRect(
            toRectf(m_gui_images.at(m_selected_btn)->getAbsolutePosition()),
            {selection_color});

	if (m_mode == Mode::Dragging) {
		for (const auto &rect : m_error_rects)
            m_menu->addRect(toRectf(rect), {error_color});
	}

    m_menu->setClipRect(AbsoluteClippingRect);
    m_menu->draw();

	IGUIElement::draw();
}

void GUITouchscreenLayout::updateDragState(v2u screensize, v2i mouse_movement)
{
	assert(m_mode == Mode::Dragging);

    auto cache = Environment->getResourceCache();
    recti rect = m_layout.getRect(m_selected_btn, screensize, m_button_size, cache);
	rect += mouse_movement;
    rect.constrainTo(recti(v2i(0, 0), screensize.X, screensize.Y));

	ButtonMeta &meta = m_layout.layout.at(m_selected_btn);
	meta.setPos(rect.getCenter(), screensize, m_button_size);

    rect = m_layout.getRect(m_selected_btn, screensize, m_button_size, cache);

	m_error_rects.clear();
	for (const auto &[other_btn, other_meta] : m_layout.layout) {
		if (other_btn == m_selected_btn)
			continue;
        recti other_rect = m_layout.getRect(other_btn, screensize, m_button_size, cache);
		if (other_rect.isRectCollided(rect))
			m_error_rects.push_back(other_rect);
	}
	if (m_error_rects.empty())
		m_last_good_layout = m_layout;
}

bool GUITouchscreenLayout::OnEvent(const core::Event& event)
{
	if (event.Type == EET_KEY_INPUT_EVENT) {
		if (event.KeyInput.Key == core::KEY_ESCAPE && event.KeyInput.PressedDown) {
			quitMenu();
			return true;
		}
	}

    v2u screensize = Environment->getRenderSystem()->getWindowSize();

	if (event.Type == EET_MOUSE_INPUT_EVENT) {
		v2i mouse_pos = v2i(event.MouseInput.X, event.MouseInput.Y);

		switch (event.MouseInput.Type) {
		case EMIE_LMOUSE_PRESSED_DOWN: {
			m_mouse_down = true;
			m_last_mouse_pos = mouse_pos;

			IGUIElement *el = Environment->getRootGUIElement()->getElementFromPoint(mouse_pos);
			// Clicking on nothing deselects.
			m_selected_btn = touch_gui_button_id_END;
			for (const auto &[btn, gui_image] : m_gui_images) {
                if (el == gui_image) {
					m_selected_btn = btn;
					break;
				}
			}

			if (m_mode == Mode::Add) {
				if (m_selected_btn != touch_gui_button_id_END) {
					m_mode = Mode::Dragging;
					m_last_good_layout = m_layout;
					m_layout.layout[m_selected_btn] = m_add_layout.layout.at(m_selected_btn);
					updateDragState(screensize, v2i(0, 0));
				} else {
					// Clicking on nothing quits add mode without adding a button.
					m_mode = Mode::Default;
				}
			}

			regenerateGui(screensize);
			return true;
		}
		case EMIE_MOUSE_MOVED: {
			if (m_mouse_down && m_selected_btn != touch_gui_button_id_END) {
				if (m_mode != Mode::Dragging) {
					m_mode = Mode::Dragging;
					m_last_good_layout = m_layout;
				}
				updateDragState(screensize, mouse_pos - m_last_mouse_pos);

				regenerateGui(screensize);
			}

			m_last_mouse_pos = mouse_pos;
			return true;
		}
		case EMIE_LMOUSE_LEFT_UP: {
			m_mouse_down = false;

			if (m_mode == Mode::Dragging) {
				m_mode = Mode::Default;
				if (!m_error_rects.empty())
					m_layout = m_last_good_layout;

				regenerateGui(screensize);
			}

			return true;
		}
		default:
			break;
		}
	}

	if (event.Type == EET_GUI_EVENT) {
		switch (event.GUI.Type) {
		case EGET_BUTTON_CLICKED: {
            if (event.GUI.Caller == m_gui_add_btn) {
				m_mode = Mode::Add;
				regenerateGui(screensize);
				return true;
			}

            if (event.GUI.Caller == m_gui_reset_btn) {
				m_layout = ButtonLayout::predefined;
				regenerateGui(screensize);
				return true;
			}

            if (event.GUI.Caller == m_gui_done_btn) {
				if (g_touchcontrols)
					g_touchcontrols->applyLayout(m_layout);
				std::ostringstream oss;
				m_layout.serializeJson(oss);
				g_settings->set("touch_layout", oss.str());
				quitMenu();
				return true;
			}

            if (event.GUI.Caller == m_gui_remove_btn) {
				m_layout.layout.erase(m_selected_btn);
				regenerateGui(screensize);
				return true;
			}

			break;
		}
		default:
			break;
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}
