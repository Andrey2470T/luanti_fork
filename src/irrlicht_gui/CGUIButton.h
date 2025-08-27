// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUIButton.h"
#include "IGUISpriteBank.h"
#include <Image/Image.h>

namespace gui
{

class CGUIButton : public IGUIButton
{
public:
	//! constructor
	CGUIButton(IGUIEnvironment *environment, IGUIElement *parent,
            s32 id, recti rectangle, bool noclip = false);

	//! destructor
	virtual ~CGUIButton();

	//! called if an event happened.
    bool OnEvent(const main::Event &event) override;

	//! draws the element and its children
	void draw() override;

	//! sets another skin independent font. if this is set to zero, the button uses the font of the skin.
    void setOverrideFont(render::TTFont *font = 0) override;

	//! Gets the override font (if any)
    render::TTFont *getOverrideFont() const override;

	//! Get the font which is used right now for drawing
    render::TTFont *getActiveFont() const override;

	//! Sets another color for the button text.
    void setOverrideColor(img::color8 color) override;

	//! Gets the override color
    img::color8 getOverrideColor(void) const override;

	//! Gets the currently used text color
    img::color8 getActiveColor() const override;

	//! Sets if the button text should use the override color or the color in the gui skin.
	void enableOverrideColor(bool enable) override;

	//! Checks if an override color is enabled
	bool isOverrideColorEnabled(void) const override;

	//! Sets an image which should be displayed on the button when it is in the given state.
    void setImage(EGUI_BUTTON_IMAGE_STATE state, img::Image *image = 0, const recti &sourceRect = recti(0, 0, 0, 0)) override;

	//! Sets an image which should be displayed on the button when it is in normal state.
    void setImage(img::Image *image = 0) override
	{
		setImage(EGBIS_IMAGE_UP, image);
	}

	//! Sets an image which should be displayed on the button when it is in normal state.
    void setImage(img::Image *image, const recti &pos) override
	{
		setImage(EGBIS_IMAGE_UP, image, pos);
	}

	//! Sets an image which should be displayed on the button when it is in pressed state.
    void setPressedImage(img::Image *image = 0) override
	{
		setImage(EGBIS_IMAGE_DOWN, image);
	}

	//! Sets an image which should be displayed on the button when it is in pressed state.
    void setPressedImage(img::Image *image, const recti &pos) override
	{
		setImage(EGBIS_IMAGE_DOWN, image, pos);
	}

	//! Sets the sprite bank used by the button
	void setSpriteBank(IGUISpriteBank *bank = 0) override;

	//! Sets the animated sprite for a specific button state
	/** \param index: Number of the sprite within the sprite bank, use -1 for no sprite
	\param state: State of the button to set the sprite for
	\param index: The sprite number from the current sprite bank
	\param color: The color of the sprite
	*/
	virtual void setSprite(EGUI_BUTTON_STATE state, s32 index,
            img::color8 color = img::white,
			bool loop = false) override;

	//! Get the sprite-index for the given state or -1 when no sprite is set
	s32 getSpriteIndex(EGUI_BUTTON_STATE state) const override;

	//! Get the sprite color for the given state. Color is only used when a sprite is set.
    img::color8 getSpriteColor(EGUI_BUTTON_STATE state) const override;

	//! Returns if the sprite in the given state does loop
	bool getSpriteLoop(EGUI_BUTTON_STATE state) const override;

	//! Sets if the button should behave like a push button. Which means it
	//! can be in two states: Normal or Pressed. With a click on the button,
	//! the user can change the state of the button.
	void setIsPushButton(bool isPushButton = true) override;

	//! Checks whether the button is a push button
	bool isPushButton() const override;

	//! Sets the pressed state of the button if this is a pushbutton
	void setPressed(bool pressed = true) override;

	//! Returns if the button is currently pressed
	bool isPressed() const override;

	//! Sets if the button should use the skin to draw its border
	void setDrawBorder(bool border = true) override;

	//! Checks if the button face and border are being drawn
	bool isDrawingBorder() const override;

	//! Sets if the alpha channel should be used for drawing images on the button (default is false)
	void setUseAlphaChannel(bool useAlphaChannel = true) override;

	//! Checks if the alpha channel should be used for drawing images on the button
	bool isAlphaChannelUsed() const override;

	//! Sets if the button should scale the button images to fit
	void setScaleImage(bool scaleImage = true) override;

	//! Checks whether the button scales the used images
	bool isScalingImage() const override;

	//! Get if the shift key was pressed in last EGET_BUTTON_CLICKED event
	bool getClickShiftState() const override
	{
		return ClickShiftState;
	}

	//! Get if the control key was pressed in last EGET_BUTTON_CLICKED event
	bool getClickControlState() const override
	{
		return ClickControlState;
	}

protected:
    void drawSprite(EGUI_BUTTON_STATE state, u32 startTime, const v2i &center);
	EGUI_BUTTON_IMAGE_STATE getImageState(bool pressed) const;

private:
	struct ButtonSprite
	{
		ButtonSprite() :
				Index(-1), Loop(false)
		{
		}

		bool operator==(const ButtonSprite &other) const
		{
			return Index == other.Index && Color == other.Color && Loop == other.Loop;
		}

		s32 Index;
        img::color8 Color;
		bool Loop;
	};

    ButtonSprite ButtonSprites[(u8)EGBS_COUNT];
	IGUISpriteBank *SpriteBank;

	struct ButtonImage
	{
		ButtonImage() :
                Texture(0), SourceRect(recti(0, 0, 0, 0))
		{
		}

		ButtonImage(const ButtonImage &other) :
                Texture(0), SourceRect(recti(0, 0, 0, 0))
		{
			*this = other;
		}

		ButtonImage &operator=(const ButtonImage &other)
		{
			if (this == &other)
				return *this;

			Texture = other.Texture;
			SourceRect = other.SourceRect;
			return *this;
		}

		bool operator==(const ButtonImage &other) const
		{
			return Texture == other.Texture && SourceRect == other.SourceRect;
		}

        img::Image *Texture;
        recti SourceRect;
	};

    ButtonImage ButtonImages[(u8)EGBIS_COUNT];

    render::TTFont *OverrideFont;

	bool OverrideColorEnabled;
    img::color8 OverrideColor;

	u32 ClickTime, HoverTime, FocusTime;

	bool ClickShiftState;
	bool ClickControlState;

	bool IsPushButton;
	bool Pressed;
	bool UseAlphaChannel;
	bool DrawBorder;
	bool ScaleImage;
};

} // end namespace gui
