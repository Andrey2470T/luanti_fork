#include "glyph_atlas.h"
#include <Utils/String.h>
#include <Image/ImageLoader.h>
#include "client/media/filecache.h"
#include "client/render/rendersystem.h"
#include "porting.h"
#include "settings.h"
#include "client/media/resource.h"
#include "convert_json.h"
#include <json/json.h>
#include "file.h"
#include <fstream>
#include <Utils/MathFuncs.h>

GlyphAtlas::GlyphAtlas(u32 num, render::TTFont *ttfont, u32 &offset)
    : Atlas(), font(ttfont)
{
    u32 size = font->getCurrentSize();
        
    u32 tex_size;
        
    if (size <= 21) tex_size = 256;
    else if (size <= 42) tex_size = 512;
    else if (size <= 84) tex_size = 1024;
    else if (size <= 168) tex_size = 2048;
    else tex_size = 4096;

    u16 slots = tex_size / size * tex_size / size;
    slots_count = std::min<u16>(font->getGlyphsNum() - offset, slots);
    chars_offset = offset;
    offset += slots_count;

    std::string prefix = getName(tex_size, num);
    createTexture(prefix, tex_size);

    fill(num);
    packTiles();
    drawTiles();
}

Glyph *GlyphAtlas::getGlyphByChar(wchar_t ch)
{
    auto foundGlyph = wchar_to_glyph_mapping[ch];

    if (!foundGlyph)
        return nullptr;

    return foundGlyph;
}

void GlyphAtlas::fill(u32 num)
{
    auto &glyphs = font->getGlyphsSet();

    for (u32 k = chars_offset; k < chars_offset + slots_count; k++) {
        Glyph *newGlyph = new Glyph(glyphs.at(k), num, font);
        addTile(newGlyph);
        wchar_to_glyph_mapping[glyphs.at(k)] = newGlyph;
    }
}
    
void GlyphAtlas::packTiles()
{
	u32 texture_size = texture->getWidth();
    u32 font_size = font->getCurrentSize();
	for (u32 i = 0; i < tiles.size(); i++) {
        auto tile = tiles[i].get();
		v2u pos(
			(i % (texture_size / font_size)) * font_size,
			(i / (texture_size / font_size)) * font_size
		);

        tile->pos = pos;

        if (tile->image && pos.Y + tile->size.Y > texture_size && tile->image) {
            v2u clipPos = tile->image->getClipPos();
            v2u clipSize = tile->image->getClipSize();
            u32 newSizeY = texture_size > pos.Y ? texture_size - pos.Y : 0;

            tile->image->setClipRegion(clipPos.X, clipPos.Y, clipSize.X, newSizeY);
        }

        tile->size.Y -= 2;
	}
}

/*bool GlyphAtlas::readCache(u32 num)
{
    fs::path atlasPath = fs::path(porting::path_cache) / "atlases";
    FileCache atlasCache(atlasPath);

    std::string atlasName = getName(texture->getWidth(), num);

    Json::Value root;
    Json::Value atlasInfo;

    std::ifstream atlasesFile(atlasPath / "atlases.json");

    bool readable = atlasesFile.is_open();

    if (readable) {
        Json::CharReaderBuilder reader;
        std::string errors;
        readable = Json::parseFromStream(reader, atlasesFile, &root, &errors);
        atlasesFile.close();

        if (readable)
            atlasInfo = root[atlasName];
    }

    if (!readable) {
        warningstream << "GlyphAtlas::readCache() failed to read content about " << atlasName <<
            " atlas from atlases.json, creating a new atlas" << std::endl;
        return false;
    }

    for (const auto &tile : atlasInfo["tiles"]) {
        Glyph *glyph = new Glyph(tile["symbol"].asInt(), num);
        glyph->pos = v2u(tile["pos"]["x"].asUInt(), tile["pos"]["y"].asUInt());
        glyph->size = v2u(tile["size"]["x"].asUInt(), tile["size"]["y"].asUInt());

        tiles.emplace_back(glyph);
    }

    for (const auto &hti : atlasInfo["hash_to_index"])
        hash_to_index[hti["hash"].asUInt64()] = hti["index"].asUInt();


    auto imgCache = img::ImageLoader::load(atlasPath / atlasName / ".png");

    if (!imgCache) {
        tiles.clear();
        hash_to_index.clear();
        return false;
    }

    texture = std::make_unique<render::Texture2D>(atlasName,
        imgCache->getWidth(), imgCache->getHeight(), imgCache->getFormat(), 0);

    return true;
}

void GlyphAtlas::saveToCache(u32 num)
{
    fs::path atlasPath = fs::path(porting::path_cache) / "atlases";
    FileCache atlasCache(atlasPath);

    std::string atlasName = getName(texture->getWidth(), num);

    auto imgCache = texture->downloadData();

    img::ImageLoader::save(imgCache.at(0), atlasPath / (atlasName + ".png"));

    Json::Value root;

    std::ifstream atlasesFile(atlasPath / "atlases.json");

    if (atlasesFile.is_open()) {
        Json::CharReaderBuilder reader;
        std::string errors;
        bool parsingSuccessful = Json::parseFromStream(reader, atlasesFile, &root, &errors);
        atlasesFile.close();

        if (!parsingSuccessful) {
            warningstream << "GlyphAtlas::saveToCache() failed to read atlases.json content" << std::endl;
            return;
        }
    }

    Json::Value atlasInfo;

    Json::Value tilesInfo(Json::arrayValue);
    for (const auto &tile : tiles) {
        Glyph *glyph = dynamic_cast<Glyph *>(tile.get());

        Json::Value tileInfo;

        Json::Value tilePos;
        tilePos["x"] = glyph->pos.X;
        tilePos["y"] = glyph->pos.Y;

        tileInfo["pos"] = tilePos;

        Json::Value sizePos;
        sizePos["x"] = glyph->size.X;
        sizePos["y"] = glyph->size.Y;

        tileInfo["size"] = sizePos;
        tileInfo["atlasNum"] = num;
        tileInfo["symbol"] = glyph->symbol;

        tilesInfo.append(tileInfo);
    }

    atlasInfo["tiles"] = tilesInfo;

    Json::Value htisInfo(Json::arrayValue);
    for (const auto &hti : hash_to_index) {
        Json::Value htiInfo;
        htiInfo["hash"] = hti.first;
        htiInfo["index"] = hti.second;

        htisInfo.append(htiInfo);
    }

    atlasInfo["hash_to_index"] = htisInfo;

    root[atlasName] = atlasInfo;

    File::write(atlasPath / "atlases.json", fastWriteJson(root));
}*/

std::string GlyphAtlas::getName(u32 size, u32 num) const
{
    std::ostringstream name("GlyphAtlas_");

    switch(font->getMode()) {
    case render::FontMode::MONO:
        name << "Mono_";
        break;
    case render::FontMode::GRAY:
        name << "Gray_";
        break;
    case render::FontMode::FALLBACK:
        name << "Fallback_";
        break;
    }

    switch (font->getStyle()) {
    case render::FontStyle::NORMAL:
        name << "Normal_";
        break;
    case render::FontStyle::BOLD:
        name << "Bold_";
        break;
    case render::FontStyle::ITALIC:
        name << "Italic_";
        break;
    case render::FontStyle::STRIKETHROUGH:
        name << "Strikethrough_";
        break;
    case render::FontStyle::UNDERLINE:
        name << "Underline_";
        break;
    }

    name << size << "x" << size << "_" << num;

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

FontManager::FontManager(RenderSystem *_rndsys, ResourceCache *_cache)
    : rndsys(_rndsys), cache(_cache)
{
    readDefaultFontSizes();
    for (auto &name : settings)
        g_settings->registerChangedCallback(name, font_sizes_changed, this);
}

FontManager::~FontManager()
{
    for (auto &p : fonts)
        cache->clearResource<render::TTFont>(ResourceType::FONT, p.second.first, true);

    g_settings->deregisterAllChangedCallbacks(this);
}

u32 FontManager::getScreenDpi() const
{
    return rndsys->getWindow()->getScreenDPI();
}

render::TTFont *FontManager::getFont(render::FontMode mode, render::FontStyle style, std::optional<u32> size) const
{
    if (!size.has_value())
        size = defaultSizes[(u8)mode];

    u64 hash = render::TTFont::hash(size.value(), true, style, mode);

    auto it = fonts.find(hash);

    if (it == fonts.end())
        return nullptr;

    return it->second.first;
}

AtlasPool *FontManager::getPool(render::FontMode mode, render::FontStyle style, std::optional<u32> size)
{
    if (!size.has_value())
        size = defaultSizes[(u8)mode];

    auto it = fonts.find(render::TTFont::hash(size.value(), true, style, mode));

    if (it == fonts.end())
        return nullptr;

    return it->second.second.get();
}

render::TTFont *FontManager::getFontOrCreate(render::FontMode mode, render::FontStyle style, std::optional<u32> size)
{
    if (!size.has_value())
        size = defaultSizes[(u8)mode];

    auto font = getFont(mode, style, size);

    if (!font) {
        auto hash = addFont(mode, style, size);

        if (hash.has_value())
            font = fonts[hash.value()].first;
    }

    return font;
}

AtlasPool *FontManager::getPoolOrCreate(render::FontMode mode, render::FontStyle style, std::optional<u32> size)
{
    if (!size.has_value())
        size = defaultSizes[(u8)mode];
    auto pool = getPool(mode, style, size);

    if (!pool) {
        auto hash = addFont(mode, style, size);

        if (hash.has_value())
            pool = fonts[hash.value()].second.get();
    }

    return pool;
}

std::optional<u64> FontManager::addFont(render::FontMode mode, render::FontStyle style, std::optional<u32> size)
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
    u64 hash = 0;

    for (auto &path : paths) {
        if (!fs::exists(path) || path_found)
            continue;

        path_found = true;

        auto font = render::TTFont::load(path, size.value(), 0, (u8)mode, true, font_shadow, font_shadow_alpha);

        if (!font)
            continue;

        hash = render::TTFont::hash(font);

        fonts[hash] = std::pair(font, std::make_unique<AtlasPool>(
            AtlasType::GLYPH, "", cache, 0, false, false));

        cache->cacheResource<render::TTFont>(ResourceType::FONT, font, font->getName());

        fonts[hash].second->buildGlyphAtlas(font);
    }

    if (!path_found) {
        warningstream << "FontManager::addFont() Couldn't find any path to the resource with name " << path_setting << std::endl;
        return std::nullopt;
    }
    return hash;
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
