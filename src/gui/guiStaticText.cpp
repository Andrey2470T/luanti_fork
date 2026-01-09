// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiStaticText.h"

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
        drawBatch(std::make_unique<SpriteDrawBatch>(environment->getRenderSystem(), environment->getResourceCache()))
{
    Text = drawBatch->addTextSprite(text);
    Text->getTextObj().enableDrawBorder(border);
    Text->getTextObj().enableDrawBackground(background);
}

void CGUIStaticText::updateMesh()
{
    if (!Rebuild)
        return;
    Text->setBoundRect(rectf(v2f(AbsoluteRect.ULC.X, AbsoluteRect.ULC.Y), v2f(AbsoluteRect.LRC.X, AbsoluteRect.LRC.Y)));
    drawBatch->rebuild();

    Rebuild = false;
}

//! draws the element and its children
void CGUIStaticText::draw()
{
	if (!IsVisible)
		return;

    updateMesh();
    drawBatch->draw();

    IGUIElement::draw();
}

//! Sets another skin independent font.
void CGUIStaticText::setOverrideFont(render::TTFont *font)
{
    Text->getTextObj().setOverrideFont(font);
    Rebuild = true;
}

//! Gets the override font (if any)
render::TTFont *CGUIStaticText::getOverrideFont() const
{
    return Text->getTextObj().getOverrideFont();
}

//! Get the font which is used right now for drawing
render::TTFont *CGUIStaticText::getActiveFont() const
{
    return Text->getTextObj().getActiveFont();
}

//! Sets another color for the text.
void CGUIStaticText::setOverrideColor(img::color8 color)
{
    Text->getTextObj().setOverrideColor(color);
    Rebuild = true;
}

//! Sets another color for the text.
void CGUIStaticText::setBackgroundColor(img::color8 color)
{
    Text->getTextObj().setBackgroundColor(color);
    Rebuild = true;
}

//! Sets whether to draw the background
void CGUIStaticText::setDrawBackground(bool draw)
{
    Text->getTextObj().enableDrawBackground(draw);
    Rebuild = true;
}

//! Gets the background color
img::color8 CGUIStaticText::getBackgroundColor() const
{
    return Text->getTextObj().getBackgroundColor();
}

//! Checks if background drawing is enabled
bool CGUIStaticText::isDrawBackgroundEnabled() const
{
    return Text->getTextObj().isDrawBackground();
}

//! Sets whether to draw the border
void CGUIStaticText::setDrawBorder(bool draw)
{
    Text->getTextObj().enableDrawBorder(draw);
    Rebuild = true;
}

//! Checks if border drawing is enabled
bool CGUIStaticText::isDrawBorderEnabled() const
{
    return Text->getTextObj().isDrawBorder();
}

void CGUIStaticText::setTextRestrainedInside(bool restrainTextInside)
{
    Text->getTextObj().enableClipText(restrainTextInside);
    Rebuild = true;
}

bool CGUIStaticText::isTextRestrainedInside() const
{
    return Text->getTextObj().isClipText();
}

void CGUIStaticText::setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical)
{
    Text->getTextObj().setAlignment(horizontal, vertical);
    Rebuild = true;
}

img::color8 CGUIStaticText::getOverrideColor() const
{
    return Text->getTextObj().getOverrideColor();
}

img::color8 CGUIStaticText::getActiveColor() const
{
    return Text->getTextObj().getActiveColor();
}

//! Sets if the static text should use the override color or the
//! color in the gui skin.
void CGUIStaticText::enableOverrideColor(bool enable)
{
    Text->getTextObj().enableOverrideColor(enable);
    Rebuild = true;
}

bool CGUIStaticText::isOverrideColorEnabled() const
{
    return Text->getTextObj().isOverrideColorEnabled();
}

//! Enables or disables word wrap for using the static text as
//! multiline text control.
void CGUIStaticText::setWordWrap(bool enable)
{
    Text->getTextObj().enableWordWrap(enable);
    Rebuild = true;
}

bool CGUIStaticText::isWordWrapEnabled() const
{
    return Text->getTextObj().isWordWrap();
}

void CGUIStaticText::setText(const wchar_t *text)
{
    IGUIElement::setText(text);
    Text->setText(text);
}

void CGUIStaticText::setRightToLeft(bool rtl)
{
    Text->getTextObj().enableRightToLeft(rtl);
    Rebuild = true;
}

bool CGUIStaticText::isRightToLeft() const
{
    return Text->getTextObj().isRightToLeft();
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
