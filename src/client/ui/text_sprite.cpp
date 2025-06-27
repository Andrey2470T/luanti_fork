#include "text_sprite.h"
#include "client/media/resource.h"
#include "client/ui/batcher2d.h"
#include "client/ui/renderer2d.h"
#include <Utils/String.h>
#include "client/ui/glyph_atlas.h"

inline std::vector<UIPrimitiveType> getGlyphs(u32 count, bool background, bool border)
{
    std::vector<UIPrimitiveType> glyphs(count);

    for (u32 i = 0; i < count; i++)
        glyphs[i] = UIPrimitiveType::RECTANGLE;

    return glyphs;
}

UITextSprite::UITextSprite(FontManager *font_manager, const EnrichedString &text,
    Renderer2D *renderer, ResourceCache *resCache, const recti &clip, bool border, bool wordWrap, bool fillBackground)
    : UISprite(getGlyphAtlasTexture(font_manager), renderer, resCache, false, false), drawBorder(border),
    drawBackground(fillBackground),  wordWrap(wordWrap), clipRect(clip)
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

    updateWrappedText();
}

void UITextSprite::setColor(const img::color8 &c)
{
    text.setDefaultColor(c);
    updateWrappedText();
}

void UITextSprite::setBackgroundColor(const img::color8 &c)
{
    text.setBackground(c);
    drawBackground = true;
}

void UITextSprite::enableWordWrap(bool wrap)
{
    if (wordWrap != wrap) {
        wordWrap = wrap;
        updateWrappedText();
    }
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

void UITextSprite::setClipRect(const recti &r)
{
    if (clipText)
        renderer->setClipRect(r);
    else
        renderer->setClipRect(recti());
}

void UITextSprite::draw()
{
    u32 rectN = 0;
    if (drawBackground || drawBorder) {
        renderer->setRenderState(true, false);
        renderer->setUniforms(1.0f, false);

        UISprite::setClipRect(clipRect);

        u32 count = drawBackground && drawBorder ? 2 : 1;
        renderer->drawPrimitives(shape.get(), mesh.get(), rectN, count);
        rectN += count;
    }

    if (visible) {
        renderer->setRenderState(true, true);
        renderer->setUniforms(1.0f, true);
        renderer->setTexture(texture);

        setClipRect(clipRect);

        renderer->drawPrimitives(shape.get(), mesh.get(), rectN, text.size());
    }
}

void UITextSprite::updateBuffer(rectf &&r, FontManager *font_manager)
{
    shape->clear();
    mesh->clear();

    u32 rectN = 0;
    if (drawBackground) {
        ++rectN;
        auto bg_color = getBackgroundColor();
        shape->addRectangle(r, {bg_color, bg_color, bg_color, bg_color});
        reallocateBuffer();
        Batcher2D::appendRectangle(mesh.get(), r, {bg_color, bg_color, bg_color, bg_color}, {v2f(), v2f()});
    }

    if (drawBorder) {
        std::array<img::color8, 4> colors;
        for (u32 i = 0; i < 4; i++) {
            shape->addRectangle(r, colors);
            reallocateBuffer();
            Batcher2D::appendRectangle(mesh.get(), r, colors, {v2f(), v2f()});
        }

        skin->update3DSunkenPane(this, img::color8(), true, false, r, rectN);
    }

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

    auto fontAtlases = font_manager->getPool(font->getMode(), font->getStyle(), font->getCurrentSize());

    if (!fontAtlases)
        return;

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

            rectf glyphUV;
            v2u glyphSize;
            u32 atlasSize;
            for (u32 i = 0; i < fontAtlases->getAtlasCount(); i++) {
                auto curAtlas = dynamic_cast<GlyphAtlas *>(fontAtlases->getAtlas(i));

                Glyph *glyph = curAtlas->getByChar((wchar_t)ch);

                if (!glyph)
                    continue;

                atlasSize = curAtlas->getTextureSize();
                glyphUV = glyph->toUV(atlasSize);
                glyphSize = glyph->size;
            }

            rectf glyphPos(offset, v2f(glyphSize.X, glyphSize.Y));

            u32 shadowOffset, shadowAlpha;
            font->getShadowParameters(&shadowOffset, &shadowAlpha);

            if (shadowOffset) {
                rectf glyphShadowPos(glyphPos.ULC + v2f(shadowOffset), glyphPos.LRC);

                std::array<img::color8, 4> shadowColors = {colors[0], colors[1], colors[2], colors[3]};
                shadowColors[0].A(shadowAlpha);
                shadowColors[1].A(shadowAlpha);
                shadowColors[2].A(shadowAlpha);
                shadowColors[3].A(shadowAlpha);

                shape->addRectangle(glyphShadowPos, shadowColors);
                reallocateBuffer();
                Batcher2D::appendImageRectangle(mesh.get(), v2u(atlasSize), glyphUV, glyphShadowPos, shadowColors, false);
            }

            std::array<img::color8, 4> arrColors = {colors[0], colors[1], colors[2], colors[3]};

            shape->addRectangle(glyphPos, arrColors);
            reallocateBuffer();
            Batcher2D::appendImageRectangle(mesh.get(), v2u(atlasSize), glyphUV, glyphPos, arrColors, false);

            offset.X += advance;
            prevCh = ch;

            // Calculate the glyph offset.
            //s32 offx = Glyphs[n-1].offset.X;
            //s32 offy = (font_metrics.ascender / 64) - Glyphs[n-1].offset.Y;

            // Apply kerning.
            //core::vector2di k = getKerning(currentChar, previousChar);
            //offset.X += k.X;
            //offset.Y += k.Y;

                // Determine rendering information.
                /*SGUITTGlyph& glyph = Glyphs[n-1];
                CGUITTGlyphPage* const page = Glyph_Pages[glyph.glyph_page];
                page->render_positions.push_back(v2i(offset.X + offx, offset.Y + offy));
                page->render_source_rects.push_back(glyph.source_rect);
                const size_t iterPos = iter - utext.begin();
                if (iterPos < colors.size())
                    page->render_colors.push_back(colors[iterPos]);
                else
                    page->render_colors.push_back(img::color8(255,255,255,255));
                Render_Map[glyph.glyph_page] = page;
            }*/

            /*if (n > 0)
            {
                offset.X += getWidthFromCharacter(currentChar);
            }
            else if (fallback != 0)
            {
                // Let the fallback font draw it, this isn't super efficient but hopefully that doesn't matter
                wchar_t l1[] = { (wchar_t) currentChar, 0 };

                if (visible)
                {
                    // Apply kerning.
                    offset += fallback->getKerning(*l1, (wchar_t) previousChar);

                    const u32 current_color = iter - utext.begin();
                    fallback->draw(std::wstring(l1),
                                   recti({offset.X-1, offset.Y-1}, position.LowerRightCorner), // ???
                                   current_color < colors.size() ? colors[current_color] : img::color8(255, 255, 255, 255),
                                   false, false, clip);
                }

                offset.X += fallback->getDimension(l1).Width;
            }

            previousChar = currentChar;
            ++iter;
        }
        CGUITTFont *tmp = static_cast<CGUITTFont*>(font);
        tmp->draw(str,
            r, HAlign == EGUIA_CENTER, VAlign == EGUIA_CENTER,
            (RestrainTextInside ? &AbsoluteClippingRect : NULL));*/
        }


        rc.LRC.Y += height_line;
        rc.ULC.Y += height_line;
    }

    mesh->uploadData();
}
void UITextSprite::updateWrappedText()
{
    brokenText.clear();

    if (text.hasBackground())
        setBackgroundColor(text.getBackground());
    else
        enableDrawBackground(false);

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

render::Texture2D *UITextSprite::getGlyphAtlasTexture(FontManager *manager) const
{
    auto font = getActiveFont();
    auto pool = manager->getPoolOrCreate(font->getMode(), font->getStyle(), font->getCurrentSize());

    return pool->getAtlas(0)->getTexture(); // Beforehand assume each glyph atlas pool contains one atlas each
}


