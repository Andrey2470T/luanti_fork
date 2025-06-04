// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2019 Irrlick
//
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "GUISkin.h"

#include <Render/Texture2D.h>
#include <Image/Converting.h>
#include <Render/TTFont.h>
#include "client/ui/sprite.h"
#include "IGUIElement.h"
#include "client/media/resource.h"
#include "client/ui/renderer2d.h"
#include <Render/DrawContext.h>

GUISkin::GUISkin(GUISkinType type, ResourceCache *_cache, Renderer2D *_renderer)
    : cache(_cache), renderer(_renderer), Type(type)
{
    if ((Type == GUISkinType::WindowsClassic) || (Type == GUISkinType::WindowsMetallic))
	{
        Colors[(u8)GUIDefaultColor::DarkShadows3D]      = img::color8(img::PF_RGBA8, 101,50,50,50);
        Colors[(u8)GUIDefaultColor::Shadow3D]           = img::color8(img::PF_RGBA8, 101,130,130,130);
        Colors[(u8)GUIDefaultColor::Face3D]             = img::color8(img::PF_RGBA8, 220,100,100,100);
        Colors[(u8)GUIDefaultColor::HighLight3D]        = img::color8(img::PF_RGBA8, 101,255,255,255);
        Colors[(u8)GUIDefaultColor::Light3D]            = img::color8(img::PF_RGBA8, 101,210,210,210);
        Colors[(u8)GUIDefaultColor::ActiveBorder]       = img::color8(img::PF_RGBA8, 101,16,14,115);
        Colors[(u8)GUIDefaultColor::ActiveCaption]      = img::color8(img::PF_RGBA8, 255,255,255,255);
        Colors[(u8)GUIDefaultColor::AppWorkspace]       = img::color8(img::PF_RGBA8, 101,100,100,100);
        Colors[(u8)GUIDefaultColor::ButtonText]         = img::color8(img::PF_RGBA8, 240,10,10,10);
        Colors[(u8)GUIDefaultColor::GrayText]           = img::color8(img::PF_RGBA8, 240,130,130,130);
        Colors[(u8)GUIDefaultColor::HighLight]          = img::color8(img::PF_RGBA8, 101,8,36,107);
        Colors[(u8)GUIDefaultColor::HighLightText]      = img::color8(img::PF_RGBA8, 240,255,255,255);
        Colors[(u8)GUIDefaultColor::InactiveBorder]     = img::color8(img::PF_RGBA8, 101,165,165,165);
        Colors[(u8)GUIDefaultColor::InactiveCaption]    = img::color8(img::PF_RGBA8, 255,30,30,30);
        Colors[(u8)GUIDefaultColor::Tooltip]            = img::color8(img::PF_RGBA8, 200,0,0,0);
        Colors[(u8)GUIDefaultColor::TooltipBackground]  = img::color8(img::PF_RGBA8, 200,255,255,225);
        Colors[(u8)GUIDefaultColor::Scrollbar]          = img::color8(img::PF_RGBA8, 101,230,230,230);
        Colors[(u8)GUIDefaultColor::Window]             = img::color8(img::PF_RGBA8, 101,255,255,255);
        Colors[(u8)GUIDefaultColor::WindowSymbol]       = img::color8(img::PF_RGBA8, 200,10,10,10);
        Colors[(u8)GUIDefaultColor::Icon]               = img::color8(img::PF_RGBA8, 200,255,255,255);
        Colors[(u8)GUIDefaultColor::IconHighLight]      = img::color8(img::PF_RGBA8, 200,8,36,107);
        Colors[(u8)GUIDefaultColor::GrayWindowSymbol]   = img::color8(img::PF_RGBA8, 240,100,100,100);
        Colors[(u8)GUIDefaultColor::Editable] 			= img::color8(img::PF_RGBA8, 255,255,255,255);
        Colors[(u8)GUIDefaultColor::GrayEditable]		= img::color8(img::PF_RGBA8, 255,120,120,120);
        Colors[(u8)GUIDefaultColor::FocusedEditable]	= img::color8(img::PF_RGBA8, 255,240,240,255);


        Sizes[(u8)GUIDefaultSize::ScrollbarSize] = 14;
        Sizes[(u8)GUIDefaultSize::MenuHeight] = 30;
        Sizes[(u8)GUIDefaultSize::WindowButtonWidth] = 15;
        Sizes[(u8)GUIDefaultSize::CheckBoxWidth] = 18;
        Sizes[(u8)GUIDefaultSize::MessageBoxWidth] = 500;
        Sizes[(u8)GUIDefaultSize::MessageBoxHeight] = 200;
        Sizes[(u8)GUIDefaultSize::ButtonWidth] = 80;
        Sizes[(u8)GUIDefaultSize::ButtonHeight] = 30;

        Sizes[(u8)GUIDefaultSize::TextDistanceX] = 2;
        Sizes[(u8)GUIDefaultSize::TextDistanceY] = 0;

        Sizes[(u8)GUIDefaultSize::TitlebartextDistanceX] = 2;
        Sizes[(u8)GUIDefaultSize::TitlebartextDistanceY] = 0;
	}
	else
	{
		//0x80a6a8af
        Colors[(u8)GUIDefaultColor::Shadow3D]           =	img::colorU32NumberToObject(0x60767982);
		//Colors[EGDC_3D_FACE]			=	0xc0c9ccd4;		// tab background
        Colors[(u8)GUIDefaultColor::Face3D]             =	img::colorU32NumberToObject(0xc0cbd2d9);		// tab background
        Colors[(u8)GUIDefaultColor::Shadow3D]			=	img::colorU32NumberToObject(0x50e4e8f1);		// tab background, and left-top highlight
        Colors[(u8)GUIDefaultColor::HighLight3D]		=	img::colorU32NumberToObject(0x40c7ccdc);
        Colors[(u8)GUIDefaultColor::Light3D]			=	img::colorU32NumberToObject(0x802e313a);
        Colors[(u8)GUIDefaultColor::ActiveBorder]		=	img::colorU32NumberToObject(0x80404040);		// window title
        Colors[(u8)GUIDefaultColor::ActiveCaption]      =	img::colorU32NumberToObject(0xffd0d0d0);
        Colors[(u8)GUIDefaultColor::AppWorkspace]		=	img::colorU32NumberToObject(0xc0646464);		// unused
        Colors[(u8)GUIDefaultColor::ButtonText]         =	img::colorU32NumberToObject(0xd0161616);
        Colors[(u8)GUIDefaultColor::GrayText]			=	img::colorU32NumberToObject(0x3c141414);
        Colors[(u8)GUIDefaultColor::HighLight]			=	img::colorU32NumberToObject(0x6c606060);
        Colors[(u8)GUIDefaultColor::HighLightText]      =	img::colorU32NumberToObject(0xd0e0e0e0);
        Colors[(u8)GUIDefaultColor::InactiveBorder]     =	img::colorU32NumberToObject(0xf0a5a5a5);
        Colors[(u8)GUIDefaultColor::InactiveCaption]	=	img::colorU32NumberToObject(0xffd2d2d2);
        Colors[(u8)GUIDefaultColor::Tooltip]			=	img::colorU32NumberToObject(0xf00f2033);
        Colors[(u8)GUIDefaultColor::TooltipBackground]	= 	img::colorU32NumberToObject(0xc0cbd2d9);
        Colors[(u8)GUIDefaultColor::Scrollbar]			= 	img::colorU32NumberToObject(0xf0e0e0e0);
        Colors[(u8)GUIDefaultColor::Window]				= 	img::colorU32NumberToObject(0xf0f0f0f0);
        Colors[(u8)GUIDefaultColor::WindowSymbol]		= 	img::colorU32NumberToObject(0xd0161616);
        Colors[(u8)GUIDefaultColor::Icon]				= 	img::colorU32NumberToObject(0xd0161616);
        Colors[(u8)GUIDefaultColor::IconHighLight]      = 	img::colorU32NumberToObject(0xd0606060);
        Colors[(u8)GUIDefaultColor::GrayWindowSymbol]   = 	img::colorU32NumberToObject(0x3c101010);
        Colors[(u8)GUIDefaultColor::Editable] 			= 	img::colorU32NumberToObject(0xf0ffffff);
        Colors[(u8)GUIDefaultColor::GrayEditable]		= 	img::colorU32NumberToObject(0xf0cccccc);
        Colors[(u8)GUIDefaultColor::FocusedEditable]	= 	img::colorU32NumberToObject(0xf0fffff0);

        Sizes[(u8)GUIDefaultSize::ScrollbarSize] = 14;
        Sizes[(u8)GUIDefaultSize::MenuHeight] = 48;
        Sizes[(u8)GUIDefaultSize::WindowButtonWidth] = 15;
        Sizes[(u8)GUIDefaultSize::CheckBoxWidth] = 18;
        Sizes[(u8)GUIDefaultSize::MessageBoxWidth] = 500;
        Sizes[(u8)GUIDefaultSize::MessageBoxHeight] = 200;
        Sizes[(u8)GUIDefaultSize::ButtonWidth] = 80;
        Sizes[(u8)GUIDefaultSize::ButtonHeight] = 30;

        Sizes[(u8)GUIDefaultSize::TextDistanceX] = 3;
        Sizes[(u8)GUIDefaultSize::TextDistanceY] = 2;

        Sizes[(u8)GUIDefaultSize::TitlebartextDistanceX] = 3;
        Sizes[(u8)GUIDefaultSize::TitlebartextDistanceY] = 2;
	}

    Sizes[(u8)GUIDefaultSize::MessageBoxGapSpace] = 15;
    Sizes[(u8)GUIDefaultSize::MessageBoxMinTextWidth] = 0;
    Sizes[(u8)GUIDefaultSize::MessageBoxMaxTextWidth] = 500;
    Sizes[(u8)GUIDefaultSize::MessageBoxMinTextHeight] = 0;
    Sizes[(u8)GUIDefaultSize::MessageBoxMaxTextHeight] = 99999;

    Sizes[(u8)GUIDefaultSize::ButtonPressedImageOffsetX] = 1;
    Sizes[(u8)GUIDefaultSize::ButtonPressedImageOffsetY] = 1;
    Sizes[(u8)GUIDefaultSize::ButtonPressedTextOffsetX] = 0;
    Sizes[(u8)GUIDefaultSize::ButtonPressedTextOffsetY] = 2;

    Texts[(u8)GUIDefaultText::MsgBoxOk] = L"OK";
    Texts[(u8)GUIDefaultText::MsgBoxCancel] = L"Cancel";
    Texts[(u8)GUIDefaultText::MsgBoxYes] = L"Yes";
    Texts[(u8)GUIDefaultText::MsgBoxNo] = L"No";
    Texts[(u8)GUIDefaultText::WindowClose] = L"Close";
    Texts[(u8)GUIDefaultText::WindowRestore] = L"Restore";
    Texts[(u8)GUIDefaultText::WindowMinimize] = L"Minimize";
    Texts[(u8)GUIDefaultText::WindowMaximize] = L"Maximize";

    Icons[(u8)GUIDefaultIcon::WindowMaximize] = 225;
    Icons[(u8)GUIDefaultIcon::WindowRestore] = 226;
    Icons[(u8)GUIDefaultIcon::WindowClose] = 227;
    Icons[(u8)GUIDefaultIcon::WindowMinimize] = 228;
    Icons[(u8)GUIDefaultIcon::CursorUp] = 229;
    Icons[(u8)GUIDefaultIcon::CursorDown] = 230;
    Icons[(u8)GUIDefaultIcon::CursorLeft] = 231;
    Icons[(u8)GUIDefaultIcon::CursorRight] = 232;
    Icons[(u8)GUIDefaultIcon::MenuMore] = 232;
    Icons[(u8)GUIDefaultIcon::CheckBoxChecked] = 233;
    Icons[(u8)GUIDefaultIcon::Dropdown] = 234;
    Icons[(u8)GUIDefaultIcon::SmallCursorUp] = 235;
    Icons[(u8)GUIDefaultIcon::SmallCursorDown] = 236;
    Icons[(u8)GUIDefaultIcon::RadioButtonChecked] = 237;
    Icons[(u8)GUIDefaultIcon::MoreLeft] = 238;
    Icons[(u8)GUIDefaultIcon::MoreRight] = 239;
    Icons[(u8)GUIDefaultIcon::MoreUp] = 240;
    Icons[(u8)GUIDefaultIcon::MoreDown] = 241;
    Icons[(u8)GUIDefaultIcon::WindowResize] = 242;
    Icons[(u8)GUIDefaultIcon::Expand] = 243;
    Icons[(u8)GUIDefaultIcon::Collapse] = 244;

    Icons[(u8)GUIDefaultIcon::File] = 245;
    Icons[(u8)GUIDefaultIcon::Directory] = 246;

    for (u8 i = 0; i < (u8)GUIDefaultFont::Count; i++)
        Fonts[i] = nullptr;
    UseGradient = (Type == GUISkinType::WindowsMetallic) || (Type == GUISkinType::BurningSkin) ;
}

GUISkin::~GUISkin()
{
    for (u8 i = 0; i < (u8)GUIDefaultFont::Count; i++) {
        auto font = Fonts[i];
        if (!font)
            continue;
        cache->clearResource<render::TTFont>(ResourceType::FONT, font);
    }
}

//! returns default color
img::color8 GUISkin::getColor(GUIDefaultColor color) const
{
    if ((u8)color < (u8)GUIDefaultColor::Count)
        return Colors[(u8)color];
	else
		return img::color8();
}


//! sets a default color
void GUISkin::setColor(GUIDefaultColor which, img::color8 newColor)
{
    if ((u8)which < (u8)GUIDefaultColor::Count)
        Colors[(u8)which] = newColor;
}


//! returns size for the given size type
s32 GUISkin::getSize(GUIDefaultSize size) const
{
    if ((u8)size < (u8)GUIDefaultSize::Count)
        return Sizes[(u8)size];
	else
		return 0;
}


//! sets a default size
void GUISkin::setSize(GUIDefaultSize which, s32 size)
{
    if ((u8)which < (u8)GUIDefaultSize::Count)
        Sizes[(u8)which] = size;
}


//! returns the default font
render::TTFont *GUISkin::getFont(GUIDefaultFont which) const
{
    if (((u8)which < (u8)GUIDefaultFont::Count) && Fonts[(u8)which])
        return Fonts[(u8)which];
	else
        return Fonts[(u8)GUIDefaultFont::Default];
}


//! sets a default font
void GUISkin::setFont(render::TTFont *font, GUIDefaultFont which)
{
    if ((u8)which >= (u8)GUIDefaultFont::Count)
		return;

    if (font) {
        if (Fonts[(u8)which])
            cache->clearResource<render::TTFont>(ResourceType::FONT, Fonts[(u8)which]);
        Fonts[(u8)which] = font;
        cache->cacheResource<render::TTFont>(ResourceType::FONT, font);
    }
}


//! gets the sprite bank stored
UISpriteBank *GUISkin::getSpriteBank() const
{
    return SpriteBank.get();
}


//! set a new sprite bank or remove one by passing 0
void GUISkin::setSpriteBank(UISpriteBank *bank)
{
    if (bank)
        SpriteBank.reset(bank);
}


//! Returns a default icon
u32 GUISkin::getIcon(GUIDefaultIcon icon) const
{
    if ((u8)icon < (u8)GUIDefaultIcon::Count)
        return Icons[(u8)icon];
	else
		return 0;
}


//! Sets a default icon
void GUISkin::setIcon(GUIDefaultIcon icon, u32 index)
{
    if ((u8)icon < (u8)GUIDefaultIcon::Count)
        Icons[(u8)icon] = index;
}


//! Returns a default text. For example for Message box button captions:
//! "OK", "Cancel", "Yes", "No" and so on.
std::wstring GUISkin::getDefaultText(GUIDefaultText text) const
{
    if ((u8)text < (u8)GUIDefaultText::Count)
        return Texts[(u8)text];
	else
        return Texts[0];
}


//! Sets a default text. For example for Message box button captions:
//! "OK", "Cancel", "Yes", "No" and so on.
void GUISkin::setDefaultText(GUIDefaultText which, const std::wstring &newText)
{
    if ((u8)which < (u8)GUIDefaultText::Count)
        Texts[(u8)which] = newText;
}


//! draws a standard 3d button pane
/**	Used for drawing for example buttons in normal state.
It uses the colors GUIDefaultColor::DarkShadows3D, GUIDefaultColor::HighLight3D, GUIDefaultColor::Shadow3D and
EGDC_3D_FACE for this. See GUIDefaultColor for details.
\param rect: Defining area where to draw.
\param clip: Clip area.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly. */
// PATCH
void GUISkin::drawColored3DButtonPaneStandard(UISprite *sprite,
    const recti& r,
    const recti* clip,
    const img::color8 *colors)
{
    if (!colors)
        colors = Colors.data();

    rectf rect(r.ULC.X, r.ULC.Y, r.LRC.X, r.LRC.Y);

    if ( Type == GUISkinType::BurningSkin )
	{
        rect.ULC.X -= 1.0f;
        rect.ULC.Y -= 1.0f;
        rect.LRC.X += 1.0f;
        rect.LRC.Y += 1.0f;
        img::color8 whiteC(img::PF_RGBA8, 255, 255, 255, 255);
        draw3DSunkenPane(sprite, colors[ (u8)GUIDefaultColor::Window ].linInterp(whiteC, 0.9f ),
            false, true, rect, clip);
		return;
	}

    sprite->setClipRect(*clip);

    sprite->setColor(colors[(u8)GUIDefaultColor::DarkShadows3D]);
    sprite->updateRect(rect);

    rect.LRC.X -= 1.0f;
    rect.LRC.Y -= 1.0f;
    sprite->setColor(colors[(u8)GUIDefaultColor::HighLight3D]);
    sprite->updateRect(rect);

    rect.ULC.X += 1.0f;
    rect.ULC.Y += 1.0f;
    sprite->setColor(colors[(u8)GUIDefaultColor::Shadow3D]);
    sprite->updateRect(rect);

    rect.LRC.X -= 1.0f;
    rect.LRC.Y -= 1.0f;

	if (!UseGradient)
	{
		Driver->draw2DRectangle(colors[EGDC_3D_FACE], rect, clip);
	}
	else
	{
		const img::color8 c1 = colors[EGDC_3D_FACE];
        const img::color8 c2 = c1.getInterpolated(colors[GUIDefaultColor::DarkShadows3D], 0.4f);
		Driver->draw2DRectangle(rect, c1, c1, c2, c2, clip);
	}
}
// END PATCH


//! draws a pressed 3d button pane
/**	Used for drawing for example buttons in pressed state.
It uses the colors GUIDefaultColor::DarkShadows3D, GUIDefaultColor::HighLight3D, GUIDefaultColor::Shadow3D and
EGDC_3D_FACE for this. See GUIDefaultColor for details.
\param rect: Defining area where to draw.
\param clip: Clip area.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly. */
// PATCH
void CGUISkin::drawColored3DButtonPanePressed(std::shared_ptr<IGUIElement> element,
					const recti& r,
					const recti* clip,
					const img::color8* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	recti rect = r;
    Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], rect, clip);

    rect.LRC.X -= 1;
    rect.LRC.Y -= 1;
    Driver->draw2DRectangle(colors[GUIDefaultColor::DarkShadows3D], rect, clip);

    rect.ULC.X += 1;
    rect.ULC.Y += 1;
    Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);

    rect.ULC.X += 1;
    rect.ULC.Y += 1;

	if (!UseGradient)
	{
		Driver->draw2DRectangle(colors[EGDC_3D_FACE], rect, clip);
	}
	else
	{
		const img::color8 c1 = colors[EGDC_3D_FACE];
        const img::color8 c2 = c1.getInterpolated(colors[GUIDefaultColor::DarkShadows3D], 0.4f);
		Driver->draw2DRectangle(rect, c1, c1, c2, c2, clip);
	}
}
// END PATCH


//! draws a sunken 3d pane
/** Used for drawing the background of edit, combo or check boxes.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param bgcolor: Background color.
\param flat: Specifies if the sunken pane should be flat or displayed as sunken
deep into the ground.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DSunkenPane(std::shared_ptr<IGUIElement> element, img::color8 bgcolor,
				bool flat, bool fillBackGround,
				const recti& r,
				const recti* clip,
				const img::color8* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	recti rect = r;

	if (fillBackGround)
		Driver->draw2DRectangle(bgcolor, rect, clip);

	if (flat)
	{
		// draw flat sunken pane

        rect.LRC.Y = rect.ULC.Y + 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);	// top

        ++rect.ULC.Y;
        rect.LRC.Y = r.LRC.Y;
        rect.LRC.X = rect.ULC.X + 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);	// left

		rect = r;
        ++rect.ULC.Y;
        rect.ULC.X = rect.LRC.X - 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], rect, clip);	// right

		rect = r;
        ++rect.ULC.X;
        rect.ULC.Y = r.LRC.Y - 1;
        --rect.LRC.X;
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], rect, clip);	// bottom
	}
	else
	{
		// draw deep sunken pane
        rect.LRC.Y = rect.ULC.Y + 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);	// top
        ++rect.ULC.X;
        ++rect.ULC.Y;
        --rect.LRC.X;
        ++rect.LRC.Y;
        Driver->draw2DRectangle(colors[GUIDefaultColor::DarkShadows3D], rect, clip);

        rect.ULC.X = r.ULC.X;
        rect.ULC.Y = r.ULC.Y+1;
        rect.LRC.X = rect.ULC.X + 1;
        rect.LRC.Y = r.LRC.Y;
        Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);	// left
        ++rect.ULC.X;
        ++rect.ULC.Y;
        ++rect.LRC.X;
        --rect.LRC.Y;
        Driver->draw2DRectangle(colors[GUIDefaultColor::DarkShadows3D], rect, clip);

		rect = r;
        rect.ULC.X = rect.LRC.X - 1;
        ++rect.ULC.Y;
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], rect, clip);	// right
        --rect.ULC.X;
        ++rect.ULC.Y;
        --rect.LRC.X;
        --rect.LRC.Y;
		Driver->draw2DRectangle(colors[EGDC_3D_LIGHT], rect, clip);

		rect = r;
        ++rect.ULC.X;
        rect.ULC.Y = r.LRC.Y - 1;
        --rect.LRC.X;
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], rect, clip);	// bottom
        ++rect.ULC.X;
        --rect.ULC.Y;
        --rect.LRC.X;
        --rect.LRC.Y;
		Driver->draw2DRectangle(colors[EGDC_3D_LIGHT], rect, clip);
	}
}
// END PATCH

//! draws a window background
// return where to draw title bar text.
// PATCH
recti CGUISkin::drawColored3DWindowBackground(std::shared_ptr<IGUIElement> element,
				bool drawTitleBar, img::color8 titleBarColor,
				const recti& r,
				const recti* clip,
				recti* checkClientArea,
				const img::color8* colors)
{
	if (!Driver)
	{
		if ( checkClientArea )
		{
			*checkClientArea = r;
		}
		return r;
	}

	if (!colors)
		colors = Colors;

	recti rect = r;

	// top border
    rect.LRC.Y = rect.ULC.Y + 1;
	if ( !checkClientArea )
	{
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], rect, clip);
	}

	// left border
    rect.LRC.Y = r.LRC.Y;
    rect.LRC.X = rect.ULC.X + 1;
	if ( !checkClientArea )
	{
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], rect, clip);
	}

	// right border dark outer line
    rect.ULC.X = r.LRC.X - 1;
    rect.LRC.X = r.LRC.X;
    rect.ULC.Y = r.ULC.Y;
    rect.LRC.Y = r.LRC.Y;
	if ( !checkClientArea )
	{
        Driver->draw2DRectangle(colors[GUIDefaultColor::DarkShadows3D], rect, clip);
	}

	// right border bright innner line
    rect.ULC.X -= 1;
    rect.LRC.X -= 1;
    rect.ULC.Y += 1;
    rect.LRC.Y -= 1;
	if ( !checkClientArea )
	{
        Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);
	}

	// bottom border dark outer line
    rect.ULC.X = r.ULC.X;
    rect.ULC.Y = r.LRC.Y - 1;
    rect.LRC.Y = r.LRC.Y;
    rect.LRC.X = r.LRC.X;
	if ( !checkClientArea )
	{
        Driver->draw2DRectangle(colors[GUIDefaultColor::DarkShadows3D], rect, clip);
	}

	// bottom border bright inner line
    rect.ULC.X += 1;
    rect.LRC.X -= 1;
    rect.ULC.Y -= 1;
    rect.LRC.Y -= 1;
	if ( !checkClientArea )
	{
        Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);
	}

	// client area for background
	rect = r;
    rect.ULC.X +=1;
    rect.ULC.Y +=1;
    rect.LRC.X -= 2;
    rect.LRC.Y -= 2;
	if (checkClientArea)
	{
		*checkClientArea = rect;
	}

	if ( !checkClientArea )
	{
		if (!UseGradient)
		{
			Driver->draw2DRectangle(colors[EGDC_3D_FACE], rect, clip);
		}
        else if ( Type == GUISkinType::BurningSkin )
		{
            const img::color8 c1 = colors[GUIDefaultColor::Window].getInterpolated ( 0xFFFFFFFF, 0.9f );
            const img::color8 c2 = colors[GUIDefaultColor::Window].getInterpolated ( 0xFFFFFFFF, 0.8f );

			Driver->draw2DRectangle(rect, c1, c1, c2, c2, clip);
		}
		else
		{
            const img::color8 c2 = colors[GUIDefaultColor::Shadow3D];
			const img::color8 c1 = colors[EGDC_3D_FACE];
			Driver->draw2DRectangle(rect, c1, c1, c1, c2, clip);
		}
	}

	// title bar
	rect = r;
    rect.ULC.X += 2;
    rect.ULC.Y += 2;
    rect.LRC.X -= 2;
    rect.LRC.Y = rect.ULC.Y + getSize(EGDS_WINDOW_BUTTON_WIDTH) + 2;

	if (drawTitleBar )
	{
		if (checkClientArea)
		{
            (*checkClientArea).ULC.Y = rect.LRC.Y;
		}
		else
		{
			// draw title bar
			//if (!UseGradient)
			//	Driver->draw2DRectangle(titleBarColor, rect, clip);
			//else
            if ( Type == GUISkinType::BurningSkin )
			{
				const img::color8 c = titleBarColor.getInterpolated( img::color8(titleBarColor.getAlpha(),255,255,255), 0.8f);
				Driver->draw2DRectangle(rect, titleBarColor, titleBarColor, c, c, clip);
			}
			else
			{
				const img::color8 c = titleBarColor.getInterpolated(img::color8(titleBarColor.getAlpha(),0,0,0), 0.2f);
				Driver->draw2DRectangle(rect, titleBarColor, c, titleBarColor, c, clip);
			}
		}
	}

	return rect;
}
// END PATCH


//! draws a standard 3d menu pane
/**	Used for drawing for menus and context menus.
It uses the colors GUIDefaultColor::DarkShadows3D, GUIDefaultColor::HighLight3D, GUIDefaultColor::Shadow3D and
EGDC_3D_FACE for this. See GUIDefaultColor for details.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DMenuPane(std::shared_ptr<IGUIElement> element,
			const recti& r, const recti* clip,
			const img::color8* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	recti rect = r;

    if ( Type == GUISkinType::BurningSkin )
	{
        rect.ULC.Y -= 3;
		draw3DButtonPaneStandard(element, rect, clip);
		return;
	}

	// in this skin, this is exactly what non pressed buttons look like,
	// so we could simply call
	// draw3DButtonPaneStandard(element, rect, clip);
	// here.
	// but if the skin is transparent, this doesn't look that nice. So
	// We draw it a little bit better, with some more draw2DRectangle calls,
	// but there aren't that much menus visible anyway.

    rect.LRC.Y = rect.ULC.Y + 1;
    Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], rect, clip);

    rect.LRC.Y = r.LRC.Y;
    rect.LRC.X = rect.ULC.X + 1;
    Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], rect, clip);

    rect.ULC.X = r.LRC.X - 1;
    rect.LRC.X = r.LRC.X;
    rect.ULC.Y = r.ULC.Y;
    rect.LRC.Y = r.LRC.Y;
    Driver->draw2DRectangle(colors[GUIDefaultColor::DarkShadows3D], rect, clip);

    rect.ULC.X -= 1;
    rect.LRC.X -= 1;
    rect.ULC.Y += 1;
    rect.LRC.Y -= 1;
    Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);

    rect.ULC.X = r.ULC.X;
    rect.ULC.Y = r.LRC.Y - 1;
    rect.LRC.Y = r.LRC.Y;
    rect.LRC.X = r.LRC.X;
    Driver->draw2DRectangle(colors[GUIDefaultColor::DarkShadows3D], rect, clip);

    rect.ULC.X += 1;
    rect.LRC.X -= 1;
    rect.ULC.Y -= 1;
    rect.LRC.Y -= 1;
    Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);

	rect = r;
    rect.ULC.X +=1;
    rect.ULC.Y +=1;
    rect.LRC.X -= 2;
    rect.LRC.Y -= 2;

	if (!UseGradient)
		Driver->draw2DRectangle(colors[EGDC_3D_FACE], rect, clip);
	else
	{
		const img::color8 c1 = colors[EGDC_3D_FACE];
        const img::color8 c2 = colors[GUIDefaultColor::Shadow3D];
		Driver->draw2DRectangle(rect, c1, c1, c2, c2, clip);
	}
}
// END PATCH


//! draws a standard 3d tool bar
/**	Used for drawing for toolbars and menus.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DToolBar(std::shared_ptr<IGUIElement> element,
				const recti& r,
				const recti* clip,
				const img::color8* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	recti rect = r;

    rect.ULC.X = r.ULC.X;
    rect.ULC.Y = r.LRC.Y - 1;
    rect.LRC.Y = r.LRC.Y;
    rect.LRC.X = r.LRC.X;
    Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], rect, clip);

	rect = r;
    rect.LRC.Y -= 1;

	if (!UseGradient)
	{
		Driver->draw2DRectangle(colors[EGDC_3D_FACE], rect, clip);
	}
	else
    if ( Type == GUISkinType::BurningSkin )
	{
		const img::color8 c1 = 0xF0000000 | colors[EGDC_3D_FACE].color;
        const img::color8 c2 = 0xF0000000 | colors[GUIDefaultColor::Shadow3D].color;

        rect.LRC.Y += 1;
		Driver->draw2DRectangle(rect, c1, c2, c1, c2, clip);
	}
	else
	{
		const img::color8 c1 = colors[EGDC_3D_FACE];
        const img::color8 c2 = colors[GUIDefaultColor::Shadow3D];
		Driver->draw2DRectangle(rect, c1, c1, c2, c2, clip);
	}
}
// END PATCH

//! draws a tab button
/**	Used for drawing for tab buttons on top of tabs.
\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param active: Specifies if the tab is currently active.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DTabButton(std::shared_ptr<IGUIElement> element, bool active,
	const recti& frameRect, const recti* clip, EGUI_ALIGNMENT alignment,
	const img::color8* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	recti tr = frameRect;

	if ( alignment == EGUIA_UPPERLEFT )
	{
        tr.LRC.X -= 2;
        tr.LRC.Y = tr.ULC.Y + 1;
        tr.ULC.X += 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], tr, clip);

		// draw left highlight
		tr = frameRect;
        tr.LRC.X = tr.ULC.X + 1;
        tr.ULC.Y += 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], tr, clip);

		// draw grey background
		tr = frameRect;
        tr.ULC.X += 1;
        tr.ULC.Y += 1;
        tr.LRC.X -= 2;
		Driver->draw2DRectangle(colors[EGDC_3D_FACE], tr, clip);

		// draw right middle gray shadow
        tr.LRC.X += 1;
        tr.ULC.X = tr.LRC.X - 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], tr, clip);

        tr.LRC.X += 1;
        tr.ULC.X += 1;
        tr.ULC.Y += 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::DarkShadows3D], tr, clip);
	}
	else
	{
        tr.LRC.X -= 2;
        tr.ULC.Y = tr.LRC.Y - 1;
        tr.ULC.X += 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], tr, clip);

		// draw left highlight
		tr = frameRect;
        tr.LRC.X = tr.ULC.X + 1;
        tr.LRC.Y -= 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], tr, clip);

		// draw grey background
		tr = frameRect;
        tr.ULC.X += 1;
        tr.ULC.Y -= 1;
        tr.LRC.X -= 2;
        tr.LRC.Y -= 1;
		Driver->draw2DRectangle(colors[EGDC_3D_FACE], tr, clip);

		// draw right middle gray shadow
        tr.LRC.X += 1;
        tr.ULC.X = tr.LRC.X - 1;
        //tr.LRC.Y -= 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], tr, clip);

        tr.LRC.X += 1;
        tr.ULC.X += 1;
        tr.LRC.Y -= 1;
        Driver->draw2DRectangle(colors[GUIDefaultColor::DarkShadows3D], tr, clip);
	}
}
// END PATCH


//! draws a tab control body
/**	\param element: Pointer to the element which wishes to draw this. This parameter
is usually not used by ISkin, but can be used for example by more complex
implementations to find out how to draw the part exactly.
\param border: Specifies if the border should be drawn.
\param background: Specifies if the background should be drawn.
\param rect: Defining area where to draw.
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColored3DTabBody(std::shared_ptr<IGUIElement> element, bool border, bool background,
	const recti& rect, const recti* clip, s32 tabHeight, EGUI_ALIGNMENT alignment,
	const img::color8* colors)
{
	if (!Driver)
		return;

	if (!colors)
		colors = Colors;

	recti tr = rect;

	if ( tabHeight == -1 )
		tabHeight = getSize(gui::EGDS_BUTTON_HEIGHT);

	// draw border.
	if (border)
	{
		if ( alignment == EGUIA_UPPERLEFT )
		{
			// draw left hightlight
            tr.ULC.Y += tabHeight + 2;
            tr.LRC.X = tr.ULC.X + 1;
            Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], tr, clip);

			// draw right shadow
            tr.ULC.X = rect.LRC.X - 1;
            tr.LRC.X = tr.ULC.X + 1;
            Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], tr, clip);

			// draw lower shadow
			tr = rect;
            tr.ULC.Y = tr.LRC.Y - 1;
            Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], tr, clip);
		}
		else
		{
			// draw left hightlight
            tr.LRC.Y -= tabHeight + 2;
            tr.LRC.X = tr.ULC.X + 1;
            Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], tr, clip);

			// draw right shadow
            tr.ULC.X = rect.LRC.X - 1;
            tr.LRC.X = tr.ULC.X + 1;
            Driver->draw2DRectangle(colors[GUIDefaultColor::Shadow3D], tr, clip);

			// draw lower shadow
			tr = rect;
            tr.LRC.Y = tr.ULC.Y + 1;
            Driver->draw2DRectangle(colors[GUIDefaultColor::HighLight3D], tr, clip);
		}
	}

	if (background)
	{
		if ( alignment == EGUIA_UPPERLEFT )
		{
			tr = rect;
            tr.ULC.Y += tabHeight + 2;
            tr.LRC.X -= 1;
            tr.ULC.X += 1;
            tr.LRC.Y -= 1;
		}
		else
		{
			tr = rect;
            tr.ULC.X += 1;
            tr.ULC.Y -= 1;
            tr.LRC.X -= 1;
            tr.LRC.Y -= tabHeight + 2;
            //tr.ULC.X += 1;
		}

		if (!UseGradient)
			Driver->draw2DRectangle(colors[EGDC_3D_FACE], tr, clip);
		else
		{
			img::color8 c1 = colors[EGDC_3D_FACE];
            img::color8 c2 = colors[GUIDefaultColor::Shadow3D];
			Driver->draw2DRectangle(tr, c1, c1, c2, c2, clip);
		}
	}
}
// END PATCH


//! draws an icon, usually from the skin's sprite bank
/**	\param parent: Pointer to the element which wishes to draw this icon.
This parameter is usually not used by IGUISkin, but can be used for example
by more complex implementations to find out how to draw the part exactly.
\param icon: Specifies the icon to be drawn.
\param position: The position to draw the icon
\param starttime: The time at the start of the animation
\param currenttime: The present time, used to calculate the frame number
\param loop: Whether the animation should loop or not
\param clip: Clip area.	*/
// PATCH
void CGUISkin::drawColoredIcon(std::shared_ptr<IGUIElement> element, GUIDefaultIcon icon,
			const v2i position,
			u32 starttime, u32 currenttime,
			bool loop, const recti* clip,
			const img::color8* colors)
{
	if (!SpriteBank)
		return;

	if (!colors)
		colors = Colors;

	bool gray = element && !element->isEnabled();
	SpriteBank->draw2DSprite(Icons[icon], position, clip,
            colors[gray? EGDC_GRAY_WINDOW_SYMBOL : GUIDefaultColor::Window_SYMBOL], starttime, currenttime, loop, true);
}
// END PATCH


GUISkinType CGUISkin::getType() const
{
	return Type;
}


//! draws a 2d rectangle.
void CGUISkin::draw2DRectangle(std::shared_ptr<IGUIElement> element,
		const img::color8 &color, const recti& pos,
		const recti* clip)
{
	Driver->draw2DRectangle(color, pos, clip);
}


//! gets the colors
// PATCH
void CGUISkin::getColors(img::color8* colors)
{
	u32 i;
    for (i=0; i<(u32)GUIDefaultColor::Count; ++i)
		colors[i] = Colors[i];
}
