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

#pragma once

#include "IGUIElement.h"
#include "util/string.h"
#include <Image/Image.h>
#include <variant>

class ImageSprite;
class Image2D9Slice;

class GUIBackgroundImage : public gui::IGUIElement
{
public:
	GUIBackgroundImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
		const recti &rectangle, const std::string &name,
        const recti &middle, bool autoclip,
        v2i autoclip_offset);

    void setTexture(img::Image *img)
    {
        texture = img;
    }
	virtual void draw() override;

private:
    img::Image *texture;
	std::string m_name;
	recti m_middle;
	bool m_autoclip;
    v2i m_autoclip_offset;

    std::variant<std::shared_ptr<ImageSprite>, std::shared_ptr<Image2D9Slice>> Image;
};
