// This file is part of the "Irrlicht Engine".
// written by Reinhard Ostermeier, reinhard@nospam.r-ostermeier.de
// modified by Thomas Alten

#include "CGUIImageList.h"

namespace gui
{

//! constructor
CGUIImageList::CGUIImageList() :
		Texture(0),
		ImageCount(0),
		ImageSize(0, 0),
		ImagesPerRow(0),
		UseAlphaChannel(false)
{}

//! Creates the image list from texture.
bool CGUIImageList::createImageList(img::Image *texture,
		v2i imageSize,
		bool useAlphaChannel)
{
	if (!texture) {
		return false;
	}

	ImageSize = imageSize;

    ImagesPerRow = Texture->getSize().X / ImageSize.X;
    ImageCount = ImagesPerRow * Texture->getSize().Y / ImageSize.Y;

	UseAlphaChannel = useAlphaChannel;

	return true;
}

//! Draws an image and clips it to the specified rectangle if wanted
void CGUIImageList::draw(s32 index, const v2i &destPos,
		const recti *clip /*= 0*/)
{
	recti sourceRect;

    if (index < 0 || index >= ImageCount) {
		return;
	}

    sourceRect.ULC.X = (index % ImagesPerRow) * ImageSize.X;
    sourceRect.ULC.Y = (index / ImagesPerRow) * ImageSize.Y;
    sourceRect.LRC.X = sourceRect.ULC.X + ImageSize.X;
    sourceRect.LRC.Y = sourceRect.ULC.Y + ImageSize.Y;

	Driver->draw2DImage(Texture, destPos, sourceRect, clip,
            img::color8(img::white), UseAlphaChannel);
}

} // end namespace gui
