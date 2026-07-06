#pragma once

#include "Video/VideoDriver.h"
#include "irrlichttypes_bloated.h"
#include <Image/Image.h>
#include <Video/Texture.h>
#include "client/render/tile.h"
#include <rectpack2D/finders_interface.h>
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
	virtual ~AtlasTile() = default;

	video::Image *createFramedImage() const;

	virtual bool draw(f32 time);

	bool operator==(const AtlasTile *other) const
	{
		return (pos == other->pos && size == other->size);
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

struct Rectpack2DPackerOutput;

class Atlas
{
protected:
	video::VideoDriver *driver;
	video::GLTexture *texture = nullptr;

	std::vector<std::unique_ptr<AtlasTile>> tiles;
	std::unordered_map<video::Image *, AtlasTile *> mapImgToTileIndex;
	std::vector<u32> animatedTiles;

	u8 frameThickness;

	bool firstDraw{true};
public:
	Atlas(
		video::VideoDriver *_driver, const std::string &_prefixName,
		u32 _atlasNum, Rectpack2DPackerOutput &output);

	~Atlas();

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

	u8 getFrameThickness() const
	{
		return frameThickness;
	}

	bool addTile(AtlasTile *tile);
	bool addAnimatedTile(AnimatedAtlasTile *tile);

	AtlasTile *getTile(u32 i) const;
	AtlasTile *getTileByImage(video::Image *img);

	void drawTiles(f32 time);

	bool operator==(const Atlas *other) const;
};

struct ImageEntry {
	video::Image *image{nullptr};
	AnimationInfo anim;

	ImageEntry(video::Image* _image, const AnimationInfo &_anim={})
		: image(_image), anim(_anim)
	{}

	bool operator==(const ImageEntry &other) const
	{
		return image == other.image && anim == other.anim;
	}
};

struct ImageEntryHash {
	size_t operator()(const ImageEntry& entry) const {
		size_t seed = 0;
		seed ^= std::hash<video::Image*>{}(entry.image) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= entry.anim.hash() + 0x9e3779b9 + (seed << 6) + (seed >> 2);

		return seed;
	}
};

typedef std::unordered_set<ImageEntry, ImageEntryHash> ImagesSet;
using namespace rectpack2D;

struct Rectpack2DPackerOutput
{
	u32 atlasSize;
	u8 frameThickness;
	std::vector<ImageEntry> images;
	std::vector<rectpack2D::rect_xywh> rects;

	void clear()
	{
		atlasSize = 0;
		frameThickness = 0;
		images.clear();
		rects.clear();
	}
};

class Rectpack2DPacker
{
	std::vector<core::rect<u32>> freeSpaces;

	u32 maxTextureSize;
	bool filtering{false};

	Rectpack2DPackerOutput output;

public:
	Rectpack2DPacker(u32 _maxTextureSize, bool _filtering)
		: maxTextureSize(_maxTextureSize), filtering(_filtering)
	{}

	Rectpack2DPackerOutput &getOutput()
	{
		return output;
	}

	void rectPackFunc();
	Rectpack2DPackerOutput &pack(std::vector<ImageEntry> &images, u32 &curImg);
};

// Interface saving and handling sets of atlases
// Note: 'addTile' and 'addAnimatedTile' calls and atlases building
// must be done *before* the atlases tiles get used in materials
// Otherwise those objects' meshes will get invalid atlases tiles UVs!
class AtlasPool
{
	video::VideoDriver *driver;

	std::string prefixName;

	Rectpack2DPacker packer;

	std::vector<std::unique_ptr<Atlas>> atlases;
	ImagesSet images;
	std::vector<ImageEntry> sortedImages;

	u32 curImg{0};
public:
	AtlasPool(video::VideoDriver *_driver, const std::string &_name);

	Atlas *getAtlas(u8 num);
	Atlas *getAtlasByImage(const ImageEntry &image, bool force_add=false);

	AtlasTile *getTileByImage(const ImageEntry &image);

	void addTile(const ImageEntry &image);

	core::rectf getTileRect(const ImageEntry &image, bool force_add=false);

	// Recursively create and fill new atlases with tiles while the internal image counter doesn't reach some limit
	void build();

	void drawTiles(f32 time);

	/*void updateMeshUVs(scene::IMeshBuffer *buffer, u32 start_index, u32 index_count, video::Image *tile,
		video::Image* oldTile=nullptr, bool toUV=true, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);
	void updateAllMeshUVs(scene::IMeshBuffer *buffer, video::Image *tile,
		video::Image* oldTile=nullptr, bool toUV=true, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);*/
//private:
//	void forceAddTile(const ImageEntry &image);
};
