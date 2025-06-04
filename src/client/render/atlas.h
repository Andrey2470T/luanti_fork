#pragma once

#include <memory>
#include <Image/Image.h>
#include <Render/StreamTexture2D.h>
#include <Render/TTFont.h>>
#include <list>

struct AtlasTile
{
	v2u pos;
	v2u size;
	
	img::Image *image;
	
	u32 atlasNum;

    AtlasTile(img::Image *img, u32 num)
        : size(img->getClipSize()), image(img), atlasNum(num)
    {}
    virtual ~AtlasTile() = default;

    rectf toUV(u32 atlasSize) const;
	
	bool operator==(const AtlasTile *other)
	{
		return image == other->image;
    }

    size_t hash() const
    {
    	return std::hash<img::Image*>{}(image);
    }
};

enum class AtlasType : u8
{
    RECTPACK2D,
    GLYPH
};

class Atlas
{
protected:
    std::unique_ptr<render::StreamTexture2D> texture;
    
    // Saves only unique tiles (determined by the hash)
    std::unordered_map<size_t, u32> hash_to_index;
    std::vector<std::unique_ptr<AtlasTile>> tiles;
    
    std::list<u32> dirty_tiles;
public:
    Atlas() = default;

    void createTexture(const std::string &name, u32 num, u32 size, u8 maxMipLevel, img::PixelFormat format=img::PF_RGBA8);
    
    render::StreamTexture2D *getTexture() const
    {
    	return texture.get();
    }
    
    u32 getTextureSize() const
    {
        return texture->getSize().X;
    }

    u32 getTilesCount() const
    {
        return tiles.size();
    }

    std::string getName() const
    {
        return texture->getName();
    }
    
    bool addTile(const AtlasTile *tile);

    AtlasTile *getTile(u32 i) const;
    
    void markDirty(u32 i);
    
    bool canFit(const rectu &area, const rectu &tile) const
    {
    	return area.isRectInside(tile);
    }
    
    virtual void packTiles() = 0;
    
    void drawTiles();

    bool operator==(const Atlas *other) const;
};

class ResourceCache;


class AtlasPool
{
    AtlasType type;
    std::string prefixName;

    ResourceCache *cache;
    u32 maxTextureSize;
    bool hasMips;

    std::vector<Atlas *> atlases;
public:
    AtlasPool(AtlasType _type, const std::string &_name, ResourceCache *_cache, u32 _maxTextureSize, bool _hasMips)
        : type(_type), prefixName(_name), cache(_cache), maxTextureSize(_maxTextureSize), hasMips(_hasMips)
    {}

    ~AtlasPool();

    Atlas *getAtlas(u32 i) const
    {
        if (i > atlases.size()-1)
            return nullptr;

        return atlases.at(i);
    }

    // Recursively create and fill new atlases with tiles while the internal image counter doesn't reach some limit
    void buildRectpack2DAtlas(const std::vector<img::Image *> &images, const std::unordered_map<u32, std::pair<u32, u32>> &animatedImages);
    void buildGlyphAtlas(render::TTFont *ttfont);

    void updateAnimatedTiles(f32 time);
};
