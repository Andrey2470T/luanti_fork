// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUISpriteBank.h"
#include "IGUIEnvironment.h"
#include "client/ui/sprite.h"
#include "client/render/rendersystem.h"
#include "client/render/atlas.h"
#include <Utils/TypeConverter.h>

namespace gui
{

CGUISpriteBank::CGUISpriteBank(IGUIEnvironment *env) :
        Environment(env),
        SpriteBank(std::make_unique<UISprite>(nullptr, Environment->getRenderSystem()->getRenderer(),
        Environment->getResourceCache(), rectf(), rectf(), std::array<img::color8, 4>{}))
{}

CGUISpriteBank::~CGUISpriteBank()
{
	clear();
}

std::vector<recti> &CGUISpriteBank::getPositions()
{
	return Rectangles;
}

std::vector<SGUISprite> &CGUISpriteBank::getSprites()
{
	return Sprites;
}

u32 CGUISpriteBank::getTextureCount() const
{
	return Textures.size();
}

img::Image *CGUISpriteBank::getTexture(u32 index) const
{
	if (index < Textures.size())
		return Textures[index];
	else
		return 0;
}

void CGUISpriteBank::addTexture(img::Image *texture)
{
	Textures.push_back(texture);
}

void CGUISpriteBank::setTexture(u32 index, img::Image *texture)
{
	while (index >= Textures.size())
		Textures.push_back(0);

	Textures[index] = texture;
}

//! clear everything
void CGUISpriteBank::clear()
{
	Textures.clear();
	Sprites.clear();
	Rectangles.clear();
}

//! Add the texture and use it for a single non-animated sprite.
s32 CGUISpriteBank::addTextureAsSprite(img::Image *texture)
{
	if (!texture)
		return -1;

	addTexture(texture);
	u32 textureIndex = getTextureCount() - 1;

	u32 rectangleIndex = Rectangles.size();
    Rectangles.push_back(recti(0, 0, texture->getSize().X, texture->getSize().Y));

	SGUISprite sprite;
	sprite.frameTime = 0;

	SGUISpriteFrame frame;
	frame.textureNumber = textureIndex;
	frame.rectNumber = rectangleIndex;
	sprite.Frames.push_back(frame);

	Sprites.push_back(sprite);

	return Sprites.size() - 1;
}

// get FrameNr for time. return true on exisiting frame
inline bool CGUISpriteBank::getFrameNr(u32 &frame, u32 index, u32 time, bool loop) const
{
	frame = 0;
	if (index >= Sprites.size())
		return false;

	const SGUISprite &sprite = Sprites[index];
	const u32 frameSize = sprite.Frames.size();
	if (frameSize < 1)
		return false;

	if (sprite.frameTime) {
		u32 f = (time / sprite.frameTime);
		if (loop)
			frame = f % frameSize;
		else
			frame = (f >= frameSize) ? frameSize - 1 : f;
	}
	return true;
}

//! draws a sprite in 2d with scale and color
void CGUISpriteBank::draw2DSprite(u32 index, const v2i &pos,
		const recti *clip, const img::color8 &color,
		u32 starttime, u32 currenttime, bool loop, bool center)
{
	u32 frame = 0;
	if (!getFrameNr(frame, index, currenttime - starttime, loop))
		return;

    img::Image *tex = getTexture(Sprites[index].Frames[frame].textureNumber);
	if (!tex)
		return;

	const u32 rn = Sprites[index].Frames[frame].rectNumber;
	if (rn >= Rectangles.size())
		return;

	const recti &r = Rectangles[rn];
	v2i p(pos);
	if (center) {
		p -= r.getSize() / 2;
	}

    auto rndsys = Environment->getRenderSystem();
    auto atlas = rndsys->getPool(false)->getAtlasByTile(tex, true);
    auto tilerect = rndsys->getPool(false)->getTileRect(tex, false, true);

    SpriteBank->setTexture(atlas->getTexture());

    auto shape = SpriteBank->getShape();
    shape->updateRectangle(0, toRectf(r), {color, color, color, color}, tilerect);
    SpriteBank->updateMesh(true);
    SpriteBank->updateMesh(false);

    SpriteBank->setClipRect(*clip);
    SpriteBank->draw();
}

void CGUISpriteBank::draw2DSprite(u32 index, const recti &destRect,
		const recti *clip, const img::color8 *const colors,
		u32 timeTicks, bool loop)
{
	u32 frame = 0;
	if (!getFrameNr(frame, index, timeTicks, loop))
		return;

    img::Image *tex = getTexture(Sprites[index].Frames[frame].textureNumber);
	if (!tex)
		return;

	const u32 rn = Sprites[index].Frames[frame].rectNumber;
	if (rn >= Rectangles.size())
		return;

    auto rndsys = Environment->getRenderSystem();
    auto atlas = rndsys->getPool(false)->getAtlasByTile(tex, true);
    auto tilerect = rndsys->getPool(false)->getTileRect(tex, false, true);

    SpriteBank->setTexture(atlas->getTexture());

    auto shape = SpriteBank->getShape();
    shape->updateRectangle(0, toRectf(destRect), {colors[0], colors[1], colors[2], colors[3]}, tilerect);
    SpriteBank->updateMesh(true);
    SpriteBank->updateMesh(false);

    SpriteBank->setClipRect(*clip);
    SpriteBank->draw();
}

} // namespace gui
