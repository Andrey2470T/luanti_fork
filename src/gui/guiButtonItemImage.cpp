// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "guiButtonItemImage.h"

#include "client/core/client.h"
//#include "client/ui/hud.h" // drawItemStack
#include "guiItemImage.h"
#include "IGUIEnvironment.h"
#include "itemdef.h"

using namespace gui;

GUIButtonItemImage::GUIButtonItemImage(gui::IGUIEnvironment *environment,
		gui::IGUIElement *parent, s32 id, recti rectangle,
        const std::string &item, Client *client,
		bool noclip)
        : GUIButton (environment, parent, id, rectangle,noclip)
{
	m_image = new GUIItemImage(environment, this, id,
			recti(0,0,rectangle.getWidth(),rectangle.getHeight()),
			item, getActiveFont(), client);
	sendToBack(m_image);

	m_client = client;
}

GUIButtonItemImage *GUIButtonItemImage::addButton(IGUIEnvironment *environment,
        const recti &rectangle,
		IGUIElement *parent, s32 id, const wchar_t *text, const std::string &item,
		Client *client)
{
	GUIButtonItemImage *button = new GUIButtonItemImage(environment,
			parent ? parent : environment->getRootGUIElement(),
            id, rectangle, item, client);

	if (text)
		button->setText(text);

	button->drop();
	return button;
}
