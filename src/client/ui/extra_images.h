#pragma once

#include <Utils/Rect.h>
#include <array>
#include <memory>
#include "sprite.h"

namespace render
{
    class Texture2D;
}

class MeshBuffer;
class Renderer;
class UISprite;
class AtlasPool;
class RenderSystem;
class SpriteDrawBatch;

typedef std::pair<u32, u32> AtlasTileAnim;

class Image2D9Slice : public UISprite
{
    rectf srcRect, destRect, middleRect;
    std::array<img::color8, 4> rectColors;
    
    AtlasPool *guiPool;
    img::Image *image=nullptr;
public:
    Image2D9Slice(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool, u32 depthLevel=0);
    Image2D9Slice(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool,
                  const rectf &src_rect, const rectf &dest_rect,
                  const rectf &middle_rect, img::Image *baseImg,
                  const std::array<img::color8, 4> &colors=UISprite::defaultColors,
                  std::optional<AtlasTileAnim> anim=std::nullopt, u32 depthLevel=0);

    void updateRects(
        const rectf &src_rect, const rectf &dest_rect, const rectf &middle_rect, img::Image *img=nullptr,
    	const std::array<img::color8, 4> &colors=UISprite::defaultColors, const recti *clipRect=nullptr,
    	std::optional<AtlasTileAnim> anim=std::nullopt);

    void appendToBatch() override;
    void updateBatch() override;
private:
    void createSlice(u8 x, u8 y);
    void createSlices();
};

class UIRects : public UISprite
{
    AtlasPool *guiPool;
    std::vector<img::Image *> images;
public:
    UIRects(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool,
        u32 init_rects_count=0, u32 depthLevel=0);
    UIRects(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool,
        const std::vector<TexturedRect> &rects, u32 depthLevel=0);

    void addRect(const TexturedRect &rect, std::optional<rectf> srcRect=std::nullopt);
    void updateRect(u32 n, const TexturedRect &rect, std::optional<rectf> srcRect=std::nullopt);

    void appendToBatch() override;
    void updateBatch() override;
};

