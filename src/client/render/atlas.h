#pragma once

#include <memory>
#include <Image/Image.h>
#include <Render/StreamTexture2D.h>

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
public:
    Atlas() = default;

    void createTexture(const std::string &name, u32 num, u32 size, u8 maxMipLevel)
    {
        std::string texName = name + "Atlas";
        texName += size;

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
    
    bool canFit(const rectu &area, const rectu &tile) const
    {
    	return area.isRectInside(tile);
    }
    
    virtual void packTiles() = 0;
    
    virtual void drawTiles() = 0;
};
