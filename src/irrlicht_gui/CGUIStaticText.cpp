// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIStaticText.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include <Utils/Rect.h>
#include "client/ui/text_sprite.h"
#include "client/render/rendersystem.h"

namespace gui
{

//! constructor
CGUIStaticText::CGUIStaticText(const wchar_t *text, bool border,
		IGUIEnvironment *environment, IGUIElement *parent,
		s32 id, const recti &rectangle,
		bool background) :
		IGUIStaticText(environment, parent, id, rectangle),
        Text(std::make_unique<UITextSprite>(
            Environment->getRenderSystem()->getFontManager(), EnrichedString(text),
            Environment->getRenderSystem()->getRenderer(), Environment->getResourceCache(),
            border, false, background))
{

    img::color8 BGColor(img::PF_RGBA8, 255, 255, 255, 101);
	if (environment && environment->getSkin()) {
        BGColor = environment->getSkin()->getColor(EGDC_3D_FACE);
	}

    Text->setColor(img::color8(img::PF_RGBA8, 255, 255, 255, 101));
    Text->setBackgroundColor(BGColor);
}

//! draws the element and its children
void CGUIStaticText::draw()
{
	if (!IsVisible)
		return;


    Text->updateBuffer(rectf(v2f(AbsoluteRect.ULC.X, AbsoluteRect.ULC.Y), v2f(AbsoluteRect.LRC.X, AbsoluteRect.LRC.Y)));
    Text->draw();
}

//! Sets another skin independent font.
void CGUIStaticText::setOverrideFont(render::TTFont *font)
{
    Text->setOverrideFont(font);
}

//! Gets the override font (if any)
render::TTFont *CGUIStaticText::getOverrideFont() const
{
    return Text->getOverrideFont();
}

//! Get the font which is used right now for drawing
render::TTFont *CGUIStaticText::getActiveFont() const
{
    return Text->getActiveFont();
}

//! Sets another color for the text.
void CGUIStaticText::setOverrideColor(img::color8 color)
{
    Text->setColor(color);
}

//! Sets another color for the text.
void CGUIStaticText::setBackgroundColor(img::color8 color)
{
    Text->setBackgroundColor(color);
}

//! Sets whether to draw the background
void CGUIStaticText::setDrawBackground(bool draw)
{
    Text->enableDrawBackground(draw);
}

//! Gets the background color
img::color8 CGUIStaticText::getBackgroundColor() const
{
    return Text->getBackgroundColor();
}

//! Checks if background drawing is enabled
bool CGUIStaticText::isDrawBackgroundEnabled() const
{
    return Text->isDrawBackground();
}

//! Sets whether to draw the border
void CGUIStaticText::setDrawBorder(bool draw)
{
    Text->enableDrawBorder(draw);
}

//! Checks if border drawing is enabled
bool CGUIStaticText::isDrawBorderEnabled() const
{
    return Text->isDrawBorder();
}

void CGUIStaticText::setTextRestrainedInside(bool restrainTextInside)
{
    Text->enableClipText(restrainTextInside);
}

bool CGUIStaticText::isTextRestrainedInside() const
{
    return Text->isClipText();
}

void CGUIStaticText::setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical)
{
    Text->setAlignment(horizontal, vertical);
}

img::color8 CGUIStaticText::getOverrideColor() const
{
    return Text->getColor();
}

img::color8 CGUIStaticText::getActiveColor() const
{
    return Text->getColor();
}

//! Sets if the static text should use the override color or the
//! color in the gui skin.
void CGUIStaticText::enableOverrideColor(bool enable)
{
}

bool CGUIStaticText::isOverrideColorEnabled() const
{
    return false;
}

//! Enables or disables word wrap for using the static text as
//! multiline text control.
void CGUIStaticText::setWordWrap(bool enable)
{
    Text->enableWordWrap(enable);
}

bool CGUIStaticText::isWordWrapEnabled() const
{
    return Text->isWordWrap();
}

void CGUIStaticText::setRightToLeft(bool rtl)
{
    Text->enableRightToLeft(rtl);
}

bool CGUIStaticText::isRightToLeft() const
{
    return Text->isRightToLeft();
}

void CGUIStaticText::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();
    Text->setText(Text->getText()); // to trigger the text breaking
}

//! Returns the height of the text in pixels when it is drawn.
s32 CGUIStaticText::getTextHeight() const
{
    return Text->getTextHeight();
}

s32 CGUIStaticText::getTextWidth() const
{
    return Text->getTextWidth();
}

} // end namespace gui
