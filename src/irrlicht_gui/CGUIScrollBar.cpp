// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIScrollBar.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include "CGUIButton.h"

namespace gui
{

//! constructor
CGUIScrollBar::CGUIScrollBar(bool horizontal, IGUIEnvironment *environment,
		IGUIElement *parent, s32 id,
		recti rectangle, bool noclip) :
		IGUIScrollBar(environment, parent, id, rectangle),
		UpButton(0),
		DownButton(0), Dragging(false), Horizontal(horizontal),
		DraggedBySlider(false), TrayClick(false), Pos(0), DrawPos(0),
		DrawHeight(0), Min(0), Max(100), SmallStep(10), LargeStep(50), DesiredPos(0),
		LastChange(0)
{
	refreshControls();

	setNotClipped(noclip);

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);

	setPos(0);
}

//! destructor
CGUIScrollBar::~CGUIScrollBar()
{
	if (UpButton)
		UpButton->drop();

	if (DownButton)
		DownButton->drop();
}

//! called if an event happened.
bool CGUIScrollBar::OnEvent(const main::Event &event)
{
	if (isEnabled()) {

		switch (event.Type) {
		case EET_KEY_INPUT_EVENT:
			if (event.KeyInput.PressedDown) {
				const s32 oldPos = Pos;
				bool absorb = true;
				switch (event.KeyInput.Key) {
				case KEY_LEFT:
				case KEY_UP:
					setPos(Pos - SmallStep);
					break;
				case KEY_RIGHT:
				case KEY_DOWN:
					setPos(Pos + SmallStep);
					break;
				case KEY_HOME:
					setPos(Min);
					break;
				case KEY_PRIOR:
					setPos(Pos - LargeStep);
					break;
				case KEY_END:
					setPos(Max);
					break;
				case KEY_NEXT:
					setPos(Pos + LargeStep);
					break;
				default:
					absorb = false;
				}

				if (Pos != oldPos) {
					main::Event newEvent;
					newEvent.Type = EET_GUI_EVENT;
                    newEvent.GUI.Caller = getID();
					newEvent.GUI.Element = 0;
					newEvent.GUI.Type = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(newEvent);
				}
				if (absorb)
					return true;
			}
			break;
		case EET_GUI_EVENT:
			if (event.GUI.Type == EGET_BUTTON_CLICKED) {
                if (event.GUI.Caller == UpButton->getID())
					setPos(Pos - SmallStep);
                else if (event.GUI.Caller == DownButton->getID())
					setPos(Pos + SmallStep);

				main::Event newEvent;
				newEvent.Type = EET_GUI_EVENT;
                newEvent.GUI.Caller = getID();
				newEvent.GUI.Element = 0;
				newEvent.GUI.Type = EGET_SCROLL_BAR_CHANGED;
				Parent->OnEvent(newEvent);

				return true;
			} else if (event.GUI.Type == EGET_ELEMENT_FOCUS_LOST) {
                if (event.GUI.Caller == getID())
					Dragging = false;
			}
			break;
		case EET_MOUSE_INPUT_EVENT: {
			const v2i p(event.MouseInput.X, event.MouseInput.Y);
			bool isInside = isPointInside(p);
			switch (event.MouseInput.Type) {
			case EMIE_MOUSE_WHEEL:
				if (Environment->hasFocus(this)) {
					// thanks to a bug report by REAPER
					// thanks to tommi by tommi for another bugfix
					// everybody needs a little thanking. hallo niko!;-)
					setPos(getPos() +
							((event.MouseInput.WheelDelta < 0 ? -1 : 1) * SmallStep * (Horizontal ? 1 : -1)));

					main::Event newEvent;
					newEvent.Type = EET_GUI_EVENT;
                    newEvent.GUI.Caller = getID();
                    newEvent.GUI.Element = std::nullopt;
					newEvent.GUI.Type = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(newEvent);
					return true;
				}
				break;
			case EMIE_LMOUSE_PRESSED_DOWN: {
				if (isInside) {
					Dragging = true;
					DraggedBySlider = SliderRect.isPointInside(p);
					TrayClick = !DraggedBySlider;
					DesiredPos = getPosFromMousePos(p);
					return true;
				}
				break;
			}
			case EMIE_LMOUSE_LEFT_UP:
			case EMIE_MOUSE_MOVED: {
				if (!event.MouseInput.isLeftPressed())
					Dragging = false;

				if (!Dragging) {
					if (event.MouseInput.Type == EMIE_MOUSE_MOVED)
						break;
					return isInside;
				}

				if (event.MouseInput.Type == EMIE_LMOUSE_LEFT_UP)
					Dragging = false;

				const s32 newPos = getPosFromMousePos(p);
				const s32 oldPos = Pos;

				if (!DraggedBySlider) {
					if (isInside) {
						DraggedBySlider = SliderRect.isPointInside(p);
						TrayClick = !DraggedBySlider;
					}

					if (DraggedBySlider) {
						setPos(newPos);
					} else {
						TrayClick = false;
						if (event.MouseInput.Type == EMIE_MOUSE_MOVED)
							return isInside;
					}
				}

				if (DraggedBySlider) {
					setPos(newPos);
				} else {
					DesiredPos = newPos;
				}

				if (Pos != oldPos && Parent) {
					main::Event newEvent;
					newEvent.Type = EET_GUI_EVENT;
                    newEvent.GUI.Caller = getID();
                    newEvent.GUI.Element = std::nullopt;
					newEvent.GUI.Type = EGET_SCROLL_BAR_CHANGED;
					Parent->OnEvent(newEvent);
				}
				return isInside;
			} break;

			default:
				break;
			}
		} break;
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}

void CGUIScrollBar::OnPostRender(u32 timeMs)
{
	if (Dragging && !DraggedBySlider && TrayClick && timeMs > LastChange + 200) {
		LastChange = timeMs;

		const s32 oldPos = Pos;

		if (DesiredPos >= Pos + LargeStep)
			setPos(Pos + LargeStep);
		else if (DesiredPos <= Pos - LargeStep)
			setPos(Pos - LargeStep);
		else if (DesiredPos >= Pos - LargeStep && DesiredPos <= Pos + LargeStep)
			setPos(DesiredPos);

		if (Pos != oldPos && Parent) {
			main::Event newEvent;
			newEvent.Type = EET_GUI_EVENT;
            newEvent.GUI.Caller = getID();
            newEvent.GUI.Element = std::nullopt;
			newEvent.GUI.Type = EGET_SCROLL_BAR_CHANGED;
			Parent->OnEvent(newEvent);
		}
	}
}

//! draws the element and its children
void CGUIScrollBar::draw()
{
	if (!IsVisible)
		return;

	GUISkin *skin = Environment->getSkin();
	if (!skin)
		return;

	img::color8 iconColor = skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL);
	if (iconColor != CurrentIconColor) {
		refreshControls();
	}

	SliderRect = AbsoluteRect;

	// draws the background
	skin->draw2DRectangle(this, skin->getColor(EGDC_SCROLLBAR), SliderRect, &AbsoluteClippingRect);

    if (!equals(range(), 0)) {
		// recalculate slider rectangle
		if (Horizontal) {
			SliderRect.ULC.X = AbsoluteRect.ULC.X + DrawPos + RelativeRect.getHeight() - DrawHeight / 2;
			SliderRect.LRC.X = SliderRect.ULC.X + DrawHeight;
		} else {
			SliderRect.ULC.Y = AbsoluteRect.ULC.Y + DrawPos + RelativeRect.getWidth() - DrawHeight / 2;
			SliderRect.LRC.Y = SliderRect.ULC.Y + DrawHeight;
		}

		skin->draw3DButtonPaneStandard(this, SliderRect, &AbsoluteClippingRect);
	}

	// draw buttons
	IGUIElement::draw();
}

void CGUIScrollBar::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();
	// todo: properly resize
	refreshControls();
	setPos(Pos);
}

//!
s32 CGUIScrollBar::getPosFromMousePos(const v2i &pos) const
{
	f32 w, p;
	if (Horizontal) {
		w = RelativeRect.getWidth() - f32(RelativeRect.getHeight()) * 3.0f;
		p = pos.X - AbsoluteRect.ULC.X - RelativeRect.getHeight() * 1.5f;
	} else {
		w = RelativeRect.getHeight() - f32(RelativeRect.getWidth()) * 3.0f;
		p = pos.Y - AbsoluteRect.ULC.Y - RelativeRect.getWidth() * 1.5f;
	}
	return (s32)(p / w * range()) + Min;
}

//! sets the position of the scrollbar
void CGUIScrollBar::setPos(s32 pos)
{
    Pos = std::clamp(pos, Min, Max);

    if (!equals(range(), 0)) {
		if (Horizontal) {
			f32 f = (RelativeRect.getWidth() - ((f32)RelativeRect.getHeight() * 3.0f)) / range();
			DrawPos = (s32)(((Pos - Min) * f) + ((f32)RelativeRect.getHeight() * 0.5f));
			DrawHeight = RelativeRect.getHeight();
		} else {
			f32 f = (RelativeRect.getHeight() - ((f32)RelativeRect.getWidth() * 3.0f)) / range();

			DrawPos = (s32)(((Pos - Min) * f) + ((f32)RelativeRect.getWidth() * 0.5f));
			DrawHeight = RelativeRect.getWidth();
		}
	}
}

//! gets the small step value
s32 CGUIScrollBar::getSmallStep() const
{
	return SmallStep;
}

//! sets the small step value
void CGUIScrollBar::setSmallStep(s32 step)
{
	if (step > 0)
		SmallStep = step;
	else
		SmallStep = 10;
}

//! gets the small step value
s32 CGUIScrollBar::getLargeStep() const
{
	return LargeStep;
}

//! sets the small step value
void CGUIScrollBar::setLargeStep(s32 step)
{
	if (step > 0)
		LargeStep = step;
	else
		LargeStep = 50;
}

//! gets the maximum value of the scrollbar.
s32 CGUIScrollBar::getMax() const
{
	return Max;
}

//! sets the maximum value of the scrollbar.
void CGUIScrollBar::setMax(s32 max)
{
	Max = max;
	if (Min > Max)
		Min = Max;

    bool enable = !equals(range(), 0);
	UpButton->setEnabled(enable);
	DownButton->setEnabled(enable);
	setPos(Pos);
}

//! gets the minimum value of the scrollbar.
s32 CGUIScrollBar::getMin() const
{
	return Min;
}

//! sets the minimum value of the scrollbar.
void CGUIScrollBar::setMin(s32 min)
{
	Min = min;
	if (Max < Min)
		Max = Min;

    bool enable = !equals(range(), 0);
	UpButton->setEnabled(enable);
	DownButton->setEnabled(enable);
	setPos(Pos);
}

//! gets the current position of the scrollbar
s32 CGUIScrollBar::getPos() const
{
	return Pos;
}

//! refreshes the position and text on child buttons
void CGUIScrollBar::refreshControls()
{
    CurrentIconColor = img::color8(img::white);

	GUISkin *skin = Environment->getSkin();
	IGUISpriteBank *sprites = 0;

	if (skin) {
		sprites = skin->getSpriteBank();
		CurrentIconColor = skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL);
	}

	if (Horizontal) {
		const s32 h = RelativeRect.getHeight();
		const s32 w = (h < RelativeRect.getWidth() / 2) ? h : RelativeRect.getWidth() / 2;
		if (!UpButton) {
			UpButton = new CGUIButton(Environment, this, -1, recti(0, 0, w, h), NoClip);
			UpButton->setSubElement(true);
			UpButton->setTabStop(false);
		}
		if (sprites) {
			UpButton->setSpriteBank(sprites);
			UpButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_CURSOR_LEFT), CurrentIconColor);
			UpButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_CURSOR_LEFT), CurrentIconColor);
		}
		UpButton->setRelativePosition(recti(0, 0, w, h));
        UpButton->setAlignment(GUIAlignment::UpperLeft, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT);
		if (!DownButton) {
			DownButton = new CGUIButton(Environment, this, -1, recti(RelativeRect.getWidth() - w, 0, RelativeRect.getWidth(), h), NoClip);
			DownButton->setSubElement(true);
			DownButton->setTabStop(false);
		}
		if (sprites) {
			DownButton->setSpriteBank(sprites);
			DownButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_CURSOR_RIGHT), CurrentIconColor);
			DownButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_CURSOR_RIGHT), CurrentIconColor);
		}
		DownButton->setRelativePosition(recti(RelativeRect.getWidth() - w, 0, RelativeRect.getWidth(), h));
        DownButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT);
	} else {
		const s32 w = RelativeRect.getWidth();
		const s32 h = (w < RelativeRect.getHeight() / 2) ? w : RelativeRect.getHeight() / 2;
		if (!UpButton) {
			UpButton = new CGUIButton(Environment, this, -1, recti(0, 0, w, h), NoClip);
			UpButton->setSubElement(true);
			UpButton->setTabStop(false);
		}
		if (sprites) {
			UpButton->setSpriteBank(sprites);
			UpButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_CURSOR_UP), CurrentIconColor);
			UpButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_CURSOR_UP), CurrentIconColor);
		}
		UpButton->setRelativePosition(recti(0, 0, w, h));
        UpButton->setAlignment(GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft);
		if (!DownButton) {
			DownButton = new CGUIButton(Environment, this, -1, recti(0, RelativeRect.getHeight() - h, w, RelativeRect.getHeight()), NoClip);
			DownButton->setSubElement(true);
			DownButton->setTabStop(false);
		}
		if (sprites) {
			DownButton->setSpriteBank(sprites);
			DownButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_CURSOR_DOWN), CurrentIconColor);
			DownButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_CURSOR_DOWN), CurrentIconColor);
		}
		DownButton->setRelativePosition(recti(0, RelativeRect.getHeight() - h, w, RelativeRect.getHeight()));
        DownButton->setAlignment(GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT);
	}
}

} // end namespace gui
