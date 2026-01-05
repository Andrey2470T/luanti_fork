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
    Image2D9Slice(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool);
    Image2D9Slice(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool,
                  const rectf &src_rect, const rectf &dest_rect,
                  const rectf &middle_rect, img::Image *baseImg,
                  const std::array<img::color8, 4> &colors=UISprite::defaultColors,
                  std::optional<AtlasTileAnim> anim=std::nullopt);

    void updateRects(
        const rectf &src_rect, const rectf &dest_rect, const rectf &middle_rect, img::Image *img=nullptr,
    	const std::array<img::color8, 4> &colors=UISprite::defaultColors, const recti *clipRect=nullptr,
    	std::optional<AtlasTileAnim> anim=std::nullopt);
    void createSlices();
private:
    void createSlice(u8 x, u8 y);
};

class UIRects : public UISprite
{
    std::vector<img::Image *> images;
public:
    UIRects(ResourceCache *resCache, SpriteDrawBatch *drawBatch, u32 init_rects_count=0);
    UIRects(ResourceCache *resCache, SpriteDrawBatch *drawBatch, const std::vector<TexturedRect> &rects);

    void addRect(const TexturedRect &rect);
    void updateRect(u32 n, const TexturedRect &rect);
};

