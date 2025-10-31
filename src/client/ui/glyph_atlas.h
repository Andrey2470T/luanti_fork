#pragma once

#include "client/render/atlas.h"
#include <Render/TTFont.h>
#include "gui/GUISkin.h"

struct Glyph : public AtlasTile
{
    wchar_t symbol;

    Glyph(wchar_t _symbol, u32 num)
        : AtlasTile(nullptr, num), symbol(_symbol)
    {}
    Glyph(wchar_t _symbol, u32 num, render::TTFont *font)
        : AtlasTile(font->getGlyphImage(_symbol), num),  symbol(_symbol)
    {}
};

class GlyphAtlas : public Atlas
{
    render::TTFont *font;
    
    u16 slots_count;
    char16_t chars_offset=0;

    u32 dpi;
public:
    GlyphAtlas(u32 num, render::TTFont *ttfont, u32 &offset, u32 _dpi);

    Glyph *getByChar(wchar_t ch) const;
    void fill(u32 num);
    
    void packTiles() override;

    bool readCache(u32 num);
    void saveToCache(u32 num);

    std::string getName(u32 size, u32 num) const override;
};


// Handling fonts caching and creating of the glyph atlases
class FontManager
{
    RenderSystem *rndsys;
    ResourceCache *cache;

    std::map<u64, std::pair<render::TTFont *, std::unique_ptr<AtlasPool>>> fonts;

    std::array<u32, 3> defaultSizes;
public:
    FontManager(RenderSystem *_rndsys, ResourceCache *_cache);

    ~FontManager();

    u32 getScreenDpi() const;
    u32 getDefaultFontSize(render::FontMode mode) const
    {
        return defaultSizes.at((u32)mode);
    }

    render::TTFont *getDefaultFont()
    {
        return getFontOrCreate(render::FontMode::MONO, render::FontStyle::NORMAL);
    }

    render::TTFont *getFont(render::FontMode mode, render::FontStyle style, std::optional<u32> size=std::nullopt) const;
    AtlasPool *getPool(render::FontMode mode, render::FontStyle style, std::optional<u32> size=std::nullopt);

    render::TTFont *getFontOrCreate(render::FontMode mode, render::FontStyle style, std::optional<u32> size=std::nullopt);
    AtlasPool *getPoolOrCreate(render::FontMode mode, render::FontStyle style, std::optional<u32> size=std::nullopt);

    img::Image *drawTextToImage(const std::wstring &text,
        render::FontMode mode, render::FontStyle style, std::optional<u32> size=std::nullopt,
        const img::color8 &color=img::color8(img::PF_RGBA8, 0, 0, 0, 255));
private:
    std::optional<u64> addFont(render::FontMode mode, render::FontStyle style, std::optional<u32> size=std::nullopt);

    void readDefaultFontSizes();
    static void font_sizes_changed(const std::string &name, void *userdata)
    {
        reinterpret_cast<FontManager *>(userdata)->readDefaultFontSizes();
    }
};
