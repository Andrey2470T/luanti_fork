// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIImage.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"

namespace gui
{

//! constructor
CGUIImage::CGUIImage(IGUIEnvironment *environment, IGUIElement *parent, s32 id, recti rectangle) :
        IGUIImage(environment, parent, id, rectangle), Texture(0), Color(img::white),
		UseAlphaChannel(false), ScaleImage(false), DrawBounds(0.f, 0.f, 1.f, 1.f), DrawBackground(true)
{}

//! sets an image
void CGUIImage::setImage(img::Image *image)
{
	if (image == Texture)
		return;

	Texture = image;
}

//! Gets the image texture
img::Image *CGUIImage::getImage() const
{
	return Texture;
}

//! sets the color of the image
void CGUIImage::setColor(img::color8 color)
{
	Color = color;
}

//! Gets the color of the image
img::color8 CGUIImage::getColor() const
{
	return Color;
}

//! draws the element and its children
void CGUIImage::draw()
{
	if (!IsVisible)
		return;

	GUISkin *skin = Environment->getSkin();

	if (Texture) {
		recti sourceRect(SourceRect);
		if (sourceRect.getWidth() == 0 || sourceRect.getHeight() == 0) {
            sourceRect = recti(v2i(Texture->getSize().X ,Texture->getSize().Y));
		}

		if (ScaleImage) {
			const img::color8 Colors[] = {Color, Color, Color, Color};

			recti clippingRect(AbsoluteClippingRect);
			checkBounds(clippingRect);

			driver->draw2DImage(Texture, AbsoluteRect, sourceRect,
					&clippingRect, Colors, UseAlphaChannel);
		} else {
			recti clippingRect(AbsoluteRect.ULC, sourceRect.getSize());
			checkBounds(clippingRect);
			clippingRect.clipAgainst(AbsoluteClippingRect);

			driver->draw2DImage(Texture, AbsoluteRect.ULC, sourceRect,
					&clippingRect, Color, UseAlphaChannel);
		}
	} else if (DrawBackground) {
		recti clippingRect(AbsoluteClippingRect);
		checkBounds(clippingRect);

		skin->draw2DRectangle(this, skin->getColor(EGDC_3D_DARK_SHADOW), AbsoluteRect, &clippingRect);
	}

	IGUIElement::draw();
}

//! sets if the image should use its alpha channel to draw itself
void CGUIImage::setUseAlphaChannel(bool use)
{
	UseAlphaChannel = use;
}

//! sets if the image should use its alpha channel to draw itself
void CGUIImage::setScaleImage(bool scale)
{
	ScaleImage = scale;
}

//! Returns true if the image is scaled to fit, false if not
bool CGUIImage::isImageScaled() const
{
	return ScaleImage;
}

//! Returns true if the image is using the alpha channel, false if not
bool CGUIImage::isAlphaChannelUsed() const
{
	return UseAlphaChannel;
}

//! Sets the source rectangle of the image. By default the full image is used.
void CGUIImage::setSourceRect(const recti &sourceRect)
{
	SourceRect = sourceRect;
}

//! Returns the customized source rectangle of the image to be used.
recti CGUIImage::getSourceRect() const
{
	return SourceRect;
}

//! Restrict target drawing-area.
void CGUIImage::setDrawBounds(const rectf &drawBoundUVs)
{
	DrawBounds = drawBoundUVs;
    DrawBounds.ULC.X = std::clamp(DrawBounds.ULC.X, 0.f, 1.f);
    DrawBounds.ULC.Y = std::clamp(DrawBounds.ULC.Y, 0.f, 1.f);
    DrawBounds.LRC.X = std::clamp(DrawBounds.LRC.X, 0.f, 1.f);
    DrawBounds.LRC.X = std::clamp(DrawBounds.LRC.X, 0.f, 1.f);
	if (DrawBounds.ULC.X > DrawBounds.LRC.X)
		DrawBounds.ULC.X = DrawBounds.LRC.X;
	if (DrawBounds.ULC.Y > DrawBounds.LRC.Y)
		DrawBounds.ULC.Y = DrawBounds.LRC.Y;
}

//! Get target drawing-area restrictions.
rectf CGUIImage::getDrawBounds() const
{
	return DrawBounds;
}

} // end namespace gui
