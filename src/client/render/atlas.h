#pragma once

#include "Video/VideoDriver.h"
#include "irrlichttypes_bloated.h"
#include <Image/Image.h>
#include <Video/Texture.h>
#include "client/render/tile.h"
#include <list>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

class Atlas;

struct AtlasTile
{
	// Left lower corner (in pixel units)
	v2u32 pos;
	// Width, height (in pixel units)
	v2u32 size{0, 0};

	video::Image *image = nullptr;

	Atlas *atlas = nullptr;

	AtlasTile(Atlas *_atlas, video::Image *_image=nullptr)
		: image(_image), atlas(_atlas)
	{}

	virtual bool draw(f32 time);

	bool operator==(const AtlasTile *other)
	{
		return (pos == other->pos && size == other->size);
	}

	size_t hash() const
	{
		if (!image)
			return 0;
		return std::hash<video::Image*>{}(image);
	}
};

struct AnimatedAtlasTile : public AtlasTile
{
	AnimationInfo info;

	AnimatedAtlasTile(Atlas *atlas, video::Image *image, const AnimationInfo &anim)
		: AtlasTile(atlas, image), info(std::move(anim))
	{}

	bool draw(f32 time) override;
};

class Atlas
{
protected:
	video::VideoDriver *driver;
	video::GLTexture *texture = nullptr;

	std::vector<std::unique_ptr<AtlasTile>> tiles;
	std::unordered_map<video::Image *, AtlasTile *> mapImgToTileIndex;
	std::vector<u32> animatedTiles;

	bool firstDraw{true};
public:
	Atlas(video::VideoDriver *_driver)
		: driver(_driver)
	{}

	~Atlas();

	void createTexture(const std::string &prefixName, u32 num, u32 size);

	video::GLTexture *getTexture() const
	{
		return texture;
	}

	u32 getTextureSize() const
	{
		return texture->getSize().Width;
	}

	u32 getTilesCount() const
	{
		return tiles.size();
	}

	bool addTile(AtlasTile *tile);

	AtlasTile *getTile(u32 i) const;
	AtlasTile *getTileByImage(video::Image *img);

	bool canFit(const core::rect<u32> &area, const core::rect<u32> &tile) const
	{
		return area.isRectInside(tile);
	}

	void packTiles();

	void drawTiles(f32 time);

	bool operator==(const Atlas *other) const;
};

typedef std::pair<video::Image *, AnimationInfo> ImagePair;

struct ImagePairHash {
	size_t operator()(const ImagePair& entry) const {
		size_t seed = 0;
		seed ^= std::hash<video::Image*>{}(entry.first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= entry.second.hash() + 0x9e3779b9 + (seed << 6) + (seed >> 2);

		return seed;
	}
};

struct ImagePairEqual {
	bool operator()(const ImagePair& a, const ImagePair& b) const {
		return a.first == b.first && a.second == b.second;
	}
};

typedef std::unordered_set<ImagePair, ImagePairHash, ImagePairEqual> ImagesSet;

class Rectpack2DPacker
{
	std::vector<core::rect<u32>> freeSpaces;

	u8 frameThickness{0};
	u32 maxTextureSize;

public:
	Rectpack2DPacker(u8 _frameThickness, u32 _maxTextureSize)
		: frameThickness(_frameThickness), maxTextureSize(_maxTextureSize)
	{}

	void pack(
		const ImagesSet &images, ImagesSet::iterator &curImg,
		u32 &atlasSize, std::vector<core::rect<u32>> &output);
};

// Interface saving and handling sets of atlases
// Note: 'addTile' and 'addAnimatedTile' calls and atlases building
// must be done *before* the atlases tiles get used in materials
// Otherwise those objects' meshes will get invalid atlases tiles UVs!
class AtlasPool
{
	video::VideoDriver *driver;

	std::string prefixName;
	u32 maxTextureSize;
	bool filtered;
	u8 maxMipLevel{0};

	std::unique_ptr<Rectpack2DPacker> packer;

	std::vector<std::unique_ptr<Atlas>> atlases;
	ImagesSet images;
public:
	AtlasPool(video::VideoDriver *_driver, const std::string &_name);

	Atlas *getAtlasByTile(video::Image *tile, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);

	AtlasTile *getTileByImage(video::Image *tile);
	AnimatedAtlasTile *getAnimatedTileByImage(video::Image *tile);

	void addTile(video::Image *img);
	void addAnimatedTile(video::Image *img, const AnimationInfo &anim);

	core::rectf getTileRect(video::Image *tile, bool toUV=false, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);

	// Recursively create and fill new atlases with tiles while the internal image counter doesn't reach some limit
	void build();

	void drawTiles(f32 time);

	/*void updateMeshUVs(scene::IMeshBuffer *buffer, u32 start_index, u32 index_count, video::Image *tile,
		video::Image* oldTile=nullptr, bool toUV=true, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);
	void updateAllMeshUVs(scene::IMeshBuffer *buffer, video::Image *tile,
		video::Image* oldTile=nullptr, bool toUV=true, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);*/
private:
	void forceAddTile(video::Image *img, std::optional<AtlasTileAnim> anim=std::nullopt);
};
