/*
Part of Minetest
Copyright (C) 2013 RealBadAngel, Maciej Kasatkin <mk@realbadangel.pl>

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "guiBackgroundImage.h"
#include "IGUIEnvironment.h"
#include "log.h"
#include "client/ui/extra_images.h"
#include "client/render/rendersystem.h"
#include "client/render/atlas.h"

GUIBackgroundImage::GUIBackgroundImage(gui::IGUIEnvironment *env,
    gui::IGUIElement *parent, s32 id, const recti &rectangle,
    const std::string &name, const recti &middle, bool autoclip, v2i autoclip_offset) :
	gui::IGUIElement(EGUIET_ELEMENT, env, parent, id, rectangle),
    m_name(name), m_middle(middle), m_autoclip(autoclip),
    m_autoclip_offset(autoclip_offset)
{
    if (m_middle.getArea() == 0) {
        Image = std::make_unique<ImageSprite>(env->getRenderSystem(),
            env->getResourceCache());
    }
    else {
        Image = std::make_unique<Image2D9Slice>(env->getResourceCache(),
            env->getRenderSystem()->getRenderer(), nullptr,
            std::array<img::color8, 4>{img::white, img::white, img::white, img::white});
    }
}

void GUIBackgroundImage::draw()
{
	if (!IsVisible)
		return;

	if (!texture) {
		errorstream << "GUIBackgroundImage::draw() Unable to load texture:"
				<< std::endl;
		errorstream << "\t" << m_name << std::endl;
		return;
	}

	recti rect;
	if (m_autoclip) {
		rect = Parent->getAbsoluteClippingRect();
        rect.ULC -= m_autoclip_offset;
        rect.LRC += m_autoclip_offset;
	} else {
		rect = AbsoluteRect;
	}

    recti srcrect(v2i(0, 0), toV2i(texture->getSize()));

	if (m_middle.getArea() == 0) {
        std::array<img::color8, 4> colors = {img::white, img::white, img::white, img::white};

        auto img = std::get<std::shared_ptr<ImageSprite>>(Image);
        img->update(texture, toRectf(rect), colors, &srcrect);
        img->draw();
	} else {
        auto img = std::get<std::shared_ptr<Image2D9Slice>>(Image);
        img->updateRects(toRectf(srcrect), toRectf(rect), toRectf(m_middle));
        img->createSlices();
        img->draw();
	}

	IGUIElement::draw();
}
