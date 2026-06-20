#pragma once

#include "irrlichttypes_bloated.h"
#include <Image/Image.h>
#include <Video/Texture.h>
#include <rectpack2D/finders_interface.h>
#include "client/render/tile.h"
#include <list>
#include <memory>
#include <optional>
#include <unordered_map>

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

	AnimatedAtlasTile(Atlas *atlas, video::Image *image, AnimationInfo anim)
		: AtlasTile(atlas, image), info(std::move(anim))
	{}

	bool draw(f32 time) override;
};

typedef std::pair<u32, u32> AtlasTileAnim;

class Rectpack2DPacker
{
	std::vector<rectpack2D::rect_xywh> rects;
	std::vector<core::rect<u32>> freeSpaces;
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

struct AnimatedAtlasTile;

// Interface saving and handling sets of atlases
// Note: 'addTile' and 'addAnimatedTile' calls and atlases building
// must be done *before* the atlases tiles get used in materials
// Otherwise those objects' meshes will get invalid atlases tiles UVs!
class AtlasPool
{
	std::string prefixName;

	u32 maxTextureSize;
	bool filtered;

	std::vector<std::unique_ptr<Atlas>> atlases;
	std::vector<video::Image *> images;
	std::unordered_map<u32, AtlasTileAnim> animatedImages;
public:
	AtlasPool(const std::string &_name, u32 _maxTextureSize, bool _filtered)
		: prefixName(_name), maxTextureSize(_maxTextureSize), filtered(_filtered)
	{}

	~AtlasPool();

	Atlas *getAtlas(u32 i) const;
	Atlas *getAtlasByTile(video::Image *tile, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);

	AtlasTile *getTileByImage(video::Image *tile);
	AnimatedAtlasTile *getAnimatedTileByImage(video::Image *tile);

	u32 getAtlasCount() const
	{
		return atlases.size();
	}

	void addTile(video::Image *img);
	void addAnimatedTile(video::Image *img, AtlasTileAnim anim);

	core::rectf getTileRect(video::Image *tile, bool toUV=false, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);

	// Recursively create and fill new atlases with tiles while the internal image counter doesn't reach some limit
	void build();

	void updateAnimatedTiles(f32 time);

	/*void updateMeshUVs(scene::IMeshBuffer *buffer, u32 start_index, u32 index_count, video::Image *tile,
		video::Image* oldTile=nullptr, bool toUV=true, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);
	void updateAllMeshUVs(scene::IMeshBuffer *buffer, video::Image *tile,
		video::Image* oldTile=nullptr, bool toUV=true, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);*/
private:
	void forceAddTile(video::Image *img, std::optional<AtlasTileAnim> anim=std::nullopt);
};
