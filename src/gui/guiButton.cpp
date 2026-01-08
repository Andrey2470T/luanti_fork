// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiButton.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include "client/render/rendersystem.h"
#include <Render/TTFont.h>
#include "client/ui/extra_images.h"
#include "porting.h"
#include "StyleSpec.h"
#include "util/numeric.h"

// Multiply with a color to get the default corresponding hovered color
#define COLOR_HOVERED_MOD 1.25f

// Multiply with a color to get the default corresponding pressed color
#define COLOR_PRESSED_MOD 0.85f

using namespace gui;

//! constructor
GUIButton::GUIButton(IGUIEnvironment* environment, IGUIElement* parent,
        s32 id, recti rectangle, bool noclip) :
    IGUIButton(environment, parent, id, rectangle),
    drawBatch(std::make_unique<SpriteDrawBatch>(environment->getRenderSystem(), environment->getResourceCache()))
{
	setNotClipped(noclip);

	// This element can be tabbed.
	setTabStop(true);
	setTabOrder(-1);

	// PATCH
	for (size_t i = 0; i < 4; i++) {
		Colors[i] = Environment->getSkin()->getColor((EGUI_DEFAULT_COLOR)i);
	}
    StaticText = Environment->addStaticText(Text.c_str(), recti(0,0,rectangle.getWidth(),rectangle.getHeight()), false, false, this, id);
	StaticText->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
	// END PATCH

    ButtonBox = drawBatch->addRectsSprite({{}});
    if (BgMiddle.getArea() == 0)
        ImageBox = drawBatch->addRectsSprite({{}});
    else
        ImageBox = drawBatch->addImage2D9Slice(rectf(), rectf(), rectf(), nullptr);
}

//! destructor
GUIButton::~GUIButton()
{
	if (SpriteBank)
		SpriteBank->drop();
}


//! Sets if the images should be scaled to fit the button
void GUIButton::setScaleImage(bool scaleImage)
{
    if (scaleImage != ScaleImage) Rebuild = true;
	ScaleImage = scaleImage;
}


//! Returns whether the button scale the used images
bool GUIButton::isScalingImage() const
{
	return ScaleImage;
}


//! Sets if the button should use the skin to draw its border
void GUIButton::setDrawBorder(bool border)
{
    if (border != DrawBorder) Rebuild = true;
	DrawBorder = border;
}


void GUIButton::setSpriteBank(IGUISpriteBank* sprites)
{
	if (sprites)
		sprites->grab();

	if (SpriteBank)
		SpriteBank->drop();

	SpriteBank = sprites;
	Rebuild = true;
}

void GUIButton::setSprite(EGUI_BUTTON_STATE state, s32 index, img::color8 color, bool loop)
{
	ButtonSprites[(u32)state].Index	= index;
	ButtonSprites[(u32)state].Color	= color;
	ButtonSprites[(u32)state].Loop	= loop;
	Rebuild = true;
}

//! Get the sprite-index for the given state or -1 when no sprite is set
s32 GUIButton::getSpriteIndex(EGUI_BUTTON_STATE state) const
{
	return ButtonSprites[(u32)state].Index;
}

//! Get the sprite color for the given state. Color is only used when a sprite is set.
img::color8 GUIButton::getSpriteColor(EGUI_BUTTON_STATE state) const
{
	return ButtonSprites[(u32)state].Color;
}

//! Returns if the sprite in the given state does loop
bool GUIButton::getSpriteLoop(EGUI_BUTTON_STATE state) const
{
	return ButtonSprites[(u32)state].Loop;
}

//! called if an event happened.
bool GUIButton::OnEvent(const core::Event& event)
{
	if (!isEnabled())
		return IGUIElement::OnEvent(event);

    switch(event.Type)
	{
	case EET_KEY_INPUT_EVENT:
		if (event.KeyInput.PressedDown &&
			(event.KeyInput.Key == core::KEY_RETURN || event.KeyInput.Key == core::KEY_SPACE))
		{
			if (!IsPushButton)
				setPressed(true);
			else
				setPressed(!Pressed);

			return true;
		}
		if (Pressed && !IsPushButton && event.KeyInput.PressedDown && event.KeyInput.Key == core::KEY_ESCAPE)
		{
			setPressed(false);
			return true;
		}
		else
		if (!event.KeyInput.PressedDown && Pressed &&
			(event.KeyInput.Key == core::KEY_RETURN || event.KeyInput.Key == core::KEY_SPACE))
		{

			if (!IsPushButton)
				setPressed(false);

			if (Parent)
			{
				ClickShiftState = event.KeyInput.Shift;
				ClickControlState = event.KeyInput.Control;

                core::Event newEvent;
                newEvent.Type = EET_GUI_EVENT;
                newEvent.GUI.Caller = this;
                newEvent.GUI.Element = 0;
                newEvent.GUI.Type = EGET_BUTTON_CLICKED;
				Parent->OnEvent(newEvent);
			}
			return true;
		}
		break;
	case EET_GUI_EVENT:
        if (event.GUI.Caller == this)
		{
            if (event.GUI.Type == EGET_ELEMENT_FOCUS_LOST)
			{
				if (!IsPushButton)
					setPressed(false);
				FocusTime = (u32)porting::getTimeMs();
			}
            else if (event.GUI.Type == EGET_ELEMENT_FOCUSED)
			{
				FocusTime = (u32)porting::getTimeMs();
			}
            else if (event.GUI.Type == EGET_ELEMENT_HOVERED || event.GUI.Type == EGET_ELEMENT_LEFT)
			{
				HoverTime = (u32)porting::getTimeMs();
			}
		}
		break;
	case EET_MOUSE_INPUT_EVENT:
        if (event.MouseInput.Type == EMIE_LMOUSE_PRESSED_DOWN)
		{
			// Sometimes formspec elements can receive mouse events when the
			// mouse is outside of the formspec. Thus, we test the position here.
			if ( !IsPushButton && AbsoluteClippingRect.isPointInside(
						v2i(event.MouseInput.X, event.MouseInput.Y ))) {
				Environment->setFocus(this);
				setPressed(true);
			}

			return true;
		}
		else
        if (event.MouseInput.Type == EMIE_LMOUSE_LEFT_UP)
		{
			bool wasPressed = Pressed;

			if ( !AbsoluteClippingRect.isPointInside( v2i(event.MouseInput.X, event.MouseInput.Y ) ) )
			{
				if (!IsPushButton)
					setPressed(false);
				return true;
			}

			if (!IsPushButton)
				setPressed(false);
			else
			{
				setPressed(!Pressed);
			}

			if ((!IsPushButton && wasPressed && Parent) ||
				(IsPushButton && wasPressed != Pressed))
			{
				ClickShiftState = event.MouseInput.Shift;
				ClickControlState = event.MouseInput.Control;

                core::Event newEvent;
                newEvent.Type = EET_GUI_EVENT;
                newEvent.GUI.Caller = this;
                newEvent.GUI.Element = 0;
                newEvent.GUI.Type = EGET_BUTTON_CLICKED;
				Parent->OnEvent(newEvent);
			}

			return true;
		}
		break;
	default:
		break;
	}

	return Parent ? Parent->OnEvent(event) : false;
}

void GUIButton::updateMesh()
{
    // PATCH
    // Track hovered state, if it has changed then we need to update the style.
    bool hovered = isHovered();
    bool focused = isFocused();
    if (hovered != WasHovered || focused != WasFocused) {
        WasHovered = hovered;
        WasFocused = focused;
        setFromState();
    }

    if (!Rebuild)
        return;
    GUISkin *skin = Environment->getSkin();
	// END PATCH

	if (DrawBorder)
	{
        ButtonBox->clear();
		if (!Pressed)
		{
			// PATCH
            skin->addColored3DButtonPaneStandard(ButtonBox, toRectT<f32>(AbsoluteRect), Colors);
			// END PATCH
		}
		else
		{
			// PATCH
            skin->addColored3DButtonPanePressed(ButtonBox, toRectT<f32>(AbsoluteRect), Colors);
			// END PATCH
		}
        ButtonBox->setClipRect(AbsoluteClippingRect);
        ButtonBox->setVisible(true);
    }

    const v2i buttonCenter(AbsoluteRect.getCenter());
	// PATCH
	// The image changes based on the state, so we use the default every time.
    EGUI_BUTTON_IMAGE_STATE imageState = EGBIS_IMAGE_UP;
	// END PATCH
	if ( ButtonImages[(u32)imageState].Texture )
	{
		v2i pos(buttonCenter);
		recti sourceRect(ButtonImages[(u32)imageState].SourceRect);
		if ( sourceRect.getWidth() == 0 && sourceRect.getHeight() == 0 )
            sourceRect = recti(v2i(0,0), toV2T<s32>(ButtonImages[(u32)imageState].Texture->getSize()));

		pos.X -= sourceRect.getWidth() / 2;
		pos.Y -= sourceRect.getHeight() / 2;

		if ( Pressed )
		{
			// Create a pressed-down effect by moving the image when it looks identical to the unpressed state image
			EGUI_BUTTON_IMAGE_STATE unpressedState = getImageState(false);
			if ( unpressedState == imageState || ButtonImages[(u32)imageState] == ButtonImages[(u32)unpressedState] )
			{
				pos.X += skin->getSize(EGDS_BUTTON_PRESSED_IMAGE_OFFSET_X);
				pos.Y += skin->getSize(EGDS_BUTTON_PRESSED_IMAGE_OFFSET_Y);
			}
		}

		// PATCH
		img::Image* texture = ButtonImages[(u32)imageState].Texture;
        std::array<img::color8, 4> image_colors = { BgColor, BgColor, BgColor, BgColor };
		if (BgMiddle.getArea() == 0) {
            auto img = dynamic_cast<UIRects *>(ImageBox);
            img->updateRect(0, {toRectT<f32>(ScaleImage? AbsoluteRect : recti(pos, sourceRect.getSize())),
                image_colors, texture});
            img->setClipRect(AbsoluteClippingRect);

		} else {
            auto sliced_img = dynamic_cast<Image2D9Slice *>(ImageBox);
            sliced_img->updateRects(toRectT<f32>(sourceRect), toRectT<f32>(BgMiddle),
                toRectT<f32>(ScaleImage ? AbsoluteRect : recti(pos, sourceRect.getSize())),
                texture, image_colors, &AbsoluteClippingRect);
		}
        ImageBox->setVisible(true);
		// END PATCH
	}

	if (SpriteBank)
	{
		if (isEnabled())
		{
            v2i pos(buttonCenter);
			// pressed / unpressed animation
			EGUI_BUTTON_STATE state = Pressed ? EGBS_BUTTON_DOWN : EGBS_BUTTON_UP;
            updateSprite(state, ClickTime, pos);

			// focused / unfocused animation
			state = Environment->hasFocus(this) ? EGBS_BUTTON_FOCUSED : EGBS_BUTTON_NOT_FOCUSED;
            updateSprite(state, FocusTime, pos);

			// mouse over / off animation
			state = isHovered() ? EGBS_BUTTON_MOUSE_OVER : EGBS_BUTTON_MOUSE_OFF;
            updateSprite(state, HoverTime, pos);
		}
		else
		{
			// draw disabled
            //drawSprite(EGBS_BUTTON_DISABLED, 0, pos);
		}
	}

    drawBatch->rebuild();

    Rebuild = false;
}

//! draws the element and its children
void GUIButton::draw()
{
    if (!IsVisible)
        return;

    updateMesh();

    drawBatch->draw();

    if (SpriteBank && isEnabled())
        SpriteBank->draw2DSprite();

    IGUIElement::draw();
}

void GUIButton::updateSprite(EGUI_BUTTON_STATE state, u32 startTime, const v2i& center)
{
	u32 stateIdx = (u32)state;
	s32 spriteIdx = ButtonSprites[stateIdx].Index;
	if (spriteIdx == -1)
		return;

	u32 rectIdx = SpriteBank->getSprites()[spriteIdx].Frames[0].rectNumber;
	recti srcRect = SpriteBank->getPositions()[rectIdx];

    GUISkin *skin = Environment->getSkin();
	s32 scale = std::max(std::floor(skin->getScale()), 1.0f);
	recti rect(center, center + srcRect.getSize() * scale);
	rect -= rect.getSize() / 2;

	const img::color8 colors[] = {
		ButtonSprites[stateIdx].Color,
		ButtonSprites[stateIdx].Color,
		ButtonSprites[stateIdx].Color,
		ButtonSprites[stateIdx].Color,
	};
    SpriteBank->update2DSprite(spriteIdx, rect, &AbsoluteClippingRect, colors,
			porting::getTimeMs() - startTime, ButtonSprites[stateIdx].Loop);
}

EGUI_BUTTON_IMAGE_STATE GUIButton::getImageState(bool pressed) const
{
	// PATCH
	return getImageState(pressed, ButtonImages);
	// END PATCH
}

EGUI_BUTTON_IMAGE_STATE GUIButton::getImageState(bool pressed, const ButtonImage* images) const
{
	// figure state we should have
	EGUI_BUTTON_IMAGE_STATE state = EGBIS_IMAGE_DISABLED;
	bool focused = isFocused();
	bool mouseOver = isHovered();
	if (isEnabled())
	{
		if ( pressed )
		{
			if ( focused && mouseOver )
				state = EGBIS_IMAGE_DOWN_FOCUSED_MOUSEOVER;
			else if ( focused )
				state = EGBIS_IMAGE_DOWN_FOCUSED;
			else if ( mouseOver )
				state = EGBIS_IMAGE_DOWN_MOUSEOVER;
			else
				state = EGBIS_IMAGE_DOWN;
		}
		else // !pressed
		{
			if ( focused && mouseOver )
                state = EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER;
			else if ( focused )
                state = EGBIS_IMAGE_UP_FOCUSED;
			else if ( mouseOver )
                state = EGBIS_IMAGE_UP_MOUSEOVER;
			else
                state = EGBIS_IMAGE_UP;
		}
	}

	// find a compatible state that has images
    while ( state != EGBIS_IMAGE_UP && !images[(u32)state].Texture )
	{
		// PATCH
		switch ( state )
		{
            case EGBIS_IMAGE_UP_FOCUSED:
                state = EGBIS_IMAGE_UP;
				break;
            case EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER:
                state = EGBIS_IMAGE_UP_FOCUSED;
				break;
			case EGBIS_IMAGE_DOWN_MOUSEOVER:
				state = EGBIS_IMAGE_DOWN;
				break;
			case EGBIS_IMAGE_DOWN_FOCUSED:
				state = EGBIS_IMAGE_DOWN;
				break;
			case EGBIS_IMAGE_DOWN_FOCUSED_MOUSEOVER:
				state = EGBIS_IMAGE_DOWN_FOCUSED;
				break;
			case EGBIS_IMAGE_DISABLED:
				if ( pressed )
					state = EGBIS_IMAGE_DOWN;
				else
                    state = EGBIS_IMAGE_UP;
				break;
			default:
                state = EGBIS_IMAGE_UP;
		}
		// END PATCH
	}

	return state;
}

//! sets another skin independent font. if this is set to zero, the button uses the font of the skin.
void GUIButton::setOverrideFont(render::TTFont* font)
{
	if (OverrideFont == font)
		return;

	OverrideFont = font;

	StaticText->setOverrideFont(font);
	Rebuild = true;
}

//! Gets the override font (if any)
render::TTFont * GUIButton::getOverrideFont() const
{
	return OverrideFont;
}

//! Get the font which is used right now for drawing
render::TTFont* GUIButton::getActiveFont() const
{
	if ( OverrideFont )
		return OverrideFont;
    GUISkin* skin = Environment->getSkin();
	if (skin)
		return skin->getFont(EGDF_BUTTON);
	return 0;
}

//! Sets another color for the text.
void GUIButton::setOverrideColor(img::color8 color)
{
	OverrideColor = color;
	OverrideColorEnabled = true;

	StaticText->setOverrideColor(color);
	Rebuild = true;
}

img::color8 GUIButton::getOverrideColor() const
{
	return OverrideColor;
}

img::color8 GUIButton::getActiveColor() const
{
    return img::black; // unused?
}

void GUIButton::enableOverrideColor(bool enable)
{
	OverrideColorEnabled = enable;
}

bool GUIButton::isOverrideColorEnabled() const
{
	return OverrideColorEnabled;
}

void GUIButton::setImage(EGUI_BUTTON_IMAGE_STATE state, img::Image* image, const recti& sourceRect)
{
	if ( state >= EGBIS_COUNT )
		return;

	u32 stateIdx = (u32)state;

	ButtonImages[stateIdx].Texture = image;
	ButtonImages[stateIdx].SourceRect = sourceRect;
	Rebuild = true;
}

// PATCH
void GUIButton::setImage(img::Image* image)
{
    setImage(EGBIS_IMAGE_UP, image);
}

void GUIButton::setImage(img::Image* image, const recti& pos)
{
    setImage(EGBIS_IMAGE_UP, image, pos);
}

void GUIButton::setPressedImage(img::Image* image)
{
    setImage(EGBIS_IMAGE_DOWN, image);
}

void GUIButton::setPressedImage(img::Image* image, const recti& pos)
{
    setImage(EGBIS_IMAGE_DOWN, image, pos);
}

//! Sets the text displayed by the button
void GUIButton::setText(const wchar_t* text)
{
	StaticText->setText(text);

	IGUIButton::setText(text);
}
// END PATCH

//! Sets if the button should behave like a push button. Which means it
//! can be in two states: Normal or Pressed. With a click on the button,
//! the user can change the state of the button.
void GUIButton::setIsPushButton(bool isPushButton)
{
	IsPushButton = isPushButton;
}


//! Returns if the button is currently pressed
bool GUIButton::isPressed() const
{
	return Pressed;
}

// PATCH
//! Returns if this element (or one of its direct children) is hovered
bool GUIButton::isHovered() const
{
	IGUIElement *hovered = Environment->getHovered();
	return  hovered == this || (hovered != nullptr && hovered->getParent() == this);
}

//! Returns if this element (or one of its direct children) is focused
bool GUIButton::isFocused() const
{
	return Environment->hasFocus((IGUIElement*)this, true);
}
// END PATCH

//! Sets the pressed state of the button if this is a pushbutton
void GUIButton::setPressed(bool pressed)
{
	if (Pressed != pressed)
	{
		ClickTime = porting::getTimeMs();
		Pressed = pressed;
		setFromState();
	}
}


//! Returns whether the button is a push button
bool GUIButton::isPushButton() const
{
	return IsPushButton;
}


//! Sets if the alpha channel should be used for drawing images on the button (default is false)
void GUIButton::setUseAlphaChannel(bool useAlphaChannel)
{
	UseAlphaChannel = useAlphaChannel;
	Rebuild = true;
}


//! Returns if the alpha channel should be used for drawing images on the button
bool GUIButton::isAlphaChannelUsed() const
{
	return UseAlphaChannel;
}


bool GUIButton::isDrawingBorder() const
{
	return DrawBorder;
}


// PATCH
GUIButton* GUIButton::addButton(IGUIEnvironment *environment,
        const recti& rectangle,
		IGUIElement* parent, s32 id, const wchar_t* text,
		const wchar_t *tooltiptext)
{
    GUIButton* button = new GUIButton(environment, parent ? parent : environment->getRootGUIElement(), id, rectangle);
	if (text)
		button->setText(text);

	if ( tooltiptext )
		button->setToolTipText ( tooltiptext );

	button->drop();
	return button;
}

void GUIButton::setColor(img::color8 color)
{
	BgColor = color;

	float d = 0.65f;
	for (size_t i = 0; i < 4; i++) {
        img::color8 base = Environment->getSkin()->getColor((EGUI_DEFAULT_COLOR)i);
        Colors[i] = base.linInterp(color, d);
	}
	Rebuild = true;
}

//! Set element properties from a StyleSpec corresponding to the button state
void GUIButton::setFromState()
{
	StyleSpec::State state = StyleSpec::STATE_DEFAULT;

	if (isPressed())
		state = static_cast<StyleSpec::State>(state | StyleSpec::STATE_PRESSED);

	if (isHovered())
		state = static_cast<StyleSpec::State>(state | StyleSpec::STATE_HOVERED);

	if (isFocused())
		state = static_cast<StyleSpec::State>(state | StyleSpec::STATE_FOCUSED);

	setFromStyle(StyleSpec::getStyleFromStatePropagation(Styles, state));
}

//! Set element properties from a StyleSpec
void GUIButton::setFromStyle(const StyleSpec& style)
{
	bool hovered = (style.getState() & StyleSpec::STATE_HOVERED) != 0;
	bool pressed = (style.getState() & StyleSpec::STATE_PRESSED) != 0;

	if (style.isNotDefault(StyleSpec::BGCOLOR)) {
		setColor(style.getColor(StyleSpec::BGCOLOR));

		// If we have a propagated hover/press color, we need to automatically
		// lighten/darken it
		if (!Styles[style.getState()].isNotDefault(StyleSpec::BGCOLOR)) {
				if (pressed) {
					BgColor = multiplyColorValue(BgColor, COLOR_PRESSED_MOD);

					for (size_t i = 0; i < 4; i++)
						Colors[i] = multiplyColorValue(Colors[i], COLOR_PRESSED_MOD);
				} else if (hovered) {
					BgColor = multiplyColorValue(BgColor, COLOR_HOVERED_MOD);

					for (size_t i = 0; i < 4; i++)
						Colors[i] = multiplyColorValue(Colors[i], COLOR_HOVERED_MOD);
				}
		}

	} else {
        BgColor = img::white;
		for (size_t i = 0; i < 4; i++) {
			img::color8 base =
                    Environment->getSkin()->getColor((EGUI_DEFAULT_COLOR)i);
			if (pressed) {
				Colors[i] = multiplyColorValue(base, COLOR_PRESSED_MOD);
			} else if (hovered) {
				Colors[i] = multiplyColorValue(base, COLOR_HOVERED_MOD);
			} else {
				Colors[i] = base;
			}
		}
	}

	if (style.isNotDefault(StyleSpec::TEXTCOLOR)) {
		setOverrideColor(style.getColor(StyleSpec::TEXTCOLOR));
	} else {
        setOverrideColor(img::white);
		OverrideColorEnabled = false;
	}
	setNotClipped(style.getBool(StyleSpec::NOCLIP, false));
	setDrawBorder(style.getBool(StyleSpec::BORDER, true));
	setUseAlphaChannel(style.getBool(StyleSpec::ALPHA, true));
    setOverrideFont(style.getFont(Environment->getRenderSystem()->getFontManager()));

	if (style.isNotDefault(StyleSpec::BGIMG)) {
        img::Image *texture = style.getTexture(StyleSpec::BGIMG, Environment->getResourceCache());
        setImage(texture);
		setScaleImage(true);
	} else {
		setImage(nullptr);
	}

	BgMiddle = style.getRect(StyleSpec::BGIMG_MIDDLE, BgMiddle);

	// Child padding and offset
	Padding = style.getRect(StyleSpec::PADDING, recti());
	Padding = recti(
			Padding.ULC + BgMiddle.ULC,
			Padding.LRC + BgMiddle.LRC);

    GUISkin *skin = Environment->getSkin();
    v2i defaultPressOffset(
            skin->getSize(EGDS_BUTTON_PRESSED_IMAGE_OFFSET_X),
            skin->getSize(EGDS_BUTTON_PRESSED_IMAGE_OFFSET_Y));
	ContentOffset = style.getVector2i(StyleSpec::CONTENT_OFFSET, isPressed()
			? defaultPressOffset
            : v2i(0));

	recti childBounds(
				Padding.ULC.X + ContentOffset.X,
				Padding.ULC.Y + ContentOffset.Y,
				AbsoluteRect.getWidth() + Padding.LRC.X + ContentOffset.X,
				AbsoluteRect.getHeight() + Padding.LRC.Y + ContentOffset.Y);

	for (IGUIElement *child : getChildren()) {
		child->setRelativePosition(childBounds);
	}
	Rebuild = true;
}

//! Set the styles used for each state
void GUIButton::setStyles(const std::array<StyleSpec, StyleSpec::NUM_STATES>& styles)
{
	Styles = styles;
	setFromState();
}
// END PATCH
