// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "guiButton.h"
#include "guiImage.h"
#include "guiSharedPointer.h"

class GUIButtonImage : public GUIButton
{
public:
	//! constructor
	GUIButtonImage(gui::IGUIEnvironment *environment, gui::IGUIElement *parent,
            s32 id, recti rectangle, bool noclip = false);

    void setForegroundImage(img::Image *image = nullptr,
			const recti &middle = recti());

	//! Set element properties from a StyleSpec
	virtual void setFromStyle(const StyleSpec &style) override;

	//! Do not drop returned handle
	static GUIButtonImage *addButton(gui::IGUIEnvironment *environment,
            const recti &rectangle,
			IGUIElement *parent, s32 id, const wchar_t *text,
			const wchar_t *tooltiptext = L"");

private:
    img::Image *m_foreground_image = nullptr;
    gui::GUISharedPointer<gui::CGUIImage> m_image;
};
