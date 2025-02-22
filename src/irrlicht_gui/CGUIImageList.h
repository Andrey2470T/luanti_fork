// This file is part of the "Irrlicht Engine".
// written by Reinhard Ostermeier, reinhard@nospam.r-ostermeier.de

#pragma once

#include "IGUIImageList.h"
#include "IVideoDriver.h"

namespace irr
{
namespace gui
{

class CGUIImageList : public IGUIImageList
{
public:
	//! constructor
	CGUIImageList(video::IVideoDriver *Driver);

	//! destructor
	virtual ~CGUIImageList();

	//! Creates the image list from texture.
	//! \param texture: The texture to use
	//! \param imageSize: Size of a single image
	//! \param useAlphaChannel: true if the alpha channel from the texture should be used
	//! \return
	//! true if the image list was created
    bool createImageList(render::Texture2D *texture,
            v2i imageSize,
			bool useAlphaChannel);

	//! Draws an image and clips it to the specified rectangle if wanted
	//! \param index: Index of the image
	//! \param destPos: Position of the image to draw
	//! \param clip: Optional pointer to a rectangle against which the text will be clipped.
	//! If the pointer is null, no clipping will be done.
    virtual void draw(s32 index, const v2i &destPos,
            const recti *clip = 0) override;

	//! Returns the count of Images in the list.
	//! \return Returns the count of Images in the list.
	s32 getImageCount() const override
	{
		return ImageCount;
	}

	//! Returns the size of the images in the list.
	//! \return Returns the size of the images in the list.
    v2i getImageSize() const override
	{
		return ImageSize;
	}

private:
	video::IVideoDriver *Driver;
    render::Texture2D *Texture;
	s32 ImageCount;
    v2i ImageSize;
	s32 ImagesPerRow;
	bool UseAlphaChannel;
};

} // end namespace gui
} // end namespace irr
