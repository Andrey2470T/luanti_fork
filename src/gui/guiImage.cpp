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
        drawBatch(std::make_unique<SpriteDrawBatch>(environment->getRenderSystem(), environment->getResourceCache()))
{
    if (MiddleRect.getArea() == 0)
        drawBatch->addRectsSprite({{}});
    else
        drawBatch->addImage2D9Slice({}, {}, {}, nullptr);
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

    Rebuild = true;
}

//! Gets the image texture
img::Image *CGUIImage::getImage() const
{
	return Texture;
}

//! sets the color of the image
void CGUIImage::setColor(img::color8 color)
{
    if (color != Color) Rebuild = true;
	Color = color;
}

//! Gets the color of the image
img::color8 CGUIImage::getColor() const
{
	return Color;
}

void CGUIImage::updateMesh()
{
    if (!Rebuild)
        return;
    GUISkin *skin = Environment->getSkin();

    auto pool = Environment->getRenderSystem()->getPool(false);
    if (Texture) {
        recti sourceRect(SourceRect);
        if (sourceRect.getWidth() == 0 || sourceRect.getHeight() == 0) {
            sourceRect = recti(v2i(Texture->getSize().X, Texture->getSize().Y));
        }

        recti clippingRect;
        RectColors Colors;

        clippingRect = AbsoluteClippingRect;
        checkBounds(clippingRect);
        if (ScaleImage) {
            Colors = Color;
        } else {
            clippingRect.clipAgainst(AbsoluteClippingRect);
        }
        if (MiddleRect.getArea() == 0) {
            auto img = dynamic_cast<UIRects *>(drawBatch->getSprite(0));
            img->updateRect(0, {toRectT<f32>(AbsoluteRect), Colors, Texture, AnimParams});
            img->setClipRect(clippingRect);

            if (FrameOffset)
                pool->getAnimatedTileByImage(Texture)->frame_offset = FrameOffset.value();
        }
        else {
            auto sliced_img = dynamic_cast<Image2D9Slice *>(drawBatch->getSprite(0));
            sliced_img->updateRects(toRectT<f32>(sourceRect), MiddleRect, toRectT<f32>(AbsoluteRect),
                Texture, Colors, &clippingRect, AnimParams);

            if (FrameOffset)
                pool->getAnimatedTileByImage(Texture)->frame_offset = FrameOffset.value();
        }
    } else if (DrawBackground) {
        recti clippingRect(AbsoluteClippingRect);
        checkBounds(clippingRect);

        auto color = skin->getColor(EGDC_3D_DARK_SHADOW);
        RectColors Colors = color;
        if (MiddleRect.getArea() == 0) {
            auto img = dynamic_cast<UIRects *>(drawBatch->getSprite(0));
            img->updateRect(0, {toRectT<f32>(AbsoluteRect), Colors});
            img->setClipRect(clippingRect);
        }
        else {
            auto sliced_img = dynamic_cast<Image2D9Slice *>(drawBatch->getSprite(0));
            sliced_img->updateRects(toRectT<f32>(SourceRect), MiddleRect, toRectT<f32>(AbsoluteRect),
                nullptr, Colors, &clippingRect, AnimParams);
        }
    }

    drawBatch->rebuild();

    Rebuild = false;
}

//! draws the element and its children
void CGUIImage::draw()
{
	if (!IsVisible)
		return;

    updateMesh();

    drawBatch->draw();

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
    if (scale != ScaleImage) Rebuild = true;
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
    if (sourceRect != SourceRect) Rebuild = true;
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
