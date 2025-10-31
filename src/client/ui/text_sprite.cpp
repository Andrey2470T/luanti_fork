#include "text_sprite.h"
#include "client/media/resource.h"
#include "client/ui/batcher2d.h"
#include "client/render/renderer.h"
#include <Utils/String.h>
#include "client/ui/glyph_atlas.h"
#include "client/render/renderer.h"

UITextSprite::UITextSprite(FontManager *font_manager, GUISkin *guiskin, const EnrichedString &text,
    Renderer *renderer, ResourceCache *resCache, bool border, bool wordWrap, bool fillBackground)
    : UISprite(nullptr, renderer, resCache, false, false), skin(guiskin), drawBorder(border),
    drawBackground(fillBackground),  wordWrap(wordWrap), mgr(font_manager)
{
    texture = getGlyphAtlasTexture();
    setText(text);
}

void UITextSprite::setOverrideFont(render::TTFont *font)
{
    if (overrideFont == font)
        return;

    overrideFont = font;

    updateWrappedText();

    texture = getGlyphAtlasTexture();
}

void UITextSprite::setOverrideColor(const img::color8 &c)
{
    overrideColor = c;
    overrideColorEnabled = true;
    /*text.setDefaultColor(c);

    if (text.hasBackground())
        setBackgroundColor(text.getBackground());
    else
        enableDrawBackground(false);*/
}

void UITextSprite::setBackgroundColor(const img::color8 &c)
{
    bgColor = c;
    overrideBGColorEnabled = true;
    drawBackground = true;
}

void UITextSprite::enableWordWrap(bool wrap)
{
    if (wordWrap != wrap) {
        wordWrap = wrap;
        updateWrappedText();
    }
}

u32 UITextSprite::getTextWidth() const
{
    auto font = getActiveFont();
    u32 width = 0;

    if (wordWrap) {
        for (auto line : brokenText)
            width = std::max(width, font->getTextWidth(line.getString()));
    }
    else
        width = font->getTextWidth(text.getString());

    return width;
}

u32 UITextSprite::getTextHeight() const
{
    auto font = getActiveFont();

    return font->getLineHeight() * brokenText.size();
}

void UITextSprite::enableRightToLeft(bool rtl)
{
    if (rightToLeft != rtl) {
        rightToLeft = rtl;
        updateWrappedText();
    }
}

void UITextSprite::setAlignment(GUIAlignment horizontal, GUIAlignment vertical)
{
    hAlign = horizontal;
    vAlign = vertical;
}

void UITextSprite::setText(const EnrichedString &_text)
{
    text = _text;
    updateWrappedText();
}

void UITextSprite::draw(std::optional<u32> primOffset, std::optional<u32> primCount)
{
    if (!isVisible())
        return;

    renderer->setRenderState(false);
    renderer->setDefaultShader(true, true);
    renderer->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);

    u32 rectN = 0;
    if (drawBackground || drawBorder) {
        setClipRect(clipRect);

        u32 count = drawBackground && drawBorder ? 2 : 1;
        drawPart(rectN, count);
        rectN += count;
    }

    /*if (visible && !text.empty()) {
        if (clipText)
            setClipRect(clipRect);

        for (auto &tex_to_charcount : texture_to_charcount_map) {
            renderer->setTexture(tex_to_charcount.first);
            renderer->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);

            drawPart(rectN, tex_to_charcount.second);
            rectN += tex_to_charcount.second;
        }
    }*/
}

void UITextSprite::updateBuffer(rectf &&r)
{
    clear();
    texture_to_charcount_map.clear();
    texture_to_glyph_map.clear();

    if (drawBackground) {
        auto bg_color = getBackgroundColor();
        shape->addRectangle(r, {bg_color, bg_color, bg_color, bg_color});
    }

    if (drawBorder)
        skin->add3DSunkenPane(this, img::color8(), true, false, r);

    render::TTFont *font = getActiveFont();

    if (brokenText.empty())
        return;

    rectf rc = r;
    u32 height_line = font->getLineHeight();
    u32 height_total = height_line * brokenText.size();
    if (vAlign == GUIAlignment::Center && wordWrap)
    {
        rc.ULC.Y = r.getCenter().Y - (height_total / 2);
    }
    else if (vAlign == GUIAlignment::LowerRight)
    {
        rc.ULC.Y = rc.LRC.Y - (f32)height_total;
    }
    if (hAlign == GUIAlignment::LowerRight)
    {
        rc.ULC.X = rc.LRC.X - getBrokenTextWidth();
    }

    auto fontAtlases = mgr->getPool(font->getMode(), font->getStyle(), font->getCurrentPixelSize(mgr->getScreenDpi()));

    if (!fontAtlases)
        return;

    auto find_atlas = [fontAtlases] (char16_t ch) -> GlyphAtlas *
    {
        for (u32 i = 0; i < fontAtlases->getAtlasCount(); i++) {
            auto atlas = dynamic_cast<GlyphAtlas *>(fontAtlases->getAtlas(i));

            if (atlas->getByChar((wchar_t)ch))
                return atlas;
        }

        return nullptr;
    };

    for (const auto &str : brokenText) {
        if (hAlign == GUIAlignment::LowerRight)
            rc.ULC.X = r.LRC.X - font->getTextWidth(str.getString());

        auto colors = text.getColors();

        v2u textSize;
        v2f offset = rc.ULC;

        bool hcenter = hAlign == GUIAlignment::Center;
        bool vcenter = vAlign == GUIAlignment::Center;

        // Determine offset positions.
        if (hcenter || vcenter)
        {
            textSize = font->getTextSize(text.getString());

            if (hcenter)
                offset.X = ((rc.getWidth() - textSize.X) / 2.0f) + offset.X;

            if (vcenter)
                offset.Y = ((rc.getHeight() - textSize.Y) / 2.0f) + offset.Y;
        }

        std::u16string str16 = wide_to_utf16(str.getString());

        char16_t prevCh = 0;
        for (const char16_t &ch : str16) {
            if (ch == ' ' || ch == '\r')  // the whitespace is invisible, the carriage is ignored
                continue;

            visible = true;
            bool lineBreak=false;
            if (ch == '\n') // Unix breaks
                lineBreak = true;

            if (lineBreak)
            {
                prevCh = 0;
                offset.Y += font->getFontHeight();
                offset.X = rc.ULC.X;

                if (hcenter)
                    offset.X += (rc.getWidth() - textSize.X) / 2.0f;

                continue;
            }

            // Calculate the glyph offset.
            s32 offx, offy, advance;
            font->getGlyphMetrics((wchar_t)ch, &offx, &offy, &advance);
            offy = font->getFontAscent() - offy;

            offset.X += font->getKerningSizeForTwoChars((wchar_t)ch, (wchar_t)prevCh);
            offset.X += offx;
            offset.Y += offy;

            auto atlas = find_atlas(ch);

            if (!atlas)
                continue;

            render::Texture2D *atlas_tex = atlas->getTexture();

            u32 shadowOffset, shadowAlpha;
            font->getShadowParameters(&shadowOffset, &shadowAlpha);

            auto charcount_it = std::find_if(texture_to_charcount_map.begin(), texture_to_charcount_map.end(),
                [atlas_tex] (const auto &tex_to_charcount_p)
                {
                    return tex_to_charcount_p.first == atlas_tex;
                });

            if (charcount_it == texture_to_charcount_map.end())
                texture_to_charcount_map.emplace_back(atlas_tex, shadowOffset ? 2 : 1);
            else {
                if (shadowOffset)
                    charcount_it->second += 2;
                else
                    ++(charcount_it->second);
            }

            Glyph *glyph = atlas->getByChar((wchar_t)ch);

            auto glyphparams_it = std::find_if(texture_to_glyph_map.begin(), texture_to_glyph_map.end(),
                [atlas_tex] (const auto &tex_to_charcount_p)
                {
                    return tex_to_charcount_p.first == atlas_tex;
                });

            rectf glyphPos(offset, v2f(glyph->size.X, glyph->size.Y));

            GlyphPrimitiveParams glyphparams;
            glyphparams.uv = rectf(v2f(glyph->pos.X, glyph->pos.Y), v2f(glyph->size.X, glyph->size.Y));;

            if (shadowOffset) {
                glyphparams.pos = rectf(glyphPos.ULC + v2f(shadowOffset), glyphPos.LRC);

                std::array<img::color8, 4> shadowColors = {colors[0], colors[1], colors[2], colors[3]};
                shadowColors[0].A(shadowAlpha);
                shadowColors[1].A(shadowAlpha);
                shadowColors[2].A(shadowAlpha);
                shadowColors[3].A(shadowAlpha);
                glyphparams.colors = shadowColors;


                if (glyphparams_it == texture_to_glyph_map.end()) {
                    std::vector<GlyphPrimitiveParams> glyphparams_vec;
                    glyphparams_vec.push_back(glyphparams);
                    texture_to_glyph_map.emplace_back(atlas_tex, glyphparams_vec);
                }
                else
                    glyphparams_it->second.push_back(glyphparams);
            }

            glyphparams.pos = glyphPos;
            std::array<img::color8, 4> arrColors = {colors[0], colors[1], colors[2], colors[3]};
            glyphparams.colors = arrColors;

            if (glyphparams_it == texture_to_glyph_map.end()) {
                std::vector<GlyphPrimitiveParams> glyphparams_vec;
                glyphparams_vec.push_back(glyphparams);
                texture_to_glyph_map.emplace_back(atlas_tex, glyphparams_vec);
            }
            else
                glyphparams_it->second.push_back(glyphparams);

            offset.X += advance;
            prevCh = ch;
        }


        rc.LRC.Y += height_line;
        rc.ULC.Y += height_line;
    }

    for (auto &tex_to_glyph : texture_to_glyph_map) {
        for (auto &glyphparams : tex_to_glyph.second)
            shape->addRectangle(glyphparams.pos, glyphparams.colors, glyphparams.uv);
    }

    rebuildMesh();
}

void UITextSprite::updateWrappedText()
{
    brokenText.clear();

    if (!wordWrap) {
        brokenText.push_back(text);
        return;
    }

    // Update word wrap
    render::TTFont* font = getActiveFont();
    if (!font)
        return;

    EnrichedString line;
    EnrichedString word;
    EnrichedString whitespace;
    u32 size = text.size();
    u32 length = 0;
    u32 elWidth = shape->getMaxArea().getWidth();
    if (drawBorder)
        elWidth -= 2*skin->getSize(GUIDefaultSize::TextDistanceX);
    wchar_t c;

    if (!rightToLeft)
    {
        // regular (left-to-right)
        for (u32 i=0; i<size; ++i)
        {
            c = text.getString()[i];
            bool lineBreak = false;

            if (c == L'\r') // Mac or Windows breaks
            {
                lineBreak = true;
                c = '\0';
            }
            else if (c == L'\n') // Unix breaks
            {
                lineBreak = true;
                c = '\0';
            }

            bool isWhitespace = (c == L' ' || c == 0);
            if ( !isWhitespace )
            {
                // part of a word
                //word += c;
                word.addChar(text, i);
            }

            if ( isWhitespace || i == (size-1))
            {
                if (word.size())
                {
                    // here comes the next whitespace, look if
                    // we must break the last word to the next line.
                    const u32 whitelgth = font->getTextWidth(whitespace.getString());
                    const u32 wordlgth = font->getTextWidth(word.getString());

                    if (wordlgth > elWidth)
                    {
                        // This word is too long to fit in the available space, look for
                        // the Unicode Soft HYphen (SHY / 00AD) character for a place to
                        // break the word at
                        int where = std::wstring(word.c_str()).find_first_of( wchar_t(0x00AD) );
                        if (where != -1)
                        {
                            EnrichedString first = word.substr(0, where);
                            EnrichedString second = word.substr(where, word.size() - where);
                            first.addCharNoColor(L'-');
                            brokenText.push_back(line + first);
                            const u32 secondLength = font->getTextWidth(second.getString());

                            length = secondLength;
                            line = second;
                        }
                        else
                        {
                            // No soft hyphen found, so there's nothing more we can do
                            // break to next line
                            if (length)
                                brokenText.push_back(line);
                            length = wordlgth;
                            line = word;
                        }
                    }
                    else if (length && (length + wordlgth + whitelgth > elWidth))
                    {
                        // break to next line
                        brokenText.push_back(line);
                        length = wordlgth;
                        line = word;
                    }
                    else
                    {
                        // add word to line
                        line += whitespace;
                        line += word;
                        length += whitelgth + wordlgth;
                    }

                    word.clear();
                    whitespace.clear();
                }

                if ( isWhitespace && c != 0)
                {
                    whitespace.addChar(text, i);
                }

                // compute line break
                if (lineBreak)
                {
                    line += whitespace;
                    line += word;
                    brokenText.push_back(line);
                    line.clear();
                    word.clear();
                    whitespace.clear();
                    length = 0;
                }
            }
        }

        line += whitespace;
        line += word;
        brokenText.push_back(line);
    }
    else
    {
        // right-to-left
        for (u32 i=size; i>=0; --i)
        {
            c = text.getString()[i];
            bool lineBreak = false;

            if (c == L'\r') // Mac or Windows breaks
            {
                lineBreak = true;
                c = '\0';
            }
            else if (c == L'\n') // Unix breaks
            {
                lineBreak = true;
                c = '\0';
            }

            if (c==L' ' || c==0 || i==0)
            {
                if (word.size())
                {
                    // here comes the next whitespace, look if
                    // we must break the last word to the next line.
                    const u32 whitelgth = font->getTextWidth(whitespace.getString());
                    const u32 wordlgth = font->getTextWidth(word.getString());

                    if (length && (length + wordlgth + whitelgth > elWidth))
                    {
                        // break to next line
                        brokenText.push_back(line);
                        length = wordlgth;
                        line = word;
                    }
                    else
                    {
                        // add word to line
                        line = whitespace + line;
                        line = word + line;
                        length += whitelgth + wordlgth;
                    }

                    word.clear();
                    whitespace.clear();
                }

                if (c != 0)
                    whitespace = text.substr(i, 1) + whitespace;

                // compute line break
                if (lineBreak)
                {
                    line = whitespace + line;
                    line = word + line;
                    brokenText.push_back(line);
                    line.clear();
                    word.clear();
                    whitespace.clear();
                    length = 0;
                }
            }
            else
            {
                // yippee this is a word..
                word = text.substr(i, 1) + word;
            }
        }

        line = whitespace + line;
        line = word + line;
        brokenText.push_back(line);
    }
}

u32 UITextSprite::getBrokenTextWidth() const
{
    u32 w = 0;

    render::TTFont *font = getActiveFont();
    for (auto &line : brokenText) {
        u32 lineW = font->getTextWidth(line.getString());

        w = std::max(w, lineW);
    }

    return w;
}

render::Texture2D *UITextSprite::getGlyphAtlasTexture() const
{
    auto font = getActiveFont();
    auto pool = mgr->getPoolOrCreate(font->getMode(), font->getStyle(), font->getCurrentPixelSize(mgr->getScreenDpi()));

    return pool->getAtlas(0)->getTexture(); // Beforehand assume each glyph atlas pool contains one atlas each
}


