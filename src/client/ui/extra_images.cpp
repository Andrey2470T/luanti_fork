#include "extra_images.h"
#include "Render/Texture2D.h"
#include "Image/ImageFilters.h"
#include "settings.h"
#include "client/media/resource.h"
#include "batcher2d.h"
#include "sprite.h"

img::ImageModifier *g_imgmodifier = new img::ImageModifier();

ImageFiltered::ImageFiltered(ResourceCache *resCache, render::Texture2D *tex)
    : cache(resCache), output_tex(tex), input_tex(tex)
{}

ImageFiltered::~ImageFiltered()
{
    if (filtered)
        cache->cacheResource<render::Texture2D>(ResourceType::TEXTURE, output_tex);
}

void ImageFiltered::filter(const std::string &name, const v2i &srcSize, const v2i &destSize)
{
	if (!g_settings->getBool("gui_scaling_filter"))
	    return;
    if (filtered)
        cache->clearResource<render::Texture2D>(ResourceType::TEXTURE, output_tex);
    img::Image *img = input_tex->downloadData().at(0);
    img::Image *filteredImg = img::applyCleanScalePowerOf2(img, srcSize, destSize, g_imgmodifier);
    output_tex = new render::Texture2D(
        name, std::unique_ptr<img::Image>(filteredImg), input_tex->getParameters());

    filtered = true;
    cache->cacheResource<render::Texture2D>(ResourceType::TEXTURE, output_tex);
}

Image2D9Slice::Image2D9Slice(MeshCreator2D *creator2d,
                             ResourceCache *resCache, Renderer2D *renderer,
                             const rectf &src_rect, const rectf &dest_rect,
                             const rectf &middle_rect, render::Texture2D *base_tex,
                             const std::array<img::color8, 4> &colors)
    : creator2D(creator2d), cache(resCache), renderer2d(renderer), srcRect(src_rect),
    destRect(dest_rect), middleRect(middle_rect), baseTex(base_tex), rectColors(colors)
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

    slices[y*3+x] = std::make_unique<UISprite>(
        baseTex, renderer2d, cache, srcRect, destRect, std::array<img::color8, 4>(), g_settings->getBool("gui_scaling_filter"));
}

void Image2D9Slice::drawSlice(u8 i) const
{
    if (!slices[i])
        return;

    slices[i]->draw();
}
