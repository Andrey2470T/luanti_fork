#include "text_sprite.h"
#include "client/media/resource.h"
#include <Utils/String.h>
#include "client/ui/glyph_atlas.h"

UITextSprite::UITextSprite(FontManager *font_manager, GUISkin *guiskin, ResourceCache *resCache,
    SpriteDrawBatch *drawBatch, std::variant<EnrichedString, std::wstring> text,
    bool border, bool wordWrap, bool fillBackground, u32 depthLevel)
    : UISprite(resCache, drawBatch, depthLevel), text(font_manager, guiskin, text, border, wordWrap, fillBackground)
{}

void UITextSprite::appendToBatch()
{
    if (!text.needsUpdate && !changed)
        return;

    changed = false;

    u32 lastRectsCount = shape.getPrimitiveCount();

    text.updateText(shape.getMaxArea());

    clear();

    if (text.isDrawBackground()) {
        auto bg_color = text.getBackgroundColor();
        shape.addRectangle(boundRect, bg_color);
    }

    auto skin = text.getSkin();
    if (text.isDrawBorder())
        skin->add3DSunkenPane(this, img::color8(), true, false, boundRect);

    render::TTFont *font = text.getActiveFont();

    auto brokenText = text.getBrokenText();
    if (brokenText.empty())
        return;

    auto fontMgr = text.getFontManager();
    auto fontAtlases = fontMgr->getPool(font->getMode(), font->getStyle(), font->getCurrentSize());

    if (!fontAtlases)
        return;

    v2f offset = boundRect.ULC;

    GUIAlignment hAlign, vAlign;
    text.getAlignment(hAlign, vAlign);

    v2u textSize = text.getTextSize();
    u32 lineHeight = text.getLineHeight();

    if (hAlign == GUIAlignment::Center)
        offset.X += (boundRect.getWidth() - textSize.X) / 2.0f;
    else if (hAlign == GUIAlignment::LowerRight)
        offset.X += boundRect.getWidth() - textSize.X;

    if (vAlign == GUIAlignment::Center)
        offset.Y += (boundRect.getHeight() - textSize.Y) / 2.0f;
    else if (vAlign == GUIAlignment::LowerRight)
        offset.Y += boundRect.getHeight() - textSize.Y;

    auto color = text.getActiveColor();

    RectColors arrColors = color;

    v2f line_offset = offset;
    u32 char_n = 0;

    std::vector<SpriteDrawChunk> chunks;

    for (const auto &str : brokenText) {
        for (const wchar_t &ch : str.getString()) {
            auto atlas = fontAtlases->getAtlasByChar(ch);

            if (!atlas) {
                char_n++;
                continue;
            }

            u32 shadowOffset, shadowAlpha;
            font->getShadowParameters(&shadowOffset, &shadowAlpha);

            Glyph *glyph = atlas->getGlyphByChar(ch);

            rectf glyphPos(line_offset, glyph->size.X, glyph->size.Y);

            rectf glyphUV = rectf(
                glyph->pos.X, glyph->pos.Y + glyph->size.Y,
                glyph->pos.X + glyph->size.X, glyph->pos.Y);

            if (!text.isOverrideColorEnabled()) {
                auto textColors = text.getTextColors();

                if (char_n < textColors.size())
                    arrColors = textColors.at(char_n);
            }
            if (shadowOffset) {
                rectf shadowGlyphPos = glyphPos;
                shadowGlyphPos += v2f(shadowOffset);

                RectColors shadowColors = arrColors;
                shadowColors[0].A(shadowAlpha);
                shadowColors[1].A(shadowAlpha);
                shadowColors[2].A(shadowAlpha);
                shadowColors[3].A(shadowAlpha);

                shape.addRectangle(shadowGlyphPos, shadowColors, glyphUV);
            }

            shape.addRectangle(glyphPos, arrColors, glyphUV);

            chunks.emplace_back(atlas->getTexture(), clipRect, shadowOffset ? 2 : 1);

            line_offset.X += glyph->advance;
            char_n++;
        }

        line_offset.X = offset.X;
        line_offset.Y += lineHeight;
    }

    u32 curRectsCount = shape.getPrimitiveCount();

    if (lastRectsCount == curRectsCount)
        shape.changeFlag = UIShape::ChangeFlags::COUNT_CHANGED_NOREBUILD;

    drawBatch->addSpriteChunks(this, chunks);
}
