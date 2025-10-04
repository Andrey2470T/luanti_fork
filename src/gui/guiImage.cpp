// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiImage.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include "client/ui/extra_images.h"
#include "client/render/rendersystem.h"
#include "client/render/atlas.h"
#include "client/render/rectpack2d_atlas.h"

namespace gui
{

//! constructor
CGUIImage::CGUIImage(IGUIEnvironment *environment, IGUIElement *parent, s32 id, recti rectangle, const rectf &middle) :
        IGUIImage(environment, parent, id, rectangle), Texture(0), Color(img::white),
        UseAlphaChannel(false), ScaleImage(false), DrawBounds(0.f, 0.f, 1.f, 1.f), DrawBackground(true),
        MiddleRect(middle),
        Image(std::make_unique<ImageSprite>(Environment->getRenderSystem(),
            Environment->getResourceCache()))
{
    if (MiddleRect.getArea() == 0) {
        Image = std::make_shared<ImageSprite>(Environment->getRenderSystem(),
            Environment->getResourceCache());
    }
    else {
        Image = std::make_shared<Image2D9Slice>(environment->getResourceCache(),
            environment->getRenderSystem());
    }
}

//! sets an image
void CGUIImage::setImage(img::Image *image, std::optional<AtlasTileAnim> animParams,
    std::optional<u32> offset)
{
	if (image == Texture)
		return;

	Texture = image;
    AnimParams = animParams;
    FrameOffset = offset;
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

    auto pool = Environment->getRenderSystem()->getPool(false);
	if (Texture) {
		recti sourceRect(SourceRect);
		if (sourceRect.getWidth() == 0 || sourceRect.getHeight() == 0) {
            sourceRect = recti(v2i(Texture->getSize().X ,Texture->getSize().Y));
		}

        recti clippingRect;
        std::array<img::color8, 4> Colors;
		if (ScaleImage) {
            Colors = {Color, Color, Color, Color};

            clippingRect = AbsoluteClippingRect;
			checkBounds(clippingRect);
		} else {
            clippingRect = recti(AbsoluteRect.ULC, sourceRect.getSize());
			checkBounds(clippingRect);
			clippingRect.clipAgainst(AbsoluteClippingRect);
		}
        if (MiddleRect.getArea() == 0) {
            auto img = std::get<std::shared_ptr<ImageSprite>>(Image);
            img->update(Texture, toRectf(AbsoluteRect), Colors, &clippingRect, AnimParams);

            if (FrameOffset)
                pool->getAnimatedTileByImage(Texture)->frame_offset = FrameOffset.value();
            img->draw();
        }
        else {
            auto sliced_img = std::get<std::shared_ptr<Image2D9Slice>>(Image);
            sliced_img->updateRects(toRectf(sourceRect), MiddleRect, toRectf(AbsoluteRect),
                Texture, Colors, &clippingRect, AnimParams);

            if (FrameOffset)
                pool->getAnimatedTileByImage(Texture)->frame_offset = FrameOffset.value();
            sliced_img->draw();
        }
	} else if (DrawBackground) {
		recti clippingRect(AbsoluteClippingRect);
		checkBounds(clippingRect);

        auto color = skin->getColor(EGDC_3D_DARK_SHADOW);
        std::array<img::color8, 4> Colors = {color, color, color, color};
        if (MiddleRect.getArea() == 0) {
            auto img = std::get<std::shared_ptr<ImageSprite>>(Image);
            img->update(nullptr, toRectf(AbsoluteRect), Colors, &clippingRect, AnimParams);

            if (FrameOffset)
                pool->getAnimatedTileByImage(Texture)->frame_offset = FrameOffset.value();
            img->draw();
        }
        else {
            auto sliced_img = std::get<std::shared_ptr<Image2D9Slice>>(Image);
            sliced_img->updateRects(toRectf(SourceRect), MiddleRect, toRectf(AbsoluteRect),
                nullptr, Colors, &clippingRect, AnimParams);

            if (FrameOffset)
                pool->getAnimatedTileByImage(Texture)->frame_offset = FrameOffset.value();
            sliced_img->draw();
        }
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
