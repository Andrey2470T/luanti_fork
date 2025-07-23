#pragma once

#include "client/render/atlas.h"
#include <Render/TTFont.h>
#include "irrlicht_gui/GUISkin.h"

#define MAX_GLYPHS_COUNT 0xFFFF

struct Glyph : public AtlasTile
{
	char16_t symbol;

    Glyph(char16_t _symbol, u32 num, render::TTFont *font)
        : AtlasTile(font->getGlyphImage(_symbol), num),  symbol(_symbol)
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
    void fill(u32 num);
    
    void packTiles() override;
private:
    std::string composeAtlasName(render::FontMode mode, render::FontStyle style, u32 size);
};


// Handling fonts caching and creating of the glyph atlases
class FontManager
{
    ResourceCache *cache;

    std::map<u64, std::pair<render::TTFont *, std::unique_ptr<AtlasPool>>> fonts;

    std::array<u32, 3> defaultSizes;
public:
    FontManager(ResourceCache *_cache);

    ~FontManager();

    u32 getDefaultFontSize(render::FontMode mode) const
    {
        return defaultSizes.at((u32)mode);
    }

    render::TTFont *getFont(render::FontMode mode, render::FontStyle style, std::optional<u32> size) const;
    AtlasPool *getPool(render::FontMode mode, render::FontStyle style, std::optional<u32> size);

    render::TTFont *getFontOrCreate(render::FontMode mode, render::FontStyle style, std::optional<u32> size);
    AtlasPool *getPoolOrCreate(render::FontMode mode, render::FontStyle style, std::optional<u32> size);

    void addFont(render::FontMode mode, render::FontStyle style, std::optional<u32> size);
    void addFontInSkin(GUISkin *skin, render::FontMode mode, render::FontStyle style,
        std::optional<u32> size, GUIDefaultFont which=GUIDefaultFont::Default);

    img::Image *drawTextToImage(const std::wstring &text,
        render::FontMode mode, render::FontStyle style, std::optional<u32> size, const img::color8 &color=img::color8(img::PF_RGBA8, 0, 0, 0, 255));
private:
    void readDefaultFontSizes();
    static void font_sizes_changed(const std::string &name, void *userdata)
    {
        reinterpret_cast<FontManager *>(userdata)->readDefaultFontSizes();
    }
};
