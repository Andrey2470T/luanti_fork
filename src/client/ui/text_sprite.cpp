#include "text_sprite.h"
#include "client/media/resource.h"
#include "irrlicht_gui/GUISkin.h"

UITextSprite::UITextSprite(render::StreamTexture2D *tex, const EnrichedString &text, const rectf &posRect,
    Renderer2D *renderer, ResourceCache *resCache, bool border, bool wordWrap, bool fillBackground)
    : UISprite(tex, renderer, resCache, posRect, posRect, {}, false, true), drawBorder(border),
    wordWrap(wordWrap), drawBackground(fillBackground)
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


