#include "extra_images.h"
#include "Render/Texture2D.h"
#include "batcher2d.h"
#include "sprite.h"

img::ImageModifier *g_imgmodifier = new img::ImageModifier();

Image2D9Slice::Image2D9Slice(ResourceCache *resCache, Renderer2D *renderer,
                             const rectf &src_rect, const rectf &dest_rect,
                             const rectf &middle_rect, render::Texture2D *base_tex,
                             const std::array<img::color8, 4> &colors)
    : UISprite(base_tex, renderer, resCache, src_rect, dest_rect, colors, false), srcRect(src_rect),
        destRect(dest_rect), middleRect(middle_rect), rectColors(colors)
{
    if (middleRect.LRC.X < 0)
        middleRect.LRC.X += srcRect.getWidth();
    if (middleRect.LRC.Y < 0)
        middleRect.LRC.Y += srcRect.getHeight();
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

    shape->addRectangle(destRect, rectColors);
    Batcher2D::appendImageRectangle(mesh.get(), texture->getSize(), srcRect, destRect, rectColors, false);
}

void Image2D9Slice::createSlices()
{
    for (u8 x = 0; x < 3; x++)
        for (u8 y = 0; y < 3; y++)
            createSlice(x, y);

    flush();
}
