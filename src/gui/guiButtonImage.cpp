// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "guiButtonImage.h"

#include "debug.h"
#include "IGUIEnvironment.h"
#include "client/render/rendersystem.h"
#include "StyleSpec.h"

using namespace gui;

GUIButtonImage::GUIButtonImage(gui::IGUIEnvironment *environment,
        gui::IGUIElement *parent, s32 id, recti rectangle, bool noclip)
    : GUIButton(environment, parent, id, rectangle, noclip)
{
	GUIButton::setScaleImage(true);
    m_image = gui::make_gui_shared<CGUIImage>(environment, this, id, rectangle);
	sendToBack(m_image.get());
}

void GUIButtonImage::setForegroundImage(img::Image *image,
		const recti &middle)
{
	if (image == m_foreground_image)
		return;

    m_foreground_image = image;
    m_image->setImage(m_foreground_image);
    m_image->setMiddleRect(toRectf(middle));
}

//! Set element properties from a StyleSpec
void GUIButtonImage::setFromStyle(const StyleSpec &style)
{
	GUIButton::setFromStyle(style);

	if (style.isNotDefault(StyleSpec::FGIMG)) {
		img::Image *texture = style.getTexture(StyleSpec::FGIMG,
                Environment->getResourceCache());

        setForegroundImage(texture,
                style.getRect(StyleSpec::FGIMG_MIDDLE, toRecti(m_image->getMiddleRect())));
	} else {
		setForegroundImage();
	}
}

GUIButtonImage *GUIButtonImage::addButton(IGUIEnvironment *environment,
        const recti &rectangle,
		IGUIElement *parent, s32 id, const wchar_t *text,
		const wchar_t *tooltiptext)
{
    auto button = new GUIButtonImage(environment,
            parent ? parent : environment->getRootGUIElement(), id, rectangle);

	if (text)
		button->setText(text);

	if (tooltiptext)
		button->setToolTipText(tooltiptext);

    return button;
}
