// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUISpriteBank.h"

class ImageSprite;

namespace gui
{

class IGUIEnvironment;

//! Sprite bank interface.
class CGUISpriteBank : public IGUISpriteBank
{
public:
	CGUISpriteBank(IGUIEnvironment *env);
	virtual ~CGUISpriteBank();

    std::vector<recti> &getPositions() override;
    std::vector<SGUISprite> &getSprites() override;

	u32 getTextureCount() const override;
	img::Image *getTexture(u32 index) const override;
	void addTexture(img::Image *texture) override;
	void setTexture(u32 index, img::Image *texture) override;

	//! Add the texture and use it for a single non-animated sprite.
	s32 addTextureAsSprite(img::Image *texture) override;

	//! clears sprites, rectangles and textures
	void clear() override;

	//! Draws a sprite in 2d with position and color
	virtual void draw2DSprite(u32 index, const v2i &pos, const recti *clip = 0,
            const img::color8 &color = img::color8(img::white),
			u32 starttime = 0, u32 currenttime = 0, bool loop = true, bool center = false) override;

	//! Draws a sprite in 2d with destination rectangle and colors
	virtual void draw2DSprite(u32 index, const recti &destRect,
			const recti *clip = 0,
			const img::color8 *const colors = 0,
			u32 timeTicks = 0,
			bool loop = true) override;

protected:
	bool getFrameNr(u32 &frameNr, u32 index, u32 time, bool loop) const;

	struct SDrawBatch
	{
        std::vector<v2i> positions;
        std::vector<recti> sourceRects;
		u32 textureNumber;
	};

    std::vector<SGUISprite> Sprites;
    std::vector<recti> Rectangles;
    std::vector<img::Image *> Textures;
	IGUIEnvironment *Environment;

    std::unique_ptr<ImageSprite> SpriteBank;
};

} // end namespace gui
