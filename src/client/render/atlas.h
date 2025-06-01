#pragma once

#include <memory>
#include <Image/Image.h>
#include <Render/StreamTexture2D.h>
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

    rectf toUV(u32 atlasSize) const
    {
    	return rectf(
            (f32)pos.X / atlasSize,
            (f32)pos.Y / atlasSize,
            (f32)(pos.X + size.X) / atlasSize,
            (f32)(pos.Y + size.Y) / atlasSize
        );
    }
	
	bool operator==(const AtlasTile *other)
	{
		return image == other->image;
    }

    size_t hash() const
    {
    	return std::hash<img::Image*>{}(image);
    }
};

class Atlas
{
protected:
    std::unique_ptr<render::StreamTexture2D> texture;
    
    // Saves only unique tiles (determined by the hash)
    std::unordered_map<size_t, u32> hash_to_index;
    std::vector<std::unique_ptr<AtlasTile>> tiles;
    
    std:list<u32> dirty_tiles;
public:
    Atlas() = default;

    void createTexture(const std::string &name, u32 num, u32 size, u8 maxMipLevel)
    {
        std::string texName = name + "Atlas";
        texName += num;

        render::TextureSettings settings;
        settings.isRenderTarget = false;
        settings.hasMipMaps = maxMipLevel > 0 ? true : false;
        settings.maxMipLevel = maxMipLevel;

        texture = std::make_unique<render::StreamTexture2D>(
            texName, size, size, img::PF_RGBA8, settings);
    }
    
    render::StreamTexture2D *getTexture() const
    {
    	return texture.get();
    }
    
    v2u getTextureSize() const
    {
    	return texture->getSize();
    }
    
    bool addTile(const AtlasTile *tile)
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

    AtlasTile *getTile(u32 i) const
    {
        if (i > tiles.size()-1)
            return nullptr;

        return tiles.at(i).get();
    }
    
    void markDirty(u32 i)
    {
    	auto it = dirty_tiles.find(i);
    
        if (it != dirty_tiles.end())
            return;
            
        dirty_tiles.push_back(i);
    }
    
    bool canFit(const rectu &area, const rectu &tile) const
    {
    	return area.isRectInside(tile);
    }
    
    virtual void packTiles() = 0;
    
    void drawTiles()
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
};
