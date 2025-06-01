#include "glyph_atlas.h"
#include <stringstream>

GlyphAtlas::GlyphAtlas(u32 num, render::TTFont *ttfont, char16_t &offset)
        : font(ttfont)
{
    u32 size = font->getCurrentSize();
    std::ostreamstring prefix("TTFontGlyph.");
    prefix << (u8)font->getMode() << ".";
    prefix << (u8)font->getStyle() << ".";
    prefix << size << ".";
    prefix << num;
        
    u32 tex_size;
        
    if (size <= 21) tex_size = 256;
    else if (size <= 42) tex_size = 512;
    else if (size <= 84) tex_size = 1024;
    else if (size <= 168) tex_size = 2048;
    else tex_size = 4096;
        
    createTexture(prefix.str(), num, tex_size, 0);
        
    u16 slots = tex_size / size * tex_size / size;
    slots_count = std::min(0xFFFF-1 - offset, slots);
    chars_offset = offset;
    offset += slots_count;
}
        
void GlyphAtlas::fill(u32 num, const img::color8 &c)
{
    for (char16_t ch = chars_offset; ch < chars_offset + slots_count; ch++) {
    	if (!font->hasGlyph((wchar_t)ch)
            continue;
                
        Glyph *newGlyph = new Glyph(ch, c, num, font);
        addTile(newGlyph);
    }
}
    
void GlyphAtlas::packTiles()
{
    u32 texture_size = texture->getWidth();
    u32 font_size = font->getCurrentSize();
    for (u32 i = 0; i < tiles.size(); i++) {
        v2u pos(
            (i % (texture_size / font_size)) * font_size,
            (i / (texture_size / font_size)) * font_size
        );
            
        tiles[i].pos = pos;
    }
}