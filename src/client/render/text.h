#pragma once

#include "util/enriched_string.h"
#include "gui/GUIEnums.h"
#include <Render/TTFont.h>
#include "gui/GUISkin.h"

class FontManager;
class UITextSprite;

// Text used for 2d and 3d display
class Text
{
    GUISkin *skin;

    EnrichedString text;
    std::vector<EnrichedString> brokenText;

    u32 textWidth, textHeight, lineHeight;

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

    bool needsUpdate = true;
public:
    Text(
        FontManager *font_manager, GUISkin *guiskin,
        std::variant<EnrichedString, std::wstring> text,
        bool border = false, bool wordWrap = false, bool fillBackground = false);

    GUISkin *getSkin() const
    {
        return skin;
    }
    FontManager *getFontManager() const
    {
        return mgr;
    }

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
    const std::vector<img::color8> &getTextColors() const
    {
        return text.getColors();
    }
    const std::vector<EnrichedString> &getBrokenText() const
    {
        return brokenText;
    }
    void getAlignment(GUIAlignment &horizontal, GUIAlignment &vertical) const
    {
        horizontal = hAlign;
        vertical = vAlign;
    }

    u32 getTextWidth() const
    {
        return textWidth;
    }
    u32 getTextHeight() const
    {
        return textHeight;
    }
    u32 getLineHeight() const
    {
        return lineHeight;
    }
    v2u getTextSize() const
    {
        return v2u(getTextWidth(), getTextHeight());
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
        setText(EnrichedString(text));
    }

    friend class UITextSprite;
private:
    void updateText(rectf clipRect);
};
