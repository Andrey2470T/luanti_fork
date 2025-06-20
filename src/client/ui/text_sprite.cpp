#include "text_sprite.h"
#include "client/media/resource.h"
#include "irrlicht_gui/GUISkin.h"

inline std::vector<UIPrimitiveType> getGlyphs(u32 count)
{
    std::vector<UIPrimitiveType> glyphs(count);

    for (u32 i = 0; i < count; i++)
        glyphs[i] = UIPrimitiveType::RECTANGLE;

    return glyphs;
}

UITextSprite::UITextSprite(render::StreamTexture2D *tex, const EnrichedString &text,
    Renderer2D *renderer, ResourceCache *resCache, bool border, bool wordWrap, bool fillBackground)
    : UISprite(tex, renderer, resCache, getGlyphs(text.size()), true), drawBorder(border),
    drawBackground(fillBackground),  wordWrap(wordWrap)
{
    setText(text);
}

UITextSprite::~UITextSprite()
{
    if (overrideFont)
        cache->clearResource<render::TTFont>(ResourceType::FONT, overrideFont);
}

void UITextSprite::setOverrideFont(render::TTFont *font)
{
    if (overrideFont)
        cache->clearResource<render::TTFont>(ResourceType::FONT, overrideFont);

    overrideFont = font;
    cache->cacheResource<render::TTFont>(ResourceType::FONT, overrideFont);
}

render::TTFont *UITextSprite::getActiveFont() const
{
    if (overrideFont)
        return overrideFont;

    return skin->getFont();
}


