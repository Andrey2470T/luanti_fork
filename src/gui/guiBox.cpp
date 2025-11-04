// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "guiBox.h"
#include "client/render/rendersystem.h"
#include "client/render/atlas.h"
#include "client/ui/extra_images.h"
#include "gui/IGUIEnvironment.h"

GUIBox::GUIBox(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
	const recti &rectangle,
	const std::array<img::color8, 4> &colors,
	const std::array<img::color8, 4> &bordercolors,
	const std::array<s32, 4> &borderwidths) :
	gui::IGUIElement(EGUIET_ELEMENT, env, parent, id, rectangle),
	m_colors(colors),
	m_bordercolors(bordercolors),
    m_borderwidths(borderwidths),
    m_box(std::make_unique<UIRects>(env->getRenderSystem(), 5))
{}

void GUIBox::draw()
{
	if (!IsVisible)
		return;

	std::array<s32, 4> negative_borders = {0, 0, 0, 0};
	std::array<s32, 4> positive_borders = {0, 0, 0, 0};

	for (size_t i = 0; i <= 3; i++) {
		if (m_borderwidths[i] > 0)
			positive_borders[i] = m_borderwidths[i];
		else
			negative_borders[i] = m_borderwidths[i];
	}

	v2i upperleft = AbsoluteRect.ULC;
	v2i lowerright = AbsoluteRect.LRC;

	v2i topleft_border = {
		upperleft.X - positive_borders[3],
		upperleft.Y - positive_borders[0]
	};
	v2i topleft_rect = {
		upperleft.X - negative_borders[3],
		upperleft.Y - negative_borders[0]
	};

	v2i lowerright_border = {
		lowerright.X + positive_borders[1],
		lowerright.Y + positive_borders[2]
	};
	v2i lowerright_rect = {
		lowerright.X + negative_borders[1],
		lowerright.Y + negative_borders[2]
	};

	recti main_rect(
		topleft_rect.X,
		topleft_rect.Y,
		lowerright_rect.X,
		lowerright_rect.Y
	);

	std::array<recti, 4> border_rects;

	border_rects[0] = recti(
		topleft_border.X,
		topleft_border.Y,
		lowerright_border.X,
		topleft_rect.Y
	);

	border_rects[1] = recti(
		lowerright_rect.X,
		topleft_rect.Y,
		lowerright_border.X,
		lowerright_rect.Y
	);

	border_rects[2] = recti(
		topleft_border.X,
		lowerright_rect.Y,
		lowerright_border.X,
		lowerright_border.Y
	);

	border_rects[3] = recti(
		topleft_border.X,
		topleft_rect.Y,
		topleft_rect.X,
		lowerright_rect.Y
	);

    m_box->updateRect(0, toRectT<f32>(main_rect), {m_colors[0], m_colors[1], m_colors[3], m_colors[2]});

    for (size_t i = 0; i < 4; i++)
        m_box->updateRect(i+1, toRectT<f32>(border_rects[i]),
            {m_bordercolors[i], m_bordercolors[i], m_bordercolors[i], m_bordercolors[i]});

    m_box->updateMesh();

    m_box->setClipRect(AbsoluteClippingRect);
    m_box->draw();

    IGUIElement::draw();
}
