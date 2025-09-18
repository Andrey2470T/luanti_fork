#include "extra_images.h"
#include "Render/Texture2D.h"
#include "batcher2d.h"
#include "client/render/atlas.h"
#include "client/render/rendersystem.h"
#include "sprite.h"
#include "client/render/renderer.h"

Image2D9Slice::Image2D9Slice(ResourceCache *resCache, RenderSystem *rndsys)
    : UISprite(nullptr, rndsys->getRenderer(), resCache, {UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE,
              UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE,
              UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE}, true), pool(rndsys->getPool(false))
{}
Image2D9Slice::Image2D9Slice(ResourceCache *resCache, RenderSystem *rndsys,
                             const rectf &src_rect, const rectf &dest_rect,
                             const rectf &middle_rect, img::Image *baseImg,
                             const std::array<img::color8, 4> &colors,
                             std::optional<AtlasTileAnim> anim)
    : UISprite(rndsys->getPool(false)->getAtlasByTile(baseImg, true, anim)->getTexture(), rndsys->getRenderer(), resCache,
        {UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE,
        UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE,
        UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE}, true), srcRect(src_rect),
        destRect(dest_rect), middleRect(middle_rect), pool(rndsys->getPool(false))
{
    updateRects(src_rect, dest_rect, middle_rect, baseImg, colors, nullptr, anim);
}

void Image2D9Slice::updateRects(const rectf &src_rect, const rectf &dest_rect, const rectf &middle_rect, img::Image *img,
	const std::array<img::color8, 4> &colors, const recti *clipRect, std::optional<AtlasTileAnim> anim)
{
    if (middleRect.LRC.X < 0)
        middleRect.LRC.X += srcRect.getWidth();
    if (middleRect.LRC.Y < 0)
        middleRect.LRC.Y += srcRect.getHeight();
        
    rectColors = colors;
    
	if (img && img != image ) {
        image = img;
        setTexture(pool->getAtlasByTile(image, true, anim)->getTexture());
        srcRect = pool->getTileRect(image, false, true);
    }
    if (clipRect)
        setClipRect(*clipRect);

    createSlices();
}

void Image2D9Slice::createSlice(u8 x, u8 y)
{
    v2f srcSize(srcRect.getWidth(), srcRect.getHeight());
    v2f lower_right_offset = srcSize - v2f(middleRect.LRC.X, middleRect.LRC.Y);

    switch (x) {
    case 0:
        destRect.LRC.X = destRect.ULC.X + middleRect.ULC.X;
        srcRect.LRC.X = srcRect.ULC.X + middleRect.ULC.X;
        break;

    case 1:
        destRect.ULC.X += middleRect.ULC.X;
        destRect.LRC.X -= lower_right_offset.X;
        srcRect.ULC.X += middleRect.ULC.X;
        srcRect.LRC.X -= lower_right_offset.X;
        break;

    case 2:
        destRect.ULC.X = destRect.LRC.X - lower_right_offset.X;
        srcRect.ULC.X = srcRect.LRC.X - lower_right_offset.X;
        break;
    };

    switch (y) {
    case 0:
        destRect.LRC.Y = destRect.ULC.Y + middleRect.ULC.Y;
        srcRect.LRC.Y = srcRect.ULC.Y + middleRect.ULC.Y;
        break;

    case 1:
        destRect.ULC.Y += middleRect.ULC.Y;
        destRect.LRC.Y -= lower_right_offset.Y;
        srcRect.ULC.Y += middleRect.ULC.Y;
        srcRect.LRC.Y -= lower_right_offset.Y;
        break;

    case 2:
        destRect.ULC.Y = destRect.LRC.Y - lower_right_offset.Y;
        srcRect.ULC.Y = srcRect.LRC.Y - lower_right_offset.Y;
        break;
    };

    shape->updateRectangle(y*3+x, destRect, rectColors, srcRect);
}

void Image2D9Slice::createSlices()
{
    for (u8 x = 0; x < 3; x++)
        for (u8 y = 0; y < 3; y++)
            createSlice(x, y);

    updateMesh(true);
    updateMesh(false);
}

UIRects::UIRects(RenderSystem *rndsys, u32 init_rects_count)
    : UISprite(nullptr, rndsys->getRenderer(), nullptr,
		std::vector<UIPrimitiveType>(init_rects_count, UIPrimitiveType::RECTANGLE), false)
{}

void UIRects::addRect(const rectf &rect, const std::array<img::color8, 4> &colors)
{
	shape->addRectangle(rect, colors);
	rebuildMesh();
}

void UIRects::updateRect(u32 n, const rectf &rect, const std::array<img::color8, 4> &colors)
{
	shape->updateRectangle(n, rect, colors);
	updateMesh(true);
	updateMesh(false);
}

ImageSprite::ImageSprite(RenderSystem *rndsys, ResourceCache *cache)
    : UISprite(nullptr, rndsys->getRenderer(), cache, {UIPrimitiveType::RECTANGLE}, true),
      pool(rndsys->getPool(false))
{}

void ImageSprite::update(img::Image *newImage, const rectf &rect, const std::array<img::color8, 4> &colors,
    const recti *cliprect, std::optional<AtlasTileAnim> anim)
{
    rectf tile_rect;

    if (newImage && image != newImage) {
        image = newImage;
        setTexture(pool->getAtlasByTile(image, true, anim)->getTexture());
        tile_rect = pool->getTileRect(image, false, true);
    }
    shape->updateRectangle(0, rect, colors, tile_rect);
    updateMesh(true);
    updateMesh(false);

    if (cliprect)
        setClipRect(*cliprect);
}
