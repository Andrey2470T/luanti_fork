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

typedef std::pair<u32, u32> AtlasTileAnim;

class Image2D9Slice : public UISprite
{
    rectf srcRect, destRect, middleRect;
    std::array<img::color8, 4> rectColors;
    
    AtlasPool *pool;
    img::Image *image=nullptr;
public:
    Image2D9Slice(ResourceCache *resCache, RenderSystem *rndsys);
    Image2D9Slice(ResourceCache *resCache, RenderSystem *rndsys,
                  const rectf &src_rect, const rectf &dest_rect,
                  const rectf &middle_rect, img::Image *baseImg,
                  const std::array<img::color8, 4> &colors=UISprite::defaultColors,
                  std::optional<AtlasTileAnim> anim=std::nullopt);

    void updateRects(const rectf &src_rect, const rectf &dest_rect, const rectf &middle_rect, img::Image *img=nullptr,
    	const std::array<img::color8, 4> &colors=UISprite::defaultColors, const recti *clipRect=nullptr,
    	std::optional<AtlasTileAnim> anim=std::nullopt);
    void createSlices();
private:
    void createSlice(u8 x, u8 y);
};

class UIRects : public UISprite
{
public:
    UIRects(RenderSystem *rndsys, u32 init_rects_count=0);

	void addRect(const rectf &rect, const std::array<img::color8, 4> &colors=UISprite::defaultColors);
    void addRect(const rectf &rect, const img::color8 &color)
	{
		addRect(rect, {color, color, color, color});
	}
	void updateRect(u32 n, const rectf &rect, const std::array<img::color8, 4> &colors=UISprite::defaultColors);
	void updateRect(u32 n, const rectf &rect, const img::color8 &color=img::white)
	{
		updateRect(n, rect, {color, color, color, color});
	}
};

class ImageSprite : public UISprite
{
    AtlasPool *pool;
    img::Image *image = nullptr;
public:
    ImageSprite(RenderSystem *rndsys, ResourceCache *cache);

    void update(img::Image *newImage, const rectf &rect, const std::array<img::color8, 4> &colors=UISprite::defaultColors,
        const recti *cliprect=nullptr, std::optional<AtlasTileAnim> anim=std::nullopt);
    void update(img::Image *newImage, const rectf &rect, const img::color8 &color=img::white,
        const recti *cliprect=nullptr, std::optional<AtlasTileAnim> anim=std::nullopt)
    {
        update(newImage, rect, {color, color, color, color}, cliprect);
    }

    void draw() override;
};

