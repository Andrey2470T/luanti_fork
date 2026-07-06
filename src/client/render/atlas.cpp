#include "atlas.h"
#include "Video/VideoDriver.h"
#include "settings.h"

struct FrameEdge
{
	core::recti source;
	core::position2di pos;
	v2s32 dir;
};

video::Image *AtlasTile::createFramedImage() const
{
	u8 frameThickness = atlas->getFrameThickness();

	if (!image || frameThickness == 0) {
		image->grab();
		return image;
	}

	auto frameRect = image->getClipRect();
	auto frameSize = frameRect.getSize();

	auto framedImage = new video::Image(image->getColorFormat(), size);

	// Draw the base image in the center
	auto framedImgOffset = frameThickness * image->getPitch() + frameThickness * image->getBytesPerPixel();
	image->copyToNoScaling(
		(u8 *)(framedImage->getData()) + framedImgOffset,
		frameSize.Width, frameSize.Height, image->getColorFormat(), image->getPitch());

	// Draw the four frame edges with some pixel thickness
	std::array<FrameEdge, 4> edges = {
		// source is right, pos is left, dir is left
		FrameEdge{
			{(s32)(size.X-frameThickness), (s32)(frameThickness), (s32)(size.X-frameThickness), (s32)(size.Y-frameThickness)},
			{(s32)(frameThickness - 1), (s32)(frameThickness)}, {(s32)(-1), (s32)(0)}},
		// source is left, pos is right, dir is right
		FrameEdge{
			{(s32)(frameThickness), (s32)(frameThickness), (s32)(frameThickness), (s32)(size.Y - frameThickness)},
			{(s32)(size.X - frameThickness + 1), (s32)(frameThickness)}, {(s32)(0), (s32)(1)}},
		// source is bottom, pos is top, dir is up
		FrameEdge{
			{(s32)(frameThickness), (s32)(size.Y - frameThickness), (s32)(size.X - frameThickness), (s32)(size.Y - frameThickness)},
			{(s32)(frameThickness), (s32)(frameThickness - 1)}, {(s32)(0), (s32)(1)}},
		// source is top, pos is bottom, dir is down
		FrameEdge{
			{(s32)(frameThickness), (s32)(frameThickness), (s32)(size.X - frameThickness), (s32)(frameThickness)},
			{(s32)(frameThickness), (s32)(size.Y - frameThickness + 1)}, {(s32)(0), (s32)(-1)}}
	};

	for (const auto &edge : edges) {
		auto &sourceRect = edge.source;

		for (u8 k = 0; k < frameThickness; k++) {
			v2s32 pos = edge.pos + edge.dir * (s8)k;
			image->copyTo(framedImage, pos, sourceRect);
		}
	}

	return framedImage;
}

bool AtlasTile::draw(f32 time)
{
	auto tex = atlas->getTexture();

	auto actualImage = createFramedImage();

	if (!actualImage || !tex)
		return false;

	tex->bind();
	tex->uploadTexture(0, actualImage, pos.X, pos.Y);

	actualImage->drop();

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

	auto actualImage = createFramedImage();

	tex->bind();
	tex->uploadTexture(0, actualImage, pos.X, pos.Y);

	actualImage->drop();

	return true;
}

Atlas::Atlas(
	video::VideoDriver *_driver, const std::string &_prefixName,
	u32 _atlasNum, Rectpack2DPackerOutput &output)
	: driver(_driver), frameThickness(output.frameThickness)
{
	assert(output.images.size() == output.rects.size());

	std::string name = _prefixName;
	name += "_";
	name += std::to_string(_atlasNum);
	name += "_";
	name += std::to_string(output.atlasSize) + "x" + std::to_string(output.atlasSize);

	texture = new video::GLTexture(
		name, {output.atlasSize, output.atlasSize}, video::ETT_2D, video::ECF_A8R8G8B8,
		driver, 0, output.frameThickness, false);
	driver->addTexture(texture);

	for (u32 i = 0; i < output.images.size(); i++) {
		auto &imgEntry = output.images.at(i);
		auto &rect = output.rects.at(i);

		AtlasTile *tile = nullptr;

		if (imgEntry.anim.hasAnimation()) {
			tile = new AnimatedAtlasTile(this, imgEntry.image, imgEntry.anim);
			addAnimatedTile(static_cast<AnimatedAtlasTile *>(tile));
		}
		else {
			tile = new AtlasTile(this, imgEntry.image);
			addTile(tile);
		}

		tile->pos = v2u32(static_cast<u32>(rect.x), static_cast<u32>(rect.y));
		tile->size = v2u32(static_cast<u32>(rect.w), static_cast<u32>(rect.h));
	}

	drawTiles(0.0f);
}

Atlas::~Atlas()
{
	if (texture)
		driver->removeTexture(texture);
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

bool Atlas::addAnimatedTile(AnimatedAtlasTile *tile)
{
	if (!addTile(tile))
		return false;
	animatedTiles.emplace_back(tiles.size() - 1);

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

		for (auto &tile : tiles) {
			if (tile->draw(time))
				wasDraw = true;
		}
	}
	else {
		for (u32 anim_i : animatedTiles) {
			auto tile = getTile(anim_i);

			if (tile->draw(time))
				wasDraw = true;
		}
	}

	if (wasDraw) {
		if (texture->hasMipMaps())
			texture->regenerateMipMaps();
		texture->unbind();
	}
}

bool Atlas::operator==(const Atlas *other) const
{
	if (!texture || !other->texture)
		return false;
	return texture->getName().getInternalName() == other->texture->getName().getInternalName();
}

using empty_rects = empty_spaces<false>;

#define CALC_LARGEST_POT_SIDE(side) \
	std::pow(2u, (u32)std::ceil(std::log2(side)))

void Rectpack2DPacker::rectPackFunc()
{
	auto report_successful = [](rect_xywh&) {
		return callback_result::CONTINUE_PACKING;
	};
	auto report_unsuccessful = [](rect_xywh&) {
		return callback_result::ABORT_PACKING;
	};

	auto res_size = find_best_packing<empty_rects>(output.rects,
		make_finder_input(output.atlasSize, 1, report_successful, report_unsuccessful, flipping_option::DISABLED));
	output.atlasSize = CALC_LARGEST_POT_SIDE(std::max(res_size.w, res_size.h));
}

Rectpack2DPackerOutput &Rectpack2DPacker::pack(std::vector<ImageEntry> &images, u32 &curImg)
{
	output.clear();

	u32 atlasArea = 0;
	u32 maxArea = maxTextureSize * maxTextureSize;

	if (filtering) {
		// The image at 'curImg' is considered to have minimal size (sorting is done from smallest to largest)
		core::rect<u32> minImgRect;

		for (u32 i = curImg; i < images.size(); i++) {
			auto img = images.at(i).image;

			if (img) {
				minImgRect = img->getClipRect();
				break;
			}
		}
		output.frameThickness = core::u32_log2(std::min(minImgRect.getWidth(), minImgRect.getHeight()));
	}

	for (; curImg != images.size(); curImg++) {
		auto &entry = images.at(curImg);

		if (!entry.image)
			continue;

		v2u32 imgSize = entry.image->getClipRect().getSize();
		u32 newRW = imgSize.X + 2 * output.frameThickness;
		u32 newRH = imgSize.Y + 2 * output.frameThickness;
		u32 imgArea = newRW	* newRH;

		if ((atlasArea + imgArea) > maxArea)
			break;

		output.images.emplace_back(entry.image, entry.anim);
		output.rects.emplace_back(0, 0, newRW, newRH);

		atlasArea += imgArea;
	}

	output.atlasSize = CALC_LARGEST_POT_SIDE(std::sqrt((f32)atlasArea));

	rectPackFunc();

	return output;
}

AtlasPool::AtlasPool(video::VideoDriver *_driver, const std::string &_name)
	: driver(_driver), prefixName(_name), packer(driver->getMaxTextureSize().Width,
	  g_settings->getBool("bilinear_filter") || g_settings->getBool("trilinear_filter") ||
	  g_settings->getBool("anisotropic_filter"))
{}

Atlas *AtlasPool::getAtlas(u8 num)
{
	return atlases.at(num).get();
}

Atlas *AtlasPool::getAtlasByImage(const ImageEntry &image, bool force_add)
{
	for (auto &atlas : atlases) {
		auto atlasTile = atlas->getTileByImage(image.image);

	if (atlasTile)
		return atlas.get();
	}

	/*if (force_add) {
		forceAddTile(image);
		return atlases.back().get();
	}*/
	return nullptr;
}

AtlasTile *AtlasPool::getTileByImage(const ImageEntry &image)
{
	auto atlas = getAtlasByImage(image);

	if (!atlas)
		return nullptr;

	return atlas->getTileByImage(image.image);
}

void AtlasPool::addTile(const ImageEntry &image)
{
	images.insert(image);
}

core::rectf AtlasPool::getTileRect(const ImageEntry &image, bool force_add)
{
	auto tile = getTileByImage(image);

	if (!tile) {
		/*if (force_add) {
			forceAddTile(image);
			tile = getTileByImage(image);
		}*/
		return core::rectf();
	}

	return core::rectf(
		static_cast<f32>(tile->pos.X), static_cast<f32>(tile->pos.Y + tile->size.Y),
		static_cast<f32>(tile->pos.X + tile->size.X), static_cast<f32>(tile->pos.Y));
}

/*
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

void AtlasPool::build()
{
	if (images.empty())
		return;

	if (sortedImages.empty()) {
		sortedImages.reserve(images.size());
		for (auto &img : images)
			sortedImages.emplace_back(img);
		std::sort(sortedImages.begin(), sortedImages.end(),
			[](const ImageEntry &a, const ImageEntry &b) {
				auto aSize = a.image ? a.image->getClipRect().getArea() : 0;
				auto bSize = b.image ? b.image->getClipRect().getArea() : 0;
				return aSize < bSize;
			});
	}

	auto res = packer.pack(sortedImages, curImg);

	auto atlas = std::make_unique<Atlas>(driver, prefixName, atlases.size(), res);
	atlases.emplace_back(atlas.release());

	if (curImg != images.size())
		build();

	images.clear();
	sortedImages.clear();
}

void AtlasPool::drawTiles(f32 time)
{
	for (auto &atlas : atlases)
		atlas->drawTiles(time);
}
