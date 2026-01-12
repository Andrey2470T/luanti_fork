#include "text.h"

Text::Text(
    FontManager *font_manager, GUISkin *guiskin,
    std::variant<EnrichedString, std::wstring> text,
    bool border, bool wordWrap, bool fillBackground)
        : skin(guiskin), drawBorder(border), drawBackground(fillBackground),  wordWrap(wordWrap), mgr(font_manager)
{
    if (std::holds_alternative<EnrichedString>(text))
        setText(std::get<EnrichedString>(text));
    else
        setText(std::get<std::wstring>(text));
}

void Text::setOverrideFont(render::TTFont *font)
{
    if (overrideFont == font)
        return;

    overrideFont = font;

    needsUpdate = true;
}

void Text::setOverrideColor(const img::color8 &c)
{
    if (overrideColorEnabled && overrideColor == c)
        return;

    overrideColor = c;
    overrideColorEnabled = true;

    needsUpdate = true;
}

void Text::setBackgroundColor(const img::color8 &c)
{
    if (overrideBGColorEnabled && bgColor == c)
        return;

    bgColor = c;
    overrideBGColorEnabled = true;
    drawBackground = true;

    needsUpdate = true;
}

void Text::enableWordWrap(bool wrap)
{
    if (wordWrap != wrap) {
        wordWrap = wrap;
        needsUpdate = true;
    }
}

void Text::enableRightToLeft(bool rtl)
{
    if (rightToLeft != rtl) {
        rightToLeft = rtl;
        needsUpdate = true;
    }
}

void Text::setAlignment(GUIAlignment horizontal, GUIAlignment vertical)
{
    if (hAlign == horizontal && vAlign == vertical)
        return;

    hAlign = horizontal;
    vAlign = vertical;

    needsUpdate = true;
}

void Text::setText(const EnrichedString &_text)
{
    if (text == _text)
        return;

    text = _text;
    needsUpdate = true;
}

void Text::updateText(std::optional<rectf> clipRect)
{
    if (!needsUpdate)
        return;

    brokenText.clear();

    // Update word wrap
    render::TTFont* font = getActiveFont();
    if (!font)
        return;

    EnrichedString line;
    EnrichedString word;
    EnrichedString whitespace;
    u32 size = text.size();
    u32 length = 0;
    u32 elWidth = clipRect.has_value() ? clipRect->getWidth() : textWidth;
    if (drawBorder)
        elWidth -= 2*skin->getSize(GUIDefaultSize::TextDistanceX);

    if (!wordWrap)
        elWidth = (u32)-1;

    wchar_t c;

    if (!rightToLeft)
    {
        // regular (left-to-right)
        for (u32 i=0; i<size; ++i)
        {
            c = text.getString()[i];
            bool lineBreak = false;

            if (c == L'\r' || c == L'\n') // Line breaks (always processed)
            {
                lineBreak = true;
                c = '\0';
            }

            //bool isWhitespace = (c == L' ' || c == 0);
            if ( c != 0 )
            {
                // part of a word
                //word += c;
                word.addChar(text, i);
            }

            if ( c == 0 || i == (size-1))
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

                /*if ( lineBreak )
                {
                    whitespace.addChar(text, i);
                }*/

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

            if (c == L'\r' || c == L'\n') // Mac, Windows orUnix breaks
            {
                lineBreak = true;
                c = '\0';
            }

            if (c == 0 || i==0)
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

    if (wordWrap) {
        for (auto line : brokenText)
            textWidth = std::max(textWidth, font->getTextWidth(line.getString()));
    }
    else
        textWidth = font->getTextWidth(text.getString());

    lineHeight = font->getLineHeight();
    textHeight = lineHeight * brokenText.size();

    needsUpdate = true;
}
