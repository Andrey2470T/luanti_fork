// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUIImage.h"

namespace irr
{
namespace gui
{

class CGUIImage : public IGUIImage
{
public:
	//! constructor
    CGUIImage(IGUIEnvironment *environment, IGUIElement *parent, s32 id, recti rectangle);

	//! destructor
	virtual ~CGUIImage();

	//! sets an image
    void setImage(render::Texture2D *image) override;

	//! Gets the image texture
    render::Texture2D *getImage() const override;

	//! sets the color of the image
    void setColor(img::color8 color) override;

	//! sets if the image should scale to fit the element
	void setScaleImage(bool scale) override;

	//! draws the element and its children
	void draw() override;

	//! sets if the image should use its alpha channel to draw itself
	void setUseAlphaChannel(bool use) override;

	//! Gets the color of the image
    img::color8 getColor() const override;

	//! Returns true if the image is scaled to fit, false if not
	bool isImageScaled() const override;

	//! Returns true if the image is using the alpha channel, false if not
	bool isAlphaChannelUsed() const override;

	//! Sets the source rectangle of the image. By default the full image is used.
    void setSourceRect(const recti &sourceRect) override;

	//! Returns the customized source rectangle of the image to be used.
    recti getSourceRect() const override;

	//! Restrict drawing-area.
    void setDrawBounds(const rectf &drawBoundUVs) override;

	//! Get drawing-area restrictions.
    rectf getDrawBounds() const override;

	//! Sets whether to draw a background color (EGDC_3D_DARK_SHADOW) when no texture is set
	void setDrawBackground(bool draw) override
	{
		DrawBackground = draw;
	}

	//! Checks if a background is drawn when no texture is set
	bool isDrawBackgroundEnabled() const override
	{
		return DrawBackground;
	}

protected:
    void checkBounds(recti &rect)
	{
		f32 clipWidth = (f32)rect.getWidth();
		f32 clipHeight = (f32)rect.getHeight();

		rect.UpperLeftCorner.X += core::round32(DrawBounds.UpperLeftCorner.X * clipWidth);
		rect.UpperLeftCorner.Y += core::round32(DrawBounds.UpperLeftCorner.Y * clipHeight);
		rect.LowerRightCorner.X -= core::round32((1.f - DrawBounds.LowerRightCorner.X) * clipWidth);
		rect.LowerRightCorner.Y -= core::round32((1.f - DrawBounds.LowerRightCorner.Y) * clipHeight);
	}

private:
    render::Texture2D *Texture;
    img::color8 Color;
	bool UseAlphaChannel;
	bool ScaleImage;
    recti SourceRect;
    rectf DrawBounds;
	bool DrawBackground;
};

} // end namespace gui
} // end namespace irr
