#pragma once

#include "sprite.h"
#include "util/enriched_string.h"
#include "gui/GUIEnums.h"
#include <Render/TTFont.h>
#include <Render/Texture2D.h>
#include "gui/GUISkin.h"
#include "client/render/text.h"

class FontManager;

class UITextSprite : public UISprite
{
    Text text;

    rectf boundRect;
public:
    UITextSprite(FontManager *font_manager, GUISkin *guiskin, ResourceCache *resCache,
        SpriteDrawBatch *drawBatch, std::variant<EnrichedString, std::wstring> text,
        bool border = false, bool wordWrap = false, bool fillBackground = false, u32 depthLevel=0);

    Text &getTextObj()
    {
        return text;
    }
    const Text &getTextObj() const
    {
        return text;
    }

    std::wstring getText() const
    {
        return text.getText();
    }
    void setText(const EnrichedString &textstr)
    {
        text.setText(textstr);
    }
    void setText(const std::wstring &textstr)
    {
        text.setText(textstr);
    }

    u32 getTextWidth() const
    {
        return text.getTextWidth();
    }
    u32 getTextHeight() const
    {
        return text.getTextHeight();
    }
    u32 getLineHeight() const
    {
        return text.getLineHeight();
    }
    v2u getTextSize() const
    {
        return v2u(getTextWidth(), getTextHeight());
    }

    void setBoundRect(const rectf &newRect)
    {
    	if (boundRect != newRect) {
        	boundRect = newRect;
        	text.needsUpdate = true;
    	}
    }

    void appendToBatch() override;
    void updateBatch() override {}
};
