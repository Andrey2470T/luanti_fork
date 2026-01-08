#include "extra_images.h"
#include "Render/Texture2D.h"
#include "batcher2d.h"
#include "client/render/atlas.h"
#include "client/render/rendersystem.h"
#include "sprite.h"
#include "client/render/renderer.h"

Image2D9Slice::Image2D9Slice(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool, u32 depthLevel)
    : UISprite(resCache, drawBatch, depthLevel), guiPool(pool)
{}
Image2D9Slice::Image2D9Slice(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool,
                             const rectf &src_rect, const rectf &dest_rect,
                             const rectf &middle_rect, img::Image *baseImg,
                             const std::array<img::color8, 4> &colors,
                             std::optional<AtlasTileAnim> anim, u32 depthLevel)
    : UISprite(resCache, drawBatch, depthLevel), srcRect(src_rect),
        destRect(dest_rect), middleRect(middle_rect), guiPool(pool)
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
        srcRect = guiPool->getTileRect(image, false, true);
    }
    if (clipRect)
        setClipRect(*clipRect);

    createSlices();

    changed = true;
}

void Image2D9Slice::appendToBatch()
{
    if (!changed)
        return;
    auto tex = guiPool->getAtlasByTile(image)->getTexture();
    drawBatch->addSpriteChunks(this, {{tex, clipRect, 9}});

    changed = false;
}
void Image2D9Slice::updateBatch()
{
    appendToBatch();
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

    shape.addRectangle(destRect, rectColors, srcRect);
}

void Image2D9Slice::createSlices()
{
    clear();

    for (u8 x = 0; x < 3; x++)
        for (u8 y = 0; y < 3; y++)
            createSlice(x, y);
}

UIRects::UIRects(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool,
    u32 init_rects_count, u32 depthLevel)
    : UISprite(resCache, drawBatch, depthLevel), guiPool(pool)
{
    for (u32 k = 0; k < init_rects_count; k++)
        shape.addRectangle({}, UISprite::defaultColors);

    changed = true;
}
UIRects::UIRects(ResourceCache *resCache, SpriteDrawBatch *drawBatch, AtlasPool *pool,
    const std::vector<TexturedRect> &rects, u32 depthLevel)
    : UISprite(resCache, drawBatch, depthLevel), guiPool(pool)
{
    for (auto &rect : rects) {
        rectf tileRect = rect.image ? guiPool->getTileRect(rect.image, false, true) : rectf();
        shape.addRectangle(rect.area, rect.colors, tileRect);
        images.push_back(rect.image);
    }

    changed = true;
}

void UIRects::addRect(const TexturedRect &rect, std::optional<rectf> srcRect)
{
    rectf tileRect = rect.image ? guiPool->getTileRect(rect.image, false, true) : rectf();

    if (srcRect.has_value())
        tileRect = srcRect.value();
    shape.addRectangle(rect.area, rect.colors, tileRect);
    images.push_back(rect.image);

    changed = true;
}

void UIRects::updateRect(u32 n, const TexturedRect &rect, std::optional<rectf> srcRect)
{
    rectf tileRect = rect.image ? guiPool->getTileRect(rect.image, false, true) : rectf();

    if (srcRect.has_value())
        tileRect = srcRect.value();
    shape.updateRectangle(n, rect.area, rect.colors, tileRect);

    if (images.at(n) != rect.image)
        images.at(n) = rect.image;

    changed = true;
}

void UIRects::appendToBatch()
{
    if (!changed)
        return;
    std::vector<SpriteDrawChunk> chunks;
    for (u32 k = 0; k < shape.getPrimitiveCount(); k++) {
        render::Texture2D *tex = images[k] ? guiPool->getAtlasByTile(images[k])->getTexture() : nullptr;
        chunks.emplace_back(tex, clipRect, 1);
    }
    drawBatch->addSpriteChunks(this, chunks);

    changed = false;
}
void UIRects::updateBatch()
{
    appendToBatch();
}
