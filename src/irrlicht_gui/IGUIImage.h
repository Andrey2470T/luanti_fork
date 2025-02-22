// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUIElement.h"
#include "Image/Color.h"

namespace render
{
    class Texture2D;
}

//! GUI element displaying an image.
class IGUIImage : public IGUIElement
{
public:
	//! constructor
    IGUIImage(IGUIEnvironment *environment, std::shared_ptr<IGUIElement> parent, s32 id, recti rectangle) :
            IGUIElement(GUIElementType::Image, environment, parent, id, rectangle) {}

	//! Sets an image texture
    virtual void setImage(render::Texture2D *image) = 0;

	//! Gets the image texture
    virtual render::Texture2D *getImage() const = 0;

	//! Sets the color of the image
	/** \param color Color with which the image is drawn. If the color
	equals Color(255,255,255,255) it is ignored. */
    virtual void setColor(img::color8 color) = 0;

	//! Sets if the image should scale to fit the element
	virtual void setScaleImage(bool scale) = 0;

	//! Sets if the image should use its alpha channel to draw itself
	virtual void setUseAlphaChannel(bool use) = 0;

	//! Gets the color of the image
    virtual img::color8 getColor() const = 0;

	//! Returns true if the image is scaled to fit, false if not
	virtual bool isImageScaled() const = 0;

	//! Returns true if the image is using the alpha channel, false if not
	virtual bool isAlphaChannelUsed() const = 0;

	//! Sets the source rectangle of the image. By default the full image is used.
	/** \param sourceRect coordinates inside the image or an area with size 0 for using the full image (default). */
    virtual void setSourceRect(const recti &sourceRect) = 0;

	//! Returns the customized source rectangle of the image to be used.
	/** By default an empty rectangle of width and height 0 is returned which means the full image is used. */
    virtual recti getSourceRect() const = 0;

	//! Restrict drawing-area.
	/** This allows for example to use the image as a progress bar.
		Base for area is the image, which means:
		-  The original clipping area when the texture is scaled or there is no texture.
		-  The source-rect for an unscaled texture (but still restricted afterward by the clipping area)
		Unlike normal clipping this does not affect the gui-children.
		\param drawBoundUVs: Coordinates between 0 and 1 where 0 are for left+top and 1 for right+bottom
	*/
    virtual void setDrawBounds(const rectf &drawBoundUVs = rectf(0.f, 0.f, 1.f, 1.f)) = 0;

	//! Get drawing-area restrictions.
    virtual rectf getDrawBounds() const = 0;

	//! Sets whether to draw a background color (EGDC_3D_DARK_SHADOW) when no texture is set
	/** By default it's enabled */
	virtual void setDrawBackground(bool draw) = 0;

	//! Checks if a background is drawn when no texture is set
	/** \return true if background drawing is enabled, false otherwise */
	virtual bool isDrawBackgroundEnabled() const = 0;
};
