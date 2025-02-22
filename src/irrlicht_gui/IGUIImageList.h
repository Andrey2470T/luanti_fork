// This file is part of the "Irrlicht Engine".
// written by Reinhard Ostermeier, reinhard@nospam.r-ostermeier.de

#pragma once


#include "Utils/Rect.h"


//! Font interface.
class IGUIImageList
{
public:
	//! Destructor
	virtual ~IGUIImageList(){};

	//! Draws an image and clips it to the specified rectangle if wanted
	//! \param index: Index of the image
	//! \param destPos: Position of the image to draw
	//! \param clip: Optional pointer to a rectangle against which the text will be clipped.
	//! If the pointer is null, no clipping will be done.
    virtual void draw(s32 index, const v2i &destPos,
            const recti *clip = 0) = 0;

	//! Returns the count of Images in the list.
	//! \return Returns the count of Images in the list.
	virtual s32 getImageCount() const = 0;

	//! Returns the size of the images in the list.
	//! \return Returns the size of the images in the list.
    virtual v2i getImageSize() const = 0;
};
