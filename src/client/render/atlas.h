#pragma once

#include <memory>
#include <Image/Image.h>
#include <Render/Texture2D.h>
#include <Render/TTFont.h>
#include <list>

struct AtlasTile
{
    // Left lower corner
	v2u pos;
    v2u size{0, 0};
	
	img::Image *image;
	
	u32 atlasNum;

    AtlasTile(img::Image *img, u32 num)
        : image(img), atlasNum(num)
    {
        if (img) {
            size = image->getClipSize();
        }
    }
    virtual ~AtlasTile() = default;

    rectf toUV(u32 atlasSize) const;
	
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

typedef std::pair<u32, u32> AtlasTileAnim;

enum class AtlasType : u8
{
    RECTPACK2D,
    GLYPH
};

class Atlas
{
protected:
    std::unique_ptr<render::Texture2D> texture;

    // Saves only unique tiles (determined by the hash)
    std::unordered_map<size_t, u32> hash_to_index;
    std::vector<std::unique_ptr<AtlasTile>> tiles;

    std::list<u32> dirty_tiles;
public:
    Atlas() = default;
    virtual ~Atlas() = default;

    void createTexture(const std::string &name, u32 size);
    
    virtual std::string getName(u32 size, u32 num) const = 0;

    render::Texture2D *getTexture() const
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
    
    bool addTile(AtlasTile *tile);

    AtlasTile *getTile(u32 i) const;
    AtlasTile *getTileByImage(img::Image *img) const;
    
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
class MeshBuffer;
struct AnimatedAtlasTile;

// Interface saving and handling sets of atlases of some type
// Note: 'addTile' and 'addAnimatedTile' calls and atlases building
// must be done *before* the atlases tiles get used in GUI, HUD, map, entities, particles and etc.
// Otherwise those objects' meshes will get invalid atlases tiles UVs!
class AtlasPool
{
    AtlasType type;
    std::string prefixName;

    ResourceCache *cache;
    u32 maxTextureSize;
    bool filtered;
    bool apply_modifiers;

    std::vector<Atlas *> atlases;
    std::vector<img::Image *> images;
    std::unordered_map<u32, AtlasTileAnim> animatedImages;
public:
    AtlasPool(AtlasType _type, const std::string &_name, ResourceCache *_cache,
        u32 _maxTextureSize, bool _filtered, bool _apply_modifiers)
        : type(_type), prefixName(_name), cache(_cache), maxTextureSize(_maxTextureSize),
          filtered(_filtered), apply_modifiers(_apply_modifiers)
    {}

    ~AtlasPool();

    Atlas *getAtlas(u32 i) const;
    Atlas *getAtlasByTile(img::Image *tile, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);

    AtlasTile *getTileByImage(img::Image *tile);
    AnimatedAtlasTile *getAnimatedTileByImage(img::Image *tile);

    u32 getAtlasCount() const
    {
        return atlases.size();
    }

    void addTile(img::Image *img);
    void addAnimatedTile(img::Image *img, AtlasTileAnim anim);

    rectf getTileRect(img::Image *tile, bool toUV=false, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);

    // Recursively create and fill new atlases with tiles while the internal image counter doesn't reach some limit
    void buildRectpack2DAtlas();
    void buildGlyphAtlas(render::TTFont *ttfont);

    void updateAnimatedTiles(f32 time);

    void updateMeshUVs(MeshBuffer *buffer, u32 start_index, u32 index_count, img::Image *tile,
        std::optional<img::Image *> oldTile=std::nullopt, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);
    void updateAllMeshUVs(MeshBuffer *buffer, img::Image *tile,
        std::optional<img::Image *> oldTile=std::nullopt, bool force_add=false, std::optional<AtlasTileAnim> anim=std::nullopt);
private:
    void forceAddTile(img::Image *img, std::optional<AtlasTileAnim> anim=std::nullopt);
};
