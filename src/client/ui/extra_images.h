#pragma once

#include "Utils/Rect.h"
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

class Image2D9Slice : public UISprite
{
    rectf srcRect, destRect, middleRect;
    std::array<img::color8, 4> rectColors;
public:
    Image2D9Slice(ResourceCache *resCache, Renderer *renderer,
        render::Texture2D *base_tex, const std::array<img::color8, 4> &colors);
    Image2D9Slice(ResourceCache *resCache, Renderer *renderer,
                  const rectf &src_rect, const rectf &dest_rect,
                  const rectf &middle_rect, render::Texture2D *base_tex,
                  const std::array<img::color8, 4> &colors);

    void updateRects(const rectf &src_rect, const rectf &dest_rect, const rectf &middle_rect);
    void createSlices();
private:
    void createSlice(u8 x, u8 y);
};

typedef std::pair<u32, u32> AtlasTileAnim;

class ImageSprite : public UISprite
{
    AtlasPool *pool;
    img::Image *image = nullptr;
public:
    ImageSprite(RenderSystem *rndsys, ResourceCache *cache);

    void update(img::Image *newImage, const rectf &rect, const std::array<img::color8, 4> &colors,
        const recti *cliprect=nullptr, std::optional<AtlasTileAnim> anim = std::nullopt);
    void update(img::Image *newImage, const rectf &rect, const img::color8 &color,
        const recti *cliprect=nullptr, std::optional<AtlasTileAnim> anim = std::nullopt);

    void draw() override;
};

