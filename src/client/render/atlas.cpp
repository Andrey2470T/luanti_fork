#include "atlas.h"
#include "rectpack2d_atlas.h"
#include "client/ui/glyph_atlas.h"
#include "client/media/resource.h"

rectf AtlasTile::toUV(u32 atlasSize) const
{
    return rectf(
        (f32)pos.X / atlasSize,
        (f32)pos.Y / atlasSize,
        (f32)(pos.X + size.X) / atlasSize,
        (f32)(pos.Y + size.Y) / atlasSize
        );
}


void Atlas::createTexture(const std::string &name, u32 num, u32 size, u8 maxMipLevel, img::PixelFormat format)
{
    std::string texName = name + "Atlas";
    texName += num;

    render::TextureSettings settings;
    settings.isRenderTarget = false;
    settings.hasMipMaps = maxMipLevel > 0 ? true : false;
    settings.maxMipLevel = maxMipLevel;

    texture = std::make_unique<render::StreamTexture2D>(
        texName, size, size, format, settings);
}

bool Atlas::addTile(const AtlasTile *tile)
{
    size_t tileHash = tile->hash();

    auto it = hash_to_index.find(tileHash);

    if (it != hash_to_index.end())
        return false;

    tiles.emplace_back(std::move(tile));
    hash_to_index[tileHash] = tiles.size();
    markDirty(tiles.size());

    return true;
}

AtlasTile *Atlas::getTile(u32 i) const
{
    if (i > tiles.size()-1)
        return nullptr;

    return tiles.at(i).get();
}

void Atlas::markDirty(u32 i)
{
    auto it = std::find(dirty_tiles.begin(), dirty_tiles.end(), i);

    if (it != dirty_tiles.end())
        return;

    dirty_tiles.push_back(i);
}

void Atlas::drawTiles()
{
    if (!texture || dirty_tiles.empty())
        return;

    for (u32 dirty_i : dirty_tiles) {
        auto tile = getTile(dirty_i);

        if (!tile)
            continue;

        texture->uploadSubData(tile->pos.X, tile->pos.Y, tile->image);
    }

    texture->bind();
    texture->flush();
    texture->unbind();

    if (texture->hasMipMaps())
        texture->regenerateMipMaps();

    dirty_tiles.clear();
}

bool Atlas::operator==(const Atlas *other) const
{
    if (getTilesCount() != other->getTilesCount())
        return false;

    bool equal = true;

    for (u32 i = 0; i < tiles.size(); i++) {
        if (tiles[i].get() != other->getTile(i)) {
            equal = false;
            break;
        }
    }

    return equal;
}


AtlasPool::~AtlasPool()
{
    for (auto &atlas : atlases)
        cache->clearResource<Atlas>(ResourceType::ATLAS, atlas);

    for (auto &tile : images)
        cache->clearResource<img::Image>(ResourceType::IMAGE, tile);
}

Atlas *AtlasPool::getAtlas(u32 i) const
{
    if (i > atlases.size()-1)
        return nullptr;

    return atlases.at(i);
}

Atlas *AtlasPool::getAtlasByTile(img::Image *tile, bool force_add)
{
    for (u32 i = 0; i < atlases.size(); i++) {
        auto atlas = atlases.at(i);
        for (u32 j = 0; j < atlas->getTilesCount(); j++) {
            auto atlasTile = atlas->getTile(j);

            if (atlasTile->image == tile)
                return atlas;
        }
    }

    if (force_add) {
        forceAddTile(tile);
        return atlases.back();
    }
    return nullptr;
}

img::Image *AtlasPool::addTile(const std::string &name)
{
    if (type != AtlasType::RECTPACK2D)
        return nullptr;
    auto img = cache->getOrLoad<img::Image>(ResourceType::IMAGE, name)->data.get();
    auto imgIt = std::find(images.begin(), images.end(), img);

    // Add only unique tiles
    if (imgIt != images.end())
        return *imgIt;

    // Don't allow to add new tiles if the atlas building was before
    if (!atlases.empty())
        return nullptr;

    images.emplace_back(img);

    return img;
}

img::Image *AtlasPool::addAnimatedTile(const std::string &name, u32 length, u32 count)
{
    if (type != AtlasType::RECTPACK2D)
        return nullptr;
    auto img = addTile(name);

    if (!img)
        return nullptr;

    animatedImages[images.size()-1] = std::make_pair(length, count);

    return img;
}

rectf AtlasPool::getTileUV(img::Image *tile, bool force_add)
{
    if (type != AtlasType::RECTPACK2D)
        return rectf();

    for (u32 i = 0; i < atlases.size(); i++) {
        auto atlas = atlases.at(i);
        for (u32 j = 0; j < atlas->getTilesCount(); j++) {
            auto atlasTile = atlas->getTile(j);

            if (atlasTile->image != tile)
                continue;

            return atlasTile->toUV(atlas->getTextureSize());
        }
    }

    if (force_add) {
        forceAddTile(tile);
        auto lastAtlas = atlases.back();
        return lastAtlas->getTile(lastAtlas->getTilesCount()-1)->toUV(lastAtlas->getTextureSize());
    }
    return rectf();
}

void AtlasPool::buildRectpack2DAtlas()
{
    if (type != AtlasType::RECTPACK2D || images.empty())
        return;

    static u32 atlasNum = 0;
    static u32 startImg = 0;

    Rectpack2DAtlas *atlas = new Rectpack2DAtlas(cache, prefixName, atlasNum, maxTextureSize, filtered, images, animatedImages, startImg);
    cache->cacheResource<Atlas>(ResourceType::ATLAS, atlas);

    atlases.push_back(atlas);

    if (startImg <= images.size()-1) {
        ++atlasNum;
        buildRectpack2DAtlas();
    }
}

void AtlasPool::buildGlyphAtlas(render::TTFont *ttfont)
{
    if (type != AtlasType::GLYPH)
        return;

    static u32 atlasNum = 0;
    static char16_t glyphOffset = 0;

    GlyphAtlas *atlas = new GlyphAtlas(atlasNum, ttfont, glyphOffset);
    cache->cacheResource<Atlas>(ResourceType::ATLAS, atlas);

    atlases.push_back(atlas);

    if (glyphOffset < MAX_GLYPHS_COUNT-1) {
        ++atlasNum;
        buildGlyphAtlas(ttfont);
    }
}

void AtlasPool::updateAnimatedTiles(f32 time)
{
    if (type != AtlasType::RECTPACK2D)
        return;

    for (u32 i = 0; i < atlases.size(); i++) {
        auto atlas = dynamic_cast<Rectpack2DAtlas*>(atlases.at(i));
        atlas->updateAnimatedTiles(time);
    }
}

void AtlasPool::forceAddTile(img::Image *img)
{
    if (type == AtlasType::RECTPACK2D) {
        auto rectpackAtlas = dynamic_cast<Rectpack2DAtlas *>(atlases.back());
        u32 lastAtlasN = atlases.size()-1;

        if (!rectpackAtlas->packSingleTile(img, lastAtlasN)) {
            Rectpack2DAtlas *atlas = new Rectpack2DAtlas(cache, prefixName, lastAtlasN+1, maxTextureSize, img, filtered);
            cache->cacheResource<Atlas>(ResourceType::ATLAS, atlas);

            atlases.push_back(atlas);
        }

    }
}
