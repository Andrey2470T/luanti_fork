// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "IGUIElement.h"
#include <Image/Color.h>
#include <array>

class UIRects;

class GUIBox : public gui::IGUIElement
{
public:
	GUIBox(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
		const recti &rectangle,
		const std::array<img::color8, 4> &colors,
		const std::array<img::color8, 4> &bordercolors,
		const std::array<s32, 4> &borderwidths);
		
	void updateMesh() override;

	virtual void draw() override;

private:
	std::array<img::color8, 4> m_colors;
	std::array<img::color8, 4> m_bordercolors;
	std::array<s32, 4> m_borderwidths;

    std::unique_ptr<UIRects> m_box;
};
