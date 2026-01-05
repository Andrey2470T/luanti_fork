#include "text_sprite.h"
#include "client/media/resource.h"
#include <Utils/String.h>
#include "client/ui/glyph_atlas.h"

UITextSprite::UITextSprite(FontManager *font_manager, GUISkin *guiskin, ResourceCache *resCache,
    SpriteDrawBatch *drawBatch, std::variant<EnrichedString, std::wstring> text,
    bool border, bool wordWrap, bool fillBackground)
    : UISprite(resCache, drawBatch), text(font_manager, guiskin, text, border, wordWrap, fillBackground)
{}

/*void UITextSprite::draw(std::optional<u32> primOffset, std::optional<u32> primCount)
{
    if (!isVisible())
        return;

    renderer->setRenderState(false);
    renderer->setDefaultShader(true, true);
    renderer->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);

    if (clipRect != recti())
        renderer->setClipRect(clipRect);

    u32 rectN = 0;
    if (drawBackground || drawBorder) {
        u32 count = drawBackground && drawBorder ? 2 : 1;
        drawPart(rectN, count);
        rectN += count;
    }

    if (visible && !text.empty()) {
        for (auto &tex_to_charcount : texture_to_charcount_map) {
            renderer->setTexture(tex_to_charcount.first);
            renderer->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);

            drawPart(rectN, tex_to_charcount.second);
            rectN += tex_to_charcount.second;
        }

        renderer->setTexture(nullptr);
    }

    if (clipRect != recti())
        renderer->setClipRect(recti());
}*/

void UITextSprite::updateBatch()
{
    if (!text.needsUpdate)
        return;

    text.needsUpdate = false;

    clear();

    std::unordered_map<render::Texture2D *, u32> texture_to_charcount_map;
    std::unordered_map<render::Texture2D *, std::vector<GlyphPrimitiveParams>> texture_to_glyph_map;

    if (text.isDrawBackground()) {
        auto bg_color = text.getBackgroundColor();
        shape.addRectangle(boundRect, {bg_color, bg_color, bg_color, bg_color});
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

    v2f offset =boundRect.ULC;

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

    std::array<img::color8, 4> arrColors = {color, color, color, color};

    v2f line_offset = offset;
    u32 char_n = 0;

    for (const auto &str : brokenText) {
        for (const wchar_t &ch : str.getString()) {
            auto atlas = fontAtlases->getAtlasByChar(ch);

            if (!atlas)
                continue;

            render::Texture2D *atlas_tex = atlas->getTexture();

            u32 shadowOffset, shadowAlpha;
            font->getShadowParameters(&shadowOffset, &shadowAlpha);

            u32 &charCount = texture_to_charcount_map[atlas_tex];

            if (shadowOffset)
                charCount += 2;
            else
                ++charCount;

            Glyph *glyph = atlas->getGlyphByChar(ch);

            auto &glyphParamsSet = texture_to_glyph_map[atlas_tex];

            rectf glyphPos(line_offset, glyph->size.X, glyph->size.Y);

            GlyphPrimitiveParams glyphparams;
            glyphparams.uv = rectf(
                glyph->pos.X, glyph->pos.Y + glyph->size.Y,
                glyph->pos.X + glyph->size.X, glyph->pos.Y);

            if (!text.isOverrideColorEnabled()) {
                auto textColors = text.getTextColors();

                if (char_n < textColors.size())
                    arrColors = {textColors.at(char_n), textColors.at(char_n), textColors.at(char_n), textColors.at(char_n)};
            }
            if (shadowOffset) {
                glyphparams.pos += v2f(shadowOffset);

                std::array<img::color8, 4> shadowColors = arrColors;
                shadowColors[0].A(shadowAlpha);
                shadowColors[1].A(shadowAlpha);
                shadowColors[2].A(shadowAlpha);
                shadowColors[3].A(shadowAlpha);
                glyphparams.colors = shadowColors;

                glyphParamsSet.push_back(glyphparams);
            }

            glyphparams.pos = glyphPos;
            glyphparams.colors = arrColors;

            glyphParamsSet.push_back(glyphparams);

            line_offset.X += glyph->advance;
            char_n++;
        }

        line_offset.X = offset.X;
        line_offset.Y += lineHeight;
    }

    for (auto &tex_to_glyph : texture_to_glyph_map) {
        for (auto &glyphparams : tex_to_glyph.second)
            shape.addRectangle(glyphparams.pos, glyphparams.colors, glyphparams.uv);
    }

    std::vector<SpriteDrawChunk> chunks;

    for (auto &tex_to_charcount : texture_to_charcount_map)
        chunks.emplace_back(tex_to_charcount.first, clipRect, 0, tex_to_charcount.second*6);

    //drawBatch->addChunks(chunks);
}


