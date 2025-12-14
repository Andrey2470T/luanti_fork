// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiCheckBox.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include <Core/TimeCounter.h>
#include "client/media/resource.h"
#include "client/ui/extra_images.h"
#include "client/ui/text_sprite.h"
#include "client/render/rendersystem.h"
#include <Utils/TypeConverter.h>

namespace gui
{

//! constructor
CGUICheckBox::CGUICheckBox(bool checked, IGUIEnvironment *environment, IGUIElement *parent, s32 id, recti rectangle) :
        IGUICheckBox(environment, parent, id, rectangle), CheckTime(0), Pressed(false), Checked(checked), Border(false), Background(false),
        CheckBoxBank(std::make_unique<UISpriteBank>(environment->getRenderSystem(), environment->getResourceCache()))
{
	// this element can be tabbed into
	setTabStop(true);
	setTabOrder(-1);
}

//! called if an event happened.
bool CGUICheckBox::OnEvent(const core::Event &event)
{
	if (isEnabled()) {
		switch (event.Type) {
		case EET_KEY_INPUT_EVENT:
			if (event.KeyInput.PressedDown &&
                    (event.KeyInput.Key == core::KEY_RETURN || event.KeyInput.Key == core::KEY_SPACE)) {
                setPressed(true);
				return true;
            } else if (Pressed && event.KeyInput.PressedDown && event.KeyInput.Key == core::KEY_ESCAPE) {
                setPressed(false);
				return true;
			} else if (!event.KeyInput.PressedDown && Pressed &&
                       (event.KeyInput.Key == core::KEY_RETURN || event.KeyInput.Key == core::KEY_SPACE)) {
                setPressed(false);
				if (Parent) {
					core::Event newEvent;
					newEvent.Type = EET_GUI_EVENT;
                    newEvent.GUI.Caller = this;
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
                if (event.GUI.Caller == this)
                    setPressed(false);
			}
			break;
		case EET_MOUSE_INPUT_EVENT:
			if (event.MouseInput.Type == EMIE_LMOUSE_PRESSED_DOWN) {
                setPressed(true);
				CheckTime = core::TimeCounter::getRealTime();
				return true;
			} else if (event.MouseInput.Type == EMIE_LMOUSE_LEFT_UP) {
				bool wasPressed = Pressed;
                setPressed(false);

				if (wasPressed && Parent) {
					if (!AbsoluteClippingRect.isPointInside(v2i(event.MouseInput.X, event.MouseInput.Y))) {
                        setPressed(false);
						return true;
					}

					core::Event newEvent;
					newEvent.Type = EET_GUI_EVENT;
                    newEvent.GUI.Caller = this;
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

void CGUICheckBox::updateMesh()
{
    if (!Rebuild)
        return;
    GUISkin *skin = Environment->getSkin();

    CheckBoxBank->clear();

    if (skin) {
        UIRects *CheckBoxRects = CheckBoxBank->addSprite({}, &AbsoluteClippingRect);

        recti frameRect(AbsoluteRect);

        // draw background
        if (Background) {
            img::color8 bgColor = skin->getColor(EGDC_3D_FACE);
            CheckBoxRects->addRect(toRectT<f32>(frameRect), {bgColor, bgColor, bgColor, bgColor});
        }

        // draw the border
        if (Border) {
            skin->add3DSunkenPane(CheckBoxRects, img::white, true, false, toRectT<f32>(frameRect));
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

        skin->add3DSunkenPane(CheckBoxRects, skin->getColor(col),
                false, true, toRectT<f32>(checkRect));

        CheckBoxRects->rebuildMesh();
        CheckBoxRects->draw();

        // the checked icon
        if (Checked) {
            u32 sprite_res = skin->getIcon(GUIDefaultIcon::CheckBoxChecked);
            std::string tex_name = "checkbox_" + std::to_string(sprite_res) + ".png";
            auto sprite_tex = Environment->getResourceCache()->get<img::Image>(ResourceType::IMAGE, tex_name);

            if (sprite_tex) {
                CheckBoxBank->addImageSprite(sprite_tex, toRectT<f32>(checkRect), &AbsoluteClippingRect);
            }
        }

        // associated text
        if (Text.size()) {
            checkRect = frameRect;
            checkRect.ULC.X += height + 5;

            render::TTFont *font = skin->getFont();
            if (font) {
                img::color8 text_c = skin->getColor(isEnabled() ? EGDC_BUTTON_TEXT : EGDC_GRAY_TEXT);
                CheckBoxBank->addTextSprite(Text, toRectT<f32>(checkRect), text_c);
            }
        }
    }

    Rebuild = false;
}

//! draws the element and its children
void CGUICheckBox::draw()
{
	if (!IsVisible)
		return;

    updateMesh();
    CheckBoxBank->drawBank();

    IGUIElement::draw();
}

void CGUICheckBox::setPressed(bool pressed)
{
    if (pressed != Pressed) Rebuild = true;
    Pressed = pressed;
}
//! set if box is checked
void CGUICheckBox::setChecked(bool checked)
{
    if (checked != Checked) Rebuild = true;
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
    if (draw != Background) Rebuild = true;
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
    if (draw != Border) Rebuild = true;
	Border = draw;
}

//! Checks if border drawing is enabled
bool CGUICheckBox::isDrawBorderEnabled() const
{
	return Border;
}

} // end namespace gui
