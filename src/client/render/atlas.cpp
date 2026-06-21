#include "atlas.h"
#include "Video/VideoDriver.h"
#include <rectpack2D/finders_interface.h>
#include "settings.h"

bool AtlasTile::draw(f32 time)
{
	auto tex = atlas->getTexture();

	if (!image || !tex)
		return false;

	 tex->uploadTexture(0, image, pos.X, pos.Y);

	 return true;
}

bool AnimatedAtlasTile::draw(f32 time)
{
	auto tex = atlas->getTexture();

	if (!image || !tex)
		return false;

	if (!info.updateFrame(time))
		return false;

	image->getClipRect() = info.getCurrentFrameRect();

	tex->uploadTexture(0, image, pos.X, pos.Y);

	return true;
}

Atlas::~Atlas()
{
	if (texture)
		driver->removeTexture(texture);
}

void Atlas::createTexture(const std::string &prefixName, u32 num, u32 size)
{
	std::string name = prefixName;
	name += "_";
	name += std::to_string(num);
	name += "_";
	name += std::to_string(size) + "x" + std::to_string(size);

	texture = driver->addTexture(core::dimension2du(size, size), name, video::ECF_A8R8G8B8);
}

bool Atlas::addTile(AtlasTile *tile)
{
	auto foundTile = getTileByImage(tile->image);

	if (foundTile)
		return false;

	tiles.emplace_back(tile);
	mapImgToTileIndex[tile->image] = tile;

	return true;
}

AtlasTile *Atlas::getTile(u32 i) const
{
	if (i > tiles.size()-1)
		return nullptr;

	return tiles.at(i).get();
}

AtlasTile *Atlas::getTileByImage(video::Image *img)
{
	return mapImgToTileIndex[img];
}

void Atlas::drawTiles(f32 time)
{
	if (!texture || tiles.empty())
		return;

	bool wasDraw = false;

	if (firstDraw) {
		firstDraw = false;

		for (auto &tile : tiles)
			wasDraw = tile->draw(time);
	}
	else {
		for (u32 anim_i : animatedTiles) {
			auto tile = getTile(anim_i);
			wasDraw = tile->draw(time);
		}
	}

	if (wasDraw && texture->hasMipMaps())
		texture->regenerateMipMaps();
}

bool Atlas::operator==(const Atlas *other) const
{
	if (!texture || !other->texture)
		return false;
	return texture->getName().getInternalName() == other->texture->getName().getInternalName();
}

using namespace rectpack2D;
using empty_rects = empty_spaces<false>;

void Rectpack2DPacker::pack(
	const ImagesSet &images, ImagesSet::iterator &curImg,
	u32 &atlasSize, std::vector<core::rect<u32>> &output)
{
	u32 atlasArea = 0;
	u32 maxArea = maxTextureSize * maxTextureSize;

	std::vector<rectpack2D::rect_xywh> rects;

	for (; curImg != images.end(); curImg++) {
		auto img = curImg->first;
		auto anim = curImg->second;

		if (!img)
			continue;

		v2u32 imgSize = img->getClipRect().getSize() + v2u32(2*frameThickness);
		u32 imgArea = imgSize.X	* imgSize.Y;

		// Double the atlas area for the cracks space
		if ((atlasArea + imgArea) * 2 > maxArea)
			break;

		rects.emplace_back(0, 0, imgSize.X, imgSize.Y);

		atlasArea += imgArea;
	}

	atlasSize = std::pow(2u, (u32)std::ceil(std::log2(std::sqrt((f32)atlasArea))));

	auto report_successful = [](rect_xywh&) {
		return callback_result::CONTINUE_PACKING;
	};
	auto report_unsuccessful = [](rect_xywh&) {
		return callback_result::ABORT_PACKING;
	};

	auto res_size = find_best_packing<empty_rects>(rects,
		make_finder_input(atlasSize, 1, report_successful, report_unsuccessful, flipping_option::DISABLED));
	atlasSize = std::max(res_size.w, res_size.h);

	for (u32 i = 0; i < rects.size(); i++) {
		auto rect = rects.at(i);
		output.emplace_back(rect.x, rect.y, rect.w, rect.h);
	}
}

AtlasPool::AtlasPool(video::VideoDriver *_driver, const std::string &_name)
	: driver(_driver), prefixName(_name), maxTextureSize(driver->getMaxTextureSize().Width),
	  filtered(g_settings->getBool("bilinear_filter") || g_settings->getBool("trilinear_filter") ||
	  g_settings->getBool("anisotropic_filter"))
{}

void AtlasPool::addTile(video::Image *img)
{
	images.emplace(img, AnimationInfo());
}

void AtlasPool::addAnimatedTile(video::Image *img, const AnimationInfo &anim)
{
	images.emplace(img, anim);
}

/*
Atlas *AtlasPool::getAtlasByTile(img::Image *tile, bool force_add, std::optional<AtlasTileAnim> anim)
{
    for (u32 i = 0; i < atlases.size(); i++) {
        auto atlas = atlases.at(i);
        auto atlasTile = atlas->getTileByImage(tile);

        if (atlasTile)
            return atlas;
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

rectf AtlasPool::getTileRect(img::Image *tile, bool toUV, bool force_add, std::optional<AtlasTileAnim> anim)
{
    if (type != AtlasType::RECTPACK2D)
        return rectf();

    for (u32 i = 0; i < atlases.size(); i++) {
        auto atlas = atlases.at(i);
        auto atlasTile = atlas->getTileByImage(tile);

        if (!atlasTile)
            continue;

        if (toUV)
            return atlasTile->toUV(atlas->getTextureSize());
        else {
            v2f ulc_f(atlasTile->pos.X, atlasTile->pos.Y + atlasTile->size.Y);
            v2f lrc_f(atlasTile->pos.X + atlasTile->size.X, atlasTile->pos.Y);
            return rectf(ulc_f, lrc_f);
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

    atlasNum = 0;
    startImg = 0;
}

void AtlasPool::updateMeshUVs(MeshBuffer *buffer, u32 start_index, u32 index_count, img::Image *tile,
    img::Image* oldTile, bool toUV, bool force_add, std::optional<AtlasTileAnim> anim)
{
    std::optional<u32> oldAtlasSize;
    std::optional<rectf> oldTileRect;

    if (oldTile) {
        auto prev_atlas = getAtlasByTile(oldTile);

        if (prev_atlas) {
            oldAtlasSize = prev_atlas->getTextureSize();
            oldTileRect = getTileRect(oldTile);
        }
    }

    auto atlas = getAtlasByTile(tile, force_add, anim);
    u32 newAtlasSize = atlas->getTextureSize();
    rectf newTileRect = getTileRect(tile);

    MeshOperations::recalculateMeshAtlasUVs(buffer, start_index, index_count,
        newAtlasSize, newTileRect, oldAtlasSize, oldTileRect, toUV);
    buffer->uploadData();
}

void AtlasPool::updateAllMeshUVs(MeshBuffer *buffer, img::Image *tile,
    img::Image *oldTile, bool toUV, bool force_add, std::optional<AtlasTileAnim> anim)
{
    updateMeshUVs(buffer, 0, buffer->getVertexCount(), tile, oldTile, toUV, force_add, anim);
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
}*/

void AtlasPool::drawTiles(f32 time)
{
	for (auto &atlas : atlases)
		atlas->drawTiles(time);
}
