#pragma once

#include <memory>
#include <Image/Image.h>
#include <Render/StreamTexture2D.h>

class ResourceCache;

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
