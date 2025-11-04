#include "atlas.h"
#include "Core/TimeCounter.h"
#include "Image/ImageLoader.h"
#include "rectpack2d_atlas.h"
#include "client/ui/glyph_atlas.h"
#include "client/media/resource.h"
#include "client/mesh/meshoperations.h"

rectf AtlasTile::toUV(u32 atlasSize) const
{
    return rectf(
        (f32)pos.X / atlasSize,
        (f32)(pos.Y + size.Y) / atlasSize,
        (f32)(pos.X + size.X) / atlasSize,
        (f32)pos.Y / atlasSize
        );
}


void Atlas::createTexture(const std::string &name, u32 size, u8 maxMipLevel)
{
    texture = std::make_unique<render::Texture2D>(
        name, size, size, img::PF_RGBA8, maxMipLevel);
}

bool Atlas::addTile(AtlasTile *tile)
{
    size_t tileHash = tile->hash();

    auto it = hash_to_index.find(tileHash);

    if (it != hash_to_index.end())
        return false;

    tiles.emplace_back(tile);
    hash_to_index[tileHash] = tiles.size() - 1;
    markDirty(tiles.size() - 1);

    return true;
}

AtlasTile *Atlas::getTile(u32 i) const
{
    if (i > tiles.size()-1)
        return nullptr;

    return tiles.at(i).get();
}

AtlasTile *Atlas::getTileByImage(img::Image *img) const
{
    auto find_tile = std::find_if(tiles.begin(), tiles.end(), [img] (const std::unique_ptr<AtlasTile> &tile)
    {
        return img == tile->image;
    });

    if (find_tile == tiles.end())
        return nullptr;

    return find_tile->get();
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

		if (!tile->image)
			continue;

		texture->uploadSubData(tile->pos.X, tile->pos.Y, tile->image);
	}

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
    u32 atlasCounter = 0;
    for (auto &atlas : atlases) {
        auto atlasImg = atlas->getTexture()->downloadData().at(0);
        img::ImageLoader::save(atlasImg,
            "/home/andrey/minetests/luanti_fork/cache/atlases/" + atlas->getName(atlas->getTextureSize(), atlasCounter++) + ".png");
        cache->clearResource<Atlas>(ResourceType::ATLAS, atlas, true);
    }

    for (auto &tile : images)
        cache->clearResource<img::Image>(ResourceType::IMAGE, tile, true);
}

Atlas *AtlasPool::getAtlas(u32 i) const
{
    if (i > atlases.size()-1)
        return nullptr;

    return atlases.at(i);
}

Atlas *AtlasPool::getAtlasByTile(img::Image *tile, bool force_add, std::optional<AtlasTileAnim> anim)
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
        forceAddTile(tile, anim);
        return atlases.back();
    }
    return nullptr;
}

AtlasTile *AtlasPool::getTileByImage(img::Image *tile)
{
    auto atlas = getAtlasByTile(tile);

    if (!atlas)
        return nullptr;

    return atlas->getTileByImage(tile);
}

AnimatedAtlasTile *AtlasPool::getAnimatedTileByImage(img::Image *tile)
{
    auto atlas_tile = getTileByImage(tile);

    if (!atlas_tile)
        return nullptr;

    return dynamic_cast<AnimatedAtlasTile *>(atlas_tile);
}

img::Image *AtlasPool::addTile(const std::string &name)
{
    if (type != AtlasType::RECTPACK2D)
        return nullptr;
    core::InfoStream << "addTile, time: " << TimeCounter::getRealTime() << " \n";
    auto img = cache->getOrLoad<img::Image>(
        ResourceType::IMAGE, name, apply_modifiers, apply_modifiers, true);

    core::InfoStream << "addTile: " << name << ", " << img->getSize() << ", time: " << TimeCounter::getRealTime() << "\n";
    auto imgIt = std::find(images.begin(), images.end(), img);

    // Add only unique tiles
    if (imgIt != images.end())
        return *imgIt;

    images.emplace_back(img);
    core::InfoStream << "addTile end, time: " << TimeCounter::getRealTime() << " \n";

    return img;
}

img::Image *AtlasPool::addAnimatedTile(const std::string &name, AtlasTileAnim anim)
{
    if (type != AtlasType::RECTPACK2D)
        return nullptr;
    auto img = addTile(name);

    if (!img)
        return nullptr;

    animatedImages[images.size()-1] = anim;

    return img;
}

rectf AtlasPool::getTileRect(img::Image *tile, bool toUV, bool force_add, std::optional<AtlasTileAnim> anim)
{
    if (type != AtlasType::RECTPACK2D)
        return rectf();

    for (u32 i = 0; i < atlases.size(); i++) {
        auto atlas = atlases.at(i);
        for (u32 j = 0; j < atlas->getTilesCount(); j++) {
            auto atlasTile = atlas->getTile(j);

            if (atlasTile->image != tile)
                continue;

            if (toUV)
                return atlasTile->toUV(atlas->getTextureSize());
            else {
                v2f ulc_f(atlasTile->pos.X, atlasTile->pos.Y + atlasTile->size.Y);
                v2f lrc_f(atlasTile->pos.X + atlasTile->size.X, atlasTile->pos.Y);
                return rectf(ulc_f, lrc_f);
            }
        }
    }

    if (force_add) {
        forceAddTile(tile, anim);
        auto lastAtlas = atlases.back();

        auto lastTile = lastAtlas->getTile(lastAtlas->getTilesCount()-1);
        if (toUV)
            return lastTile->toUV(lastAtlas->getTextureSize());
        else {
            v2f ulc_f(lastTile->pos.X, lastTile->pos.Y + lastTile->size.Y);
            v2f lrc_f(lastTile->pos.X + lastTile->size.X, lastTile->pos.Y);
            return rectf(ulc_f, lrc_f);
        }
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
    cache->cacheResource<Atlas>(ResourceType::ATLAS, atlas, atlas->getName(atlas->getTextureSize(), atlasNum));

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
    static u32 glyphOffset = 0;

    GlyphAtlas *atlas = new GlyphAtlas(atlasNum, ttfont, glyphOffset);
    cache->cacheResource<Atlas>(ResourceType::ATLAS, atlas, atlas->getName(atlas->getTextureSize(), atlasNum));

    atlases.push_back(atlas);

    if (glyphOffset < ttfont->getGlyphsNum()) {
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

void AtlasPool::updateMeshUVs(MeshBuffer *buffer, u32 start_index, u32 index_count, img::Image *tile,
    std::optional<img::Image *> oldTile, bool force_add, std::optional<AtlasTileAnim> anim)
{
    std::optional<u32> oldAtlasSize;
    std::optional<rectf> oldTileRect;

    if (oldTile.has_value()) {
        auto prev_atlas = getAtlasByTile(oldTile.value());
        oldAtlasSize = prev_atlas->getTextureSize();
        oldTileRect = getTileRect(oldTile.value());
    }

    auto atlas = getAtlasByTile(tile, force_add, anim);
    u32 newAtlasSize = atlas->getTextureSize();
    rectf newTileRect = getTileRect(tile);

    MeshOperations::recalculateMeshAtlasUVs(buffer, start_index, index_count,
        newAtlasSize, newTileRect, oldAtlasSize, oldTileRect);
}

void AtlasPool::updateAllMeshUVs(MeshBuffer *buffer, img::Image *tile,
    std::optional<img::Image *> oldTile, bool force_add, std::optional<AtlasTileAnim> anim)
{
    updateMeshUVs(buffer, 0, buffer->getIndexCount(), tile, oldTile, force_add, anim);
}

void AtlasPool::forceAddTile(img::Image *img, std::optional<AtlasTileAnim> anim)
{
    if (type == AtlasType::RECTPACK2D) {
        auto rectpackAtlas = dynamic_cast<Rectpack2DAtlas *>(atlases.back());
        u32 lastAtlasN = atlases.size()-1;

        if (!rectpackAtlas->packSingleTile(img, lastAtlasN, anim)) {
            Rectpack2DAtlas *atlas = new Rectpack2DAtlas(cache, prefixName, lastAtlasN+1, maxTextureSize, img, filtered, anim);
            cache->cacheResource<Atlas>(ResourceType::ATLAS, atlas,
                atlas->getName(atlas->getTextureSize(), atlases.size()));

            atlases.push_back(atlas);
        }

    }
}
