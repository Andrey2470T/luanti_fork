#include "glyph_atlas.h"
#include <sstream>
#include <Utils/String.h>
#include "settings.h"
#include "client/media/resource.h"

GlyphAtlas::GlyphAtlas(u32 num, render::TTFont *ttfont, char16_t &offset)
    : Atlas(), font(ttfont)
{
    u32 size = font->getCurrentSize();
    std::string prefix = composeAtlasName(font->getMode(), font->getStyle(), size);
        
    u32 tex_size;
        
    if (size <= 21) tex_size = 256;
    else if (size <= 42) tex_size = 512;
    else if (size <= 84) tex_size = 1024;
    else if (size <= 168) tex_size = 2048;
    else tex_size = 4096;

    img::PixelFormat format = img::PF_RGBA8;

    if (font->getMode() == render::FontMode::GRAY)
        format = img::PF_R8;
    createTexture(prefix, num, tex_size, 0, format);
        
    u16 slots = tex_size / size * tex_size / size;
    slots_count = std::min<u16>(MAX_GLYPHS_COUNT-1 - offset, slots);
    chars_offset = offset;
    offset += slots_count;
}

Glyph *GlyphAtlas::getByChar(wchar_t ch) const
{
    auto ch16 = wide_to_utf16(&ch);

    auto it = std::find_if(tiles.begin(), tiles.end(), [&ch16] (const std::unique_ptr<AtlasTile> &tile)
    {
        Glyph *glyph = dynamic_cast<Glyph *>(tile.get());

        return glyph->symbol == ch16[0];
    });

    if (it == tiles.end())
        return nullptr;

    return dynamic_cast<Glyph *>((*it).get());
}

void GlyphAtlas::fill(u32 num)
{
    for (char16_t ch = chars_offset; ch < chars_offset + slots_count; ch++) {
        if (!font->hasGlyph((wchar_t)ch))
            continue;
                
        Glyph *newGlyph = new Glyph(ch, num, font);
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

        tiles[i]->pos = pos;
    }
}

std::string GlyphAtlas::composeAtlasName(render::FontMode mode, render::FontStyle style, u32 size)
{
    std::ostringstream name("TTFontGlyph.");

    switch(mode) {
    case render::FontMode::MONO:
        name << "Mono.";
        break;
    case render::FontMode::GRAY:
        name << "Gray.";
        break;
    case render::FontMode::FALLBACK:
        name << "Fallback.";
        break;
    }

    switch (style) {
    case render::FontStyle::NORMAL:
        name << "Normal.";
        break;
    case render::FontStyle::BOLD:
        name << "Bold.";
        break;
    case render::FontStyle::ITALIC:
        name << "Italic.";
        break;
    case render::FontStyle::STRIKETHROUGH:
        name << "Strikethrough.";
        break;
    case render::FontStyle::UNDERLINE:
        name << "Underline.";
        break;
    }

    name << "Size:" << size << "x" << size;

    return name.str();
}

static std::vector<std::string> settings = {
    "font_size", "font_bold", "font_italic", "font_size_divisible_by",
    "mono_font_size", "mono_font_size_divisible_by",
    "font_shadow", "font_shadow_alpha",
    "font_path", "font_path_bold", "font_path_italic", "font_path_bold_italic",
    "mono_font_path", "mono_font_path_bold", "mono_font_path_italic",
    "mono_font_path_bold_italic",
    "fallback_font_path",
    "dpi_change_notifier", "display_density_factor", "gui_scaling",
};

FontManager::FontManager(ResourceCache *_cache)
    : cache(_cache)
{
    for (auto &name : settings)
        g_settings->registerChangedCallback(name, font_sizes_changed, this);
}

FontManager::~FontManager()
{
    for (auto &p : fonts)
        cache->clearResource<render::TTFont>(ResourceType::FONT, p.second.first, true);

    g_settings->deregisterAllChangedCallbacks(this);
}

render::TTFont *FontManager::getFont(render::FontMode mode, render::FontStyle style, std::optional<u32> size) const
{
    auto it = fonts.find(render::TTFont::hash(size.value(), true, style, mode));

    if (it == fonts.end())
        return nullptr;

    return it->second.first;
}

AtlasPool *FontManager::getPool(render::FontMode mode, render::FontStyle style, std::optional<u32> size)
{
    auto it = fonts.find(render::TTFont::hash(size.value(), true, style, mode));

    if (it == fonts.end())
        return nullptr;

    return it->second.second.get();
}

render::TTFont *FontManager::getFontOrCreate(render::FontMode mode, render::FontStyle style, std::optional<u32> size)
{
    auto font = getFont(mode, style, size);

    if (!font) {
        bool added = addFont(mode, style, size);

        if (added)
            font = fonts[render::TTFont::hash(size.value(), true, style, mode)].first;
    }

    return font;
}

AtlasPool *FontManager::getPoolOrCreate(render::FontMode mode, render::FontStyle style, std::optional<u32> size)
{
    auto pool = getPool(mode, style, size);

    if (!pool) {
        bool added = addFont(mode, style, size);

        if (added)
            pool = fonts[render::TTFont::hash(size.value(), true, style, mode)].second.get();
    }

    return pool;
}

bool FontManager::addFont(render::FontMode mode, render::FontStyle style, std::optional<u32> size)
{
    if (!size.has_value())
        size = defaultSizes[(u8)mode];

    u8 style8 = (u8)style;
    if (mode == render::FontMode::FALLBACK) {
        style8 &= ~(u8)render::FontStyle::BOLD;
        style8 &= ~(u8)render::FontStyle::ITALIC;
    }
    std::string prefix = "";

    if (mode == render::FontMode::MONO)
        prefix = "mono_";

    std::string suffix = "";

    if (style8 & (u8)render::FontStyle::BOLD)
        suffix = "_bold";
    if (style8 & (u8)render::FontStyle::ITALIC)
        suffix += "_italic";

    u16 divisible_by = g_settings->getU16(prefix + "font_size_divisible_by");
    if (divisible_by > 1) {
        size = std::max<u32>(
            std::round((f32)size.value() / divisible_by) * divisible_by, divisible_by);
    }

    u16 font_shadow  = 0;
    u16 font_shadow_alpha = 0;
    g_settings->getU16NoEx(prefix + "font_shadow", font_shadow);
    g_settings->getU16NoEx(prefix + "font_shadow_alpha",
                           font_shadow_alpha);

    std::string path_setting;
    if (mode == render::FontMode::FALLBACK)
        path_setting = "fallback_font_path";
    else
        path_setting = prefix + "font_path" + suffix;

    std::array<std::string, 2> paths = {
        g_settings->get(path_setting),
        Settings::getLayer(SL_DEFAULTS)->get(path_setting)
    };

    bool path_found = false;
    for (auto &path : paths) {
        if (!fs::exists(path))
            continue;

        path_found = true;
        auto font = render::TTFont::load(path, size.value(), 0, true, true, font_shadow, font_shadow_alpha);

        if (!font)
            continue;

        auto font_it = std::find_if(fonts.begin(), fonts.end(), [font] (const auto &p)
        {
            return font == p.second.first;
        });

        if (font_it != fonts.end()) {
            delete font;
            continue;
        }
        u64 hash = render::TTFont::hash(font);

        fonts[hash] = std::pair(font, std::make_unique<AtlasPool>(AtlasType::GLYPH, "", cache, 0, false));
        cache->cacheResource<render::TTFont>(ResourceType::FONT, font);
        fonts[hash].second->buildGlyphAtlas(font);
    }

    if (!path_found) {
        warningstream << "FontManager::addFont() Couldn't find any path to the resource with name " << path_setting << std::endl;
        return false;
    }
    return true;
}

void FontManager::addFontInSkin(GUISkin *skin, render::FontMode mode, render::FontStyle style,
    std::optional<u32> size, GUIDefaultFont which)
{
    bool added = addFont(mode, style, size);

    if (!added)
        return;
    u64 hash = render::TTFont::hash(size.value(), true, style, mode);
    skin->setFont(fonts[hash].first, which);
}

img::Image *FontManager::drawTextToImage(const std::wstring &text,
    render::FontMode mode, render::FontStyle style, std::optional<u32> size, const img::color8 &color)
{
    auto font = getFontOrCreate(mode, style, size);

    return font->drawText(text, color);
}

void FontManager::readDefaultFontSizes()
{
    defaultSizes[0] = std::clamp<u32>((u32)g_settings->getU16("mono_font_size"), 5, 72);
    defaultSizes[1] = std::clamp<u32>((u32)g_settings->getU16("font_size"), 5, 72);
    defaultSizes[2] = std::clamp<u32>((u32)g_settings->getU16("font_size"), 5, 72);
}
