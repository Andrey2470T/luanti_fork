#pragma once

#include "irrlichttypes_bloated.h"
#include <Image/Image.h>
#include <list>
#include <optional>
#include <unordered_map>

struct AtlasTile
{
    // Left lower corner
	v2u pos;
	v2u size{0, 0};

	video::Image *image = nullptr;

	u32 atlasNum;

	AtlasTile(u32 num, video::Image *img=nullptr)
		: image(img), atlasNum(num)
	{}

	core::rectf toUV(u32 atlasSize) const;

	bool operator==(const AtlasTile *other)
	{
		return image == other->image;
	}

	size_t hash() const
	{
		if (!image)
			return 0;
		return std::hash<img::Image*>{}(image);
	}
};

struct AnimatedAtlasTile : public AtlasTile
{
	u32 frame_length_ms;
	u32 frame_count = 1;
	u32 frame_offset = 0;

	u32 cur_frame = 0;

	AnimatedAtlasTile(video::Image *img, u32 num, u32 length, u32 count)
		: AtlasTile(img, num), frame_length_ms(length), frame_count(count)
	{}
	core::rectu getFrameCoords(u32 frame_num) const;
	void updateFrame(f32 time); // in sec
};

typedef std::pair<u32, u32> AtlasTileAnim;

class Atlas
{
protected:
	video::GLTexture *texture = nullptr;

	std::vector<AtlasTile> tiles;
	std::unordered_map<img::Image *, u32> mapImgToTileIndex;

	std::list<u32> dirtyTiles;
public:
	Atlas() = default;

	void createTexture(const std::string &name, u32 size);

	std::string getName(u32 size, u32 num) const;

	video::GLTexture *getTexture() const
	{
		return texture;
	}

	u32 getTextureSize() const
	{
		return texture->getSize().X;
	}

	u32 getTilesCount() const
	{
		return tiles.size();
	}

	bool addTile(AtlasTile *tile);

	AtlasTile *getTile(u32 i) const;
	AtlasTile *getTileByImage(video::Image *img);

	void markDirty(u32 i);

	bool canFit(const core::rectu &area, const core::rectu &tile) const
	{
		return area.isRectInside(tile);
	}

	void packTiles();

	void drawTiles();

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

	rectf getTileRect(video::Image *tile, bool toUV=false, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);

	// Recursively create and fill new atlases with tiles while the internal image counter doesn't reach some limit
	void build();

	void updateAnimatedTiles(f32 time);

	void updateMeshUVs(scene::IMeshBuffer *buffer, u32 start_index, u32 index_count, video::Image *tile,
		video::Image* oldTile=nullptr, bool toUV=true, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);
	void updateAllMeshUVs(scene::IMeshBuffer *buffer, video::Image *tile,
		video::Image* oldTile=nullptr, bool toUV=true, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);
private:
	void forceAddTile(video::Image *img, std::optional<AtlasTileAnim> anim=std::nullopt);
};
