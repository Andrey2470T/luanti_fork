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

    struct GlyphPrimitiveParams {
        rectf pos;
        std::array<img::color8, 4> colors;
        rectf uv;
    };

    rectf boundRect;
public:
    UITextSprite(FontManager *font_manager, GUISkin *guiskin, ResourceCache *resCache,
        SpriteDrawBatch *drawBatch, std::variant<EnrichedString, std::wstring> text,
        bool border = false, bool wordWrap = false, bool fillBackground = false);

    Text &getText()
    {
        return text;
    }
    const Text &getText() const
    {
        return text;
    }

    void setBoundRect(const rectf &newRect)
    {
        boundRect = newRect;
        text.needsUpdate = true;
    }

    void updateBatch() override;
    //void draw(std::optional<u32> primOffset=std::nullopt, std::optional<u32> primCount=std::nullopt) override;

    //void updateBuffer(rectf &&r);
};
