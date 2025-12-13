#pragma once

#include "sprite.h"
#include "util/enriched_string.h"
#include "gui/GUIEnums.h"
#include <Render/TTFont.h>
#include <Render/Texture2D.h>
#include "gui/GUISkin.h"

class FontManager;

class UITextSprite : public UISprite
{
    GUISkin *skin;

	EnrichedString text;
	std::vector<EnrichedString> brokenText;

    GUIAlignment hAlign = GUIAlignment::UpperLeft;
    GUIAlignment vAlign = GUIAlignment::UpperLeft;
    bool overrideColorEnabled = false;
    bool overrideBGColorEnabled = false;
	bool drawBorder;
	bool drawBackground;
    bool wordWrap = false;
    bool clipText = true;
    bool rightToLeft = false;
	
    img::color8 overrideColor = img::color8(img::PF_RGBA8, 255, 255, 255, 101);
    img::color8 bgColor = img::color8(img::PF_RGBA8, 210, 210, 210, 101);
    render::TTFont *overrideFont = nullptr;

    FontManager *mgr;

    struct GlyphPrimitiveParams {
        rectf pos;
        std::array<img::color8, 4> colors;
        rectf uv;
    };

    std::vector<std::pair<render::Texture2D *, u32>> texture_to_charcount_map;
    std::vector<std::pair<render::Texture2D *, std::vector<GlyphPrimitiveParams>>> texture_to_glyph_map;
public:
    UITextSprite(FontManager *font_manager, GUISkin *guiskin, const EnrichedString &text, Renderer *renderer,
        ResourceCache *resCache, bool border = false, bool wordWrap = true, bool fillBackground = false);

    void setOverrideFont(render::TTFont *font);
    render::TTFont *getOverrideFont() const
    {
        return overrideFont;
    }
    render::TTFont *getActiveFont() const
    {
        return overrideFont ? overrideFont : skin->getFont();
    }

    void setOverrideColor(const img::color8 &c);
    img::color8 getOverrideColor() const
    {
        return overrideColor;
    }

    void setBackgroundColor(const img::color8 &c);
    img::color8 getBackgroundColor() const
    {
        return drawBackground ? bgColor : skin->getColor(GUIDefaultColor::Face3D);
    }

    img::color8 getActiveColor() const
    {
        return overrideColorEnabled ? overrideColor : skin->getColor(EGDC_BUTTON_TEXT);
    }

    void enableOverrideColor(bool enable)
    {
        overrideColorEnabled = enable;
    }

    bool isOverrideColorEnabled() const
    {
        return overrideColorEnabled;
    }

    u32 getLinesCount() const
    {
        return wordWrap ? brokenText.size() : 1;
    }

    std::wstring getText() const
    {
        return text.getString();
    }

    u32 getTextWidth() const;
    u32 getTextHeight() const;
    v2u getTextSize() const;

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
        setText(EnrichedString(text));
    }

    void draw(std::optional<u32> primOffset=std::nullopt, std::optional<u32> primCount=std::nullopt) override;

    void updateBuffer(rectf &&r);
private:
    void updateWrappedText();
    u32 getBrokenTextWidth() const;
    render::Texture2D *getGlyphAtlasTexture() const;
};
