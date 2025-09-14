// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUIStaticText.h"
#include "IGUIButton.h"
#include "IGUISpriteBank.h"
#include "StyleSpec.h"


class GUIButton : public gui::IGUIButton
{
public:
	//! constructor
	GUIButton(gui::IGUIEnvironment* environment, gui::IGUIElement* parent,
               s32 id, recti rectangle, bool noclip=false);

	//! destructor
	virtual ~GUIButton();

	//! called if an event happened.
    virtual bool OnEvent(const core::Event& event) override;

	//! draws the element and its children
	virtual void draw() override;

	//! sets another skin independent font. if this is set to zero, the button uses the font of the skin.
    virtual void setOverrideFont(render::TTFont* font=0) override;

	//! Gets the override font (if any)
    virtual render::TTFont* getOverrideFont() const override;

	//! Get the font which is used right now for drawing
    virtual render::TTFont* getActiveFont() const override;

	//! Sets another color for the button text.
	virtual void setOverrideColor(img::color8 color) override;

	//! Gets the override color
	virtual img::color8 getOverrideColor() const override;

	//! Gets the currently used text color
	virtual img::color8 getActiveColor() const override;

	//! Sets if the button text should use the override color or the color in the gui skin.
	virtual void enableOverrideColor(bool enable) override;

	//! Checks if an override color is enabled
	virtual bool isOverrideColorEnabled(void) const override;

	// PATCH
	//! Sets an image which should be displayed on the button when it is in the given state.
    virtual void setImage(EGUI_BUTTON_IMAGE_STATE state,
			img::Image* image=nullptr,
			const recti& sourceRect=recti(0,0,0,0)) override;

	//! Sets an image which should be displayed on the button when it is in normal state.
	virtual void setImage(img::Image* image=nullptr) override;

	//! Sets an image which should be displayed on the button when it is in normal state.
	virtual void setImage(img::Image* image, const recti& pos) override;

	//! Sets an image which should be displayed on the button when it is in pressed state.
	virtual void setPressedImage(img::Image* image=nullptr) override;

	//! Sets an image which should be displayed on the button when it is in pressed state.
	virtual void setPressedImage(img::Image* image, const recti& pos) override;

	//! Sets the text displayed by the button
	virtual void setText(const wchar_t* text) override;
	// END PATCH

	//! Sets the sprite bank used by the button
	virtual void setSpriteBank(gui::IGUISpriteBank* bank=0) override;

	//! Sets the animated sprite for a specific button state
	/** \param index: Number of the sprite within the sprite bank, use -1 for no sprite
	\param state: State of the button to set the sprite for
	\param index: The sprite number from the current sprite bank
	\param color: The color of the sprite
	*/
    virtual void setSprite(EGUI_BUTTON_STATE state, s32 index,
                           img::color8 color=img::white,
						   bool loop=false) override;

	//! Get the sprite-index for the given state or -1 when no sprite is set
    virtual s32 getSpriteIndex(EGUI_BUTTON_STATE state) const override;

	//! Get the sprite color for the given state. Color is only used when a sprite is set.
    virtual img::color8 getSpriteColor(EGUI_BUTTON_STATE state) const override;

	//! Returns if the sprite in the given state does loop
    virtual bool getSpriteLoop(EGUI_BUTTON_STATE state) const override;

	//! Sets if the button should behave like a push button. Which means it
	//! can be in two states: Normal or Pressed. With a click on the button,
	//! the user can change the state of the button.
	virtual void setIsPushButton(bool isPushButton=true) override;

	//! Checks whether the button is a push button
	virtual bool isPushButton() const override;

	//! Sets the pressed state of the button if this is a pushbutton
	virtual void setPressed(bool pressed=true) override;

	//! Returns if the button is currently pressed
	virtual bool isPressed() const override;

	// PATCH
	//! Returns if this element (or one of its direct children) is hovered
	bool isHovered() const;

	//! Returns if this element (or one of its direct children) is focused
	bool isFocused() const;
	// END PATCH

	//! Sets if the button should use the skin to draw its border
	virtual void setDrawBorder(bool border=true) override;

	//! Checks if the button face and border are being drawn
	virtual bool isDrawingBorder() const override;

	//! Sets if the alpha channel should be used for drawing images on the button (default is false)
	virtual void setUseAlphaChannel(bool useAlphaChannel=true) override;

	//! Checks if the alpha channel should be used for drawing images on the button
	virtual bool isAlphaChannelUsed() const override;

	//! Sets if the button should scale the button images to fit
	virtual void setScaleImage(bool scaleImage=true) override;

	//! Checks whether the button scales the used images
	virtual bool isScalingImage() const override;

	//! Get if the shift key was pressed in last EGET_BUTTON_CLICKED event
	virtual bool getClickShiftState() const override
	{
		return ClickShiftState;
	}

	//! Get if the control key was pressed in last EGET_BUTTON_CLICKED event
	virtual bool getClickControlState() const override
	{
		return ClickControlState;
	}

	void setColor(img::color8 color);
	// PATCH
	//! Set element properties from a StyleSpec corresponding to the button state
	void setFromState();

	//! Set element properties from a StyleSpec
	virtual void setFromStyle(const StyleSpec& style);

	//! Set the styles used for each state
	void setStyles(const std::array<StyleSpec, StyleSpec::NUM_STATES>& styles);
	// END PATCH


	//! Do not drop returned handle
	static GUIButton* addButton(gui::IGUIEnvironment *environment,
            const recti& rectangle,
			IGUIElement* parent, s32 id, const wchar_t* text,
			const wchar_t *tooltiptext=L"");

protected:
    void drawSprite(EGUI_BUTTON_STATE state, u32 startTime, const v2i& center);
    EGUI_BUTTON_IMAGE_STATE getImageState(bool pressed) const;

	struct ButtonImage
	{
		ButtonImage() = default;

		ButtonImage(const ButtonImage& other)
		{
			*this = other;
		}

		ButtonImage& operator=(const ButtonImage& other)
		{
			if ( this == &other )
				return *this;

			Texture = other.Texture;
			SourceRect = other.SourceRect;
			return *this;
		}

		bool operator==(const ButtonImage& other) const
		{
			return Texture == other.Texture && SourceRect == other.SourceRect;
		}


		img::Image* Texture = nullptr;
		recti SourceRect = recti(0,0,0,0);
	};

    EGUI_BUTTON_IMAGE_STATE getImageState(bool pressed, const ButtonImage* images) const;

private:

	struct ButtonSprite
	{
		bool operator==(const ButtonSprite &other) const
		{
			return Index == other.Index && Color == other.Color && Loop == other.Loop;
		}

		s32 Index = -1;
		img::color8 Color;
		bool Loop = false;
	};

    ButtonSprite ButtonSprites[(u8)EGBS_COUNT];
	gui::IGUISpriteBank* SpriteBank = nullptr;

    ButtonImage ButtonImages[(u8)EGBIS_COUNT];

	std::array<StyleSpec, StyleSpec::NUM_STATES> Styles;

    render::TTFont* OverrideFont = nullptr;

	bool OverrideColorEnabled = false;
    img::color8 OverrideColor = img::color8(img::PF_RGBA8, 255,255,255,101);

	u32 ClickTime = 0;
	u32 HoverTime = 0;
	u32 FocusTime = 0;

	bool ClickShiftState = false;
	bool ClickControlState = false;

	bool IsPushButton = false;
	bool Pressed = false;
	bool UseAlphaChannel = false;
	bool DrawBorder = true;
	bool ScaleImage = false;

	img::color8 Colors[4];
	// PATCH
	bool WasHovered = false;
	bool WasFocused = false;

	gui::IGUIStaticText *StaticText;
    std::unique_ptr<UISpriteBank> ButtonBox;

	recti BgMiddle;
	recti Padding;
    v2i ContentOffset;
    img::color8 BgColor = img::white;
	// END PATCH
};
