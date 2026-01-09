// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "guiItemImage.h"
#include "client/core/client.h"
#include "client/render/rendersystem.h"
#include "client/ui/hud.h" // drawItemStack
#include "gui/IGUIEnvironment.h"
#include "inventory.h"
#include <Render/TTFont.h>
#include "client/ui/text_sprite.h"

GUIItemImage::GUIItemImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
	s32 id, const recti &rectangle, const std::string &item_name,
	render::TTFont *font, Client *client) :
	gui::IGUIElement(EGUIET_ELEMENT, env, parent, id, rectangle),
    m_item_name(item_name), m_font(font), m_client(client), m_label(),
    drawBatch(std::make_unique<SpriteDrawBatch>(env->getRenderSystem(), env->getResourceCache()))
{
    m_text = drawBatch->addTextSprite(L"");
    m_text->getTextObj().setOverrideFont(m_font);
}

void GUIItemImage::updateMesh()
{
    if (!Rebuild)
        return;
    IItemDefManager *idef = m_client->idef();
    ItemStack item;
    item.deSerialize(m_item_name, idef);
    // Viewport rectangle on screen
    //recti rect = recti(AbsoluteRect);
    //drawItemStack(Environment->getVideoDriver(), m_font, item, rect,
    //		&AbsoluteClippingRect, m_client, IT_ROT_NONE);
    img::color8 color = img::white;

    m_text->setText(m_label);
    m_text->setBoundRect(toRectT<f32>(AbsoluteRect));
    m_text->setClipRect(AbsoluteClippingRect);

    drawBatch->rebuild();

    Rebuild = false;
}

void GUIItemImage::draw()
{
	if (!IsVisible)
		return;

	if (!m_client) {
		IGUIElement::draw();
		return;
	}

    updateMesh();
    drawBatch->draw();

	IGUIElement::draw();
}
