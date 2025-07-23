#pragma once

#include "sprite.h"
#include "util/enriched_string.h"
#include "irrlicht_gui/GUIEnums.h"
#include <Render/TTFont.h>
#include <Render/StreamTexture2D.h>
#include "irrlicht_gui/GUISkin.h"

class FontManager;

class UITextSprite : public UISprite
{
    GUISkin *skin;

	EnrichedString text;
	std::vector<EnrichedString> brokenText;

    GUIAlignment hAlign = GUIAlignment::UpperLeft;
    GUIAlignment vAlign = GUIAlignment::UpperLeft;
	bool drawBorder;
	bool drawBackground;
	bool wordWrap;
    bool clipText = true;
    bool rightToLeft = false;
	
	render::TTFont *overrideFont;

    FontManager *mgr;
public:
    UITextSprite(FontManager *font_manager, const EnrichedString &text, Renderer *renderer,
        ResourceCache *resCache, bool border = false, bool wordWrap = true, bool fillBackground = false);
    ~UITextSprite();

    void setOverrideFont(render::TTFont *font);
    render::TTFont *getOverrideFont() const
    {
        return overrideFont;
    }
    render::TTFont *getActiveFont() const
    {
        return overrideFont ? overrideFont : skin->getFont();
    }

    void setColor(const img::color8 &c);
    img::color8 getColor() const
    {
        return text.getDefaultColor();
    }

    void setBackgroundColor(const img::color8 &c);
    img::color8 getBackgroundColor() const
    {
        return drawBackground ? text.getBackground() : skin->getColor(GUIDefaultColor::Face3D);
    }

    void enableDrawBackground(bool draw)
    {
        drawBackground = draw;
    }
    void enableDrawBorder(bool draw)
    {
        drawBorder = draw;
    }
    void enableClipText(bool clip)
    {
        clipText = clip;
    }
    void enableWordWrap(bool wrap);
    void enableRightToLeft(bool rtl);

    void setAlignment(GUIAlignment horizontal, GUIAlignment vertical);

    bool isDrawBackground() const
    {
        return drawBackground;
    }
    bool isDrawBorder() const
    {
        return drawBorder;
    }
    bool isWordWrap() const
    {
        return wordWrap;
    }
    bool isClipText() const
    {
        return clipText;
    }
    bool isRightToLeft() const
    {
        return rightToLeft;
    }

    void setText(const EnrichedString &text);
    void setText(const std::wstring &text)
    {
        setText(EnrichedString(text, getColor()));
    }

    void draw() override;

    void updateBuffer(rectf &&r);
private:
    void updateWrappedText();
    u32 getBrokenTextWidth() const;
    render::Texture2D *getGlyphAtlasTexture() const;
};
