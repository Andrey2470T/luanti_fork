// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUISpriteBank.h"

namespace irr
{

namespace video
{
class IVideoDriver;
class ITexture;
}

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
    render::Texture2D *getTexture(u32 index) const override;
    void addTexture(render::Texture2D *texture) override;
    void setTexture(u32 index, render::Texture2D *texture) override;

	//! Add the texture and use it for a single non-animated sprite.
    s32 addTextureAsSprite(render::Texture2D *texture) override;

	//! clears sprites, rectangles and textures
	void clear() override;

	//! Draws a sprite in 2d with position and color
    virtual void draw2DSprite(u32 index, const v2i &pos, const recti *clip = 0,
            const img::color8 &color = img::color8(255, 255, 255, 255),
			u32 starttime = 0, u32 currenttime = 0, bool loop = true, bool center = false) override;

	//! Draws a sprite in 2d with destination rectangle and colors
    virtual void draw2DSprite(u32 index, const recti &destRect,
            const recti *clip = 0,
            const img::color8 *const colors = 0,
			u32 timeTicks = 0,
			bool loop = true) override;

	//! Draws a sprite batch in 2d using an array of positions and a color
    virtual void draw2DSpriteBatch(const std::vector<u32> &indices, const std::vector<v2i> &pos,
            const recti *clip = 0,
            const img::color8 &color = img::color8(255, 255, 255, 255),
			u32 starttime = 0, u32 currenttime = 0,
			bool loop = true, bool center = false) override;

protected:
	bool getFrameNr(u32 &frameNr, u32 index, u32 time, bool loop) const;

	struct SDrawBatch
	{
        std::vector<v2i> positions;
        std::vector<core::recti> sourceRects;
		u32 textureNumber;
	};

    std::vector<SGUISprite> Sprites;
    std::vector<recti> Rectangles;
    std::vector<render::Texture2D *> Textures;
	IGUIEnvironment *Environment;
	video::IVideoDriver *Driver;
};

} // end namespace gui
} // end namespace irr
