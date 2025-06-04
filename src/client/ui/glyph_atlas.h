#pragma once

#include "client/render/atlas.h"
#include <Render/TTFont.h>

#define MAX_GLYPHS_COUNT 0xFFFF

struct Glyph : public AtlasTile
{
	char16_t symbol;
	img::color8 color;

    Glyph(char16_t _symbol, const img::color8 &_color, u32 num, render::TTFont *font)
        : AtlasTile(font->getGlyphImage(_symbol, _color), num),  symbol(_symbol), color(_color)
    {}
};

class GlyphAtlas : public Atlas
{
    render::TTFont *font;
    
    u16 slots_count;
    char16_t chars_offset=0;
public:
    GlyphAtlas(u32 num, render::TTFont *ttfont, char16_t &offset);

    Glyph *getByChar(wchar_t ch) const;
    void fill(u32 num, const img::color8 &c);
    
    void packTiles() override;
};
