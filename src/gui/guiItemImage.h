// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "IGUIElement.h"
#include "util/string.h"
#include <Render/TTFont.h>

class Client;
class UITextSprite;

class GUIItemImage : public gui::IGUIElement
{
public:
	GUIItemImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
		const recti &rectangle, const std::string &item_name,
		render::TTFont *font, Client *client);

	virtual void draw() override;

	virtual void setText(const wchar_t *text) override
	{
		m_label = text;
	}

private:
	std::string m_item_name;
	render::TTFont *m_font;
	Client *m_client;
    std::wstring m_label;

    std::unique_ptr<UITextSprite> m_text;
};
