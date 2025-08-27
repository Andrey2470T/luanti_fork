// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUIElement.h"
#include <Image/Color.h>
#include <Image/Image.h>
#include <Render/TTFont.h>

namespace gui
{
class IGUISpriteBank;

//! GUI Button interface.
/** \par This element can create the following events of type EGUI_EVENT_TYPE:
\li EGET_BUTTON_CLICKED
*/
class IGUIButton : public IGUIElement
{
public:
	//! constructor
    IGUIButton(IGUIEnvironment *environment, IGUIElement *parent, s32 id, recti rectangle) :
			IGUIElement(EGUIET_BUTTON, environment, parent, id, rectangle) {}

	//! Sets another skin independent font.
	/** If this is set to zero, the button uses the font of the skin.
	\param font: New font to set. */
    virtual void setOverrideFont(render::TTFont *font = 0) = 0;

	//! Gets the override font (if any)
	/** \return The override font (may be 0) */
    virtual render::TTFont *getOverrideFont(void) const = 0;

	//! Get the font which is used right now for drawing
	/** Currently this is the override font when one is set and the
	font of the active skin otherwise */
    virtual render::TTFont *getActiveFont() const = 0;

	//! Sets another color for the button text.
	/** When set, this color is used instead of EGDC_BUTTON_TEXT/EGDC_GRAY_TEXT.
	You don't need to call enableOverrideColor(true), that's done by this function.
	If you want the the color of the skin back, call enableOverrideColor(false);
	\param color: New color of the text. */
    virtual void setOverrideColor(img::color8 color) = 0;

	//! Gets the override color
	/** \return: The override color */
    virtual img::color8 getOverrideColor(void) const = 0;

	//! Gets the currently used text color
	/** Either a skin-color for the current state or the override color */
    virtual img::color8 getActiveColor() const = 0;

	//! Sets if the button text should use the override color or the color in the gui skin.
	/** \param enable: If set to true, the override color, which can be set
	with IGUIStaticText::setOverrideColor is used, otherwise the
	EGDC_BUTTON_TEXT or EGDC_GRAY_TEXT color of the skin. */
	virtual void enableOverrideColor(bool enable) = 0;

	//! Checks if an override color is enabled
	/** \return true if the override color is enabled, false otherwise */
	virtual bool isOverrideColorEnabled(void) const = 0;

	//! Sets an image which should be displayed on the button when it is in the given state.
	/** Only one image-state can be active at a time. Images are drawn below sprites.
	If a state is without image it will try to use images from other states as described
	in ::EGUI_BUTTON_IMAGE_STATE.
	Images are a little less flexible than sprites, but easier to use.
	\param state: One of ::EGUI_BUTTON_IMAGE_STATE
	\param image: Image to be displayed or NULL to remove the image
	\param sourceRect: Source rectangle on the image texture. When width or height are 0 then the full texture-size is used (default). */
    virtual void setImage(EGUI_BUTTON_IMAGE_STATE state, img::Image *image = 0, const recti &sourceRect = recti(0, 0, 0, 0)) = 0;

	//! Sets an image which should be displayed on the button when it is in normal state.
	/** This is identical to calling setImage(EGBIS_IMAGE_UP, image); and might be deprecated in future revisions.
	\param image: Image to be displayed */
    virtual void setImage(img::Image *image = 0) = 0;

	//! Sets a background image for the button when it is in normal state.
	/** This is identical to calling setImage(EGBIS_IMAGE_UP, image, sourceRect); and might be deprecated in future revisions.
	\param image: Texture containing the image to be displayed
	\param sourceRect: Position in the texture, where the image is located.
	When width or height are 0 then the full texture-size is used */
    virtual void setImage(img::Image *image, const recti &sourceRect) = 0;

	//! Sets a background image for the button when it is in pressed state.
	/** This is identical to calling setImage(EGBIS_IMAGE_DOWN, image); and might be deprecated in future revisions.
	If no images is specified for the pressed state via
	setPressedImage(), this image is also drawn in pressed state.
	\param image: Image to be displayed */
    virtual void setPressedImage(img::Image *image = 0) = 0;

	//! Sets an image which should be displayed on the button when it is in pressed state.
	/** This is identical to calling setImage(EGBIS_IMAGE_DOWN, image, sourceRect); and might be deprecated in future revisions.
	\param image: Texture containing the image to be displayed
	\param sourceRect: Position in the texture, where the image is located */
    virtual void setPressedImage(img::Image *image, const recti &sourceRect) = 0;

	//! Sets the sprite bank used by the button
	/** NOTE: The spritebank itself is _not_ serialized so far. The sprites are serialized.
	Which means after loading the gui you still have to set the spritebank manually. */
	virtual void setSpriteBank(IGUISpriteBank *bank = 0) = 0;

	//! Sets the animated sprite for a specific button state
	/** Several sprites can be drawn at the same time.
	Sprites can be animated.
	Sprites are drawn above the images.
	\param index: Number of the sprite within the sprite bank, use -1 for no sprite
	\param state: State of the button to set the sprite for
	\param index: The sprite number from the current sprite bank
	\param color: The color of the sprite
	\param loop: True if the animation should loop, false if not
	\param scale: True if the sprite should scale to button size, false if not	*/
	virtual void setSprite(EGUI_BUTTON_STATE state, s32 index,
            img::color8 color = img::white, bool loop = false) = 0;

	//! Get the sprite-index for the given state or -1 when no sprite is set
	virtual s32 getSpriteIndex(EGUI_BUTTON_STATE state) const = 0;

	//! Get the sprite color for the given state. Color is only used when a sprite is set.
    virtual img::color8 getSpriteColor(EGUI_BUTTON_STATE state) const = 0;

	//! Returns if the sprite in the given state does loop
	virtual bool getSpriteLoop(EGUI_BUTTON_STATE state) const = 0;

	//! Sets if the button should behave like a push button.
	/** Which means it can be in two states: Normal or Pressed. With a click on the button,
	the user can change the state of the button. */
	virtual void setIsPushButton(bool isPushButton = true) = 0;

	//! Sets the pressed state of the button if this is a pushbutton
	virtual void setPressed(bool pressed = true) = 0;

	//! Returns if the button is currently pressed
	virtual bool isPressed() const = 0;

	//! Sets if the alpha channel should be used for drawing background images on the button (default is false)
	virtual void setUseAlphaChannel(bool useAlphaChannel = true) = 0;

	//! Returns if the alpha channel should be used for drawing background images on the button
	virtual bool isAlphaChannelUsed() const = 0;

	//! Returns whether the button is a push button
	virtual bool isPushButton() const = 0;

	//! Sets if the button should use the skin to draw its border and button face (default is true)
	virtual void setDrawBorder(bool border = true) = 0;

	//! Returns if the border and button face are being drawn using the skin
	virtual bool isDrawingBorder() const = 0;

	//! Sets if the button should scale the button images to fit
	virtual void setScaleImage(bool scaleImage = true) = 0;

	//! Checks whether the button scales the used images
	virtual bool isScalingImage() const = 0;

	//! Get if the shift key was pressed in last EGET_BUTTON_CLICKED event
	/** Generated together with event, so info is available in the event-receiver.	*/
	virtual bool getClickShiftState() const = 0;

	//! Get if the control key was pressed in last EGET_BUTTON_CLICKED event
	/** Generated together with event, so info is available in the event-receiver.	*/
	virtual bool getClickControlState() const = 0;
};

} // end namespace gui
