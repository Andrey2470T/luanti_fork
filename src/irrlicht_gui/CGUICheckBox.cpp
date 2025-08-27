// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUICheckBox.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include <Main/TimeCounter.h>

namespace gui
{

//! constructor
CGUICheckBox::CGUICheckBox(bool checked, IGUIEnvironment *environment, IGUIElement *parent, s32 id, recti rectangle) :
		IGUICheckBox(environment, parent, id, rectangle), CheckTime(0), Pressed(false), Checked(checked), Border(false), Background(false)
{
	// this element can be tabbed into
	setTabStop(true);
	setTabOrder(-1);
}

//! called if an event happened.
bool CGUICheckBox::OnEvent(const main::Event &event)
{
	if (isEnabled()) {
		switch (event.Type) {
		case EET_KEY_INPUT_EVENT:
			if (event.KeyInput.PressedDown &&
					(event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE)) {
				Pressed = true;
				return true;
			} else if (Pressed && event.KeyInput.PressedDown && event.KeyInput.Key == KEY_ESCAPE) {
				Pressed = false;
				return true;
			} else if (!event.KeyInput.PressedDown && Pressed &&
					   (event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE)) {
				Pressed = false;
				if (Parent) {
					main::Event newEvent;
					newEvent.Type = EET_GUI_EVENT;
                    newEvent.GUI.Caller = getID();
					newEvent.GUI.Element = 0;
					Checked = !Checked;
					newEvent.GUI.Type = EGET_CHECKBOX_CHANGED;
					Parent->OnEvent(newEvent);
				}
				return true;
			}
			break;
		case EET_GUI_EVENT:
			if (event.GUI.Type == EGET_ELEMENT_FOCUS_LOST) {
                if (event.GUI.Caller == getID())
					Pressed = false;
			}
			break;
		case EET_MOUSE_INPUT_EVENT:
			if (event.MouseInput.Type == EMIE_LMOUSE_PRESSED_DOWN) {
				Pressed = true;
				CheckTime = main::TimeCounter::getRealTime();
				return true;
			} else if (event.MouseInput.Type == EMIE_LMOUSE_LEFT_UP) {
				bool wasPressed = Pressed;
				Pressed = false;

				if (wasPressed && Parent) {
					if (!AbsoluteClippingRect.isPointInside(v2i(event.MouseInput.X, event.MouseInput.Y))) {
						Pressed = false;
						return true;
					}

					main::Event newEvent;
					newEvent.Type = EET_GUI_EVENT;
                    newEvent.GUI.Caller = getID();
					newEvent.GUI.Element = 0;
					Checked = !Checked;
					newEvent.GUI.Type = EGET_CHECKBOX_CHANGED;
					Parent->OnEvent(newEvent);
				}

				return true;
			}
			break;
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}

//! draws the element and its children
void CGUICheckBox::draw()
{
	if (!IsVisible)
		return;

	GUISkin *skin = Environment->getSkin();
	if (skin) {
		recti frameRect(AbsoluteRect);

		// draw background
		if (Background) {
            img::color8 bgColor = skin->getColor(EGDC_3D_FACE);
			driver->draw2DRectangle(bgColor, frameRect, &AbsoluteClippingRect);
		}

		// draw the border
		if (Border) {
			skin->draw3DSunkenPane(this, 0, true, false, frameRect, &AbsoluteClippingRect);
			frameRect.ULC.X += skin->getSize(EGDS_TEXT_DISTANCE_X);
			frameRect.LRC.X -= skin->getSize(EGDS_TEXT_DISTANCE_X);
		}

		const s32 height = skin->getSize(EGDS_CHECK_BOX_WIDTH);

		// the rectangle around the "checked" area.
		recti checkRect(frameRect.ULC.X,
				((frameRect.getHeight() - height) / 2) + frameRect.ULC.Y,
				0, 0);

		checkRect.LRC.X = checkRect.ULC.X + height;
		checkRect.LRC.Y = checkRect.ULC.Y + height;

		EGUI_DEFAULT_COLOR col = EGDC_GRAY_EDITABLE;
		if (isEnabled())
			col = Pressed ? EGDC_FOCUSED_EDITABLE : EGDC_EDITABLE;
		skin->draw3DSunkenPane(this, skin->getColor(col),
				false, true, checkRect, &AbsoluteClippingRect);

		// the checked icon
		if (Checked) {
			skin->drawIcon(this, EGDI_CHECK_BOX_CHECKED, checkRect.getCenter(),
					CheckTime, main::TimeCounter::getRealTime(), false, &AbsoluteClippingRect);
		}

		// associated text
		if (Text.size()) {
			checkRect = frameRect;
			checkRect.ULC.X += height + 5;

			render::TTFont *font = skin->getFont();
			if (font) {
				font->draw(Text.c_str(), checkRect,
						skin->getColor(isEnabled() ? EGDC_BUTTON_TEXT : EGDC_GRAY_TEXT), false, true, &AbsoluteClippingRect);
			}
		}
	}
	IGUIElement::draw();
}

//! set if box is checked
void CGUICheckBox::setChecked(bool checked)
{
	Checked = checked;
}

//! returns if box is checked
bool CGUICheckBox::isChecked() const
{
	return Checked;
}

//! Sets whether to draw the background
void CGUICheckBox::setDrawBackground(bool draw)
{
	Background = draw;
}

//! Checks if background drawing is enabled
bool CGUICheckBox::isDrawBackgroundEnabled() const
{
	return Background;
}

//! Sets whether to draw the border
void CGUICheckBox::setDrawBorder(bool draw)
{
	Border = draw;
}

//! Checks if border drawing is enabled
bool CGUICheckBox::isDrawBorderEnabled() const
{
	return Border;
}

} // end namespace gui
