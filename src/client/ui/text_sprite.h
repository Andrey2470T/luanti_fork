#pragma once

#include "sprite.h"
#include "util/enriched_string.h"
#include "irrlicht_gui/GUIEnums.h"
#include <Render/TTFont.h>
#include <Render/StreamTexture2D.h>

class GUISkin;

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
public:
    UITextSprite(render::StreamTexture2D *tex, const EnrichedString &text, const rectf &posRect, Renderer2D *renderer, ResourceCache *resCache,
        bool border = false, bool wordWrap = true, bool fillBackground = false);
    ~UITextSprite();

    void setOverrideFont(render::TTFont *font);
    render::TTFont *getOverrideFont() const
    {
        return overrideFont;
    }
    render::TTFont *getActiveFont() const;

    void setOverrideColor(const img::color8 &c);
    img::color8 getOverrideColor() const;
    img::color8 getActiveColor() const;

    void setBackgroundColor(const img::color8 &c);
    img::color8 getBackgroundColor() const;

    void enableDrawBackground(bool draw);
    void enableDrawBorder(bool draw);
    void enableClipText(bool clip);
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
        setText(EnrichedString(text, getOverrideColor()));
    }

    void draw() override;
private:
    void updateText();
};
