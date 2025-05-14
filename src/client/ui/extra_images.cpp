#include "extra_images.h"
#include "Render/Texture2D.h"
#include "Image/ImageFilters.h"
#include "settings.h"
#include "client/media/resource.h"
#include "meshcreator2d.h"

img::ImageModifier *g_imgmodifier = new img::ImageModifier();

ImageFiltered::ImageFiltered(render::Texture2D *tex)
    : input_tex(tex)
{}

void ImageFiltered::filter(const std::string &name, const v2i &srcSize, const v2i &destSize)
{
    img::Image *img = input_tex->downloadData().at(0);
    img::Image *filteredImg = img::applyCleanScalePowerOf2(img, srcSize, destSize, g_imgmodifier);
    output_tex = new render::Texture2D(
        name, std::unique_ptr<img::Image>(filteredImg), input_tex->getParameters());
}

Image2D9Slice::Image2D9Slice(MeshCreator2D *creator2d,
                             ResourceCache *resCache, const rectu &src_rect, const rectu &dest_rect,
                             const rectu &middle_rect, render::Texture2D *base_tex,
                             const std::array<img::color8, 4> &colors)
    : creator2D(creator2d), cache(resCache), srcRect(src_rect),
    destRect(dest_rect), middleRect(middle_rect), baseTex(base_tex), rectColors(colors)
{
    if (middleRect.LRC.X < 0)
        middleRect.LRC.X += srcRect.getWidth();
    if (middleRect.LRC.Y < 0)
        middleRect.LRC.Y += srcRect.getHeight();
}

void Image2D9Slice::createSlice(u8 x, u8 y)
{
    v2i srcSize(srcRect.getWidth(), srcRect.getHeight());
    v2i lower_right_offset = srcSize - v2i(middleRect.LRC.X, middleRect.LRC.Y);

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

    v2i destSize(destRect.getWidth(), destRect.getHeight());

    render::Texture2D *sliceTex;

    if (g_settings->getBool("gui_scaling_filter")) {
        ImageFiltered *filteredTex = new ImageFiltered(baseTex);

        std::ostringstream texName("Image2D9Slice:");
        texName << x << "," << y;
        filteredTex->filter(texName.str(), srcSize, destSize);
        sliceTex = filteredTex->output_tex;
        delete filteredTex;
    }
    else
        sliceTex = baseTex->copy();

    cache->cacheResource(ResourceType::TEXTURE, sliceTex, sliceTex->getName());
    auto sliceSize = sliceTex->getSize();
    rectf srcf(v2f(sliceSize.X, sliceSize.Y));
    rectf destf(v2f(destRect.ULC.X, destRect.ULC.Y), v2f(destRect.LRC.X, destRect.LRC.Y));
    MeshBuffer *sliceRect = creator2D->createImageRectangle(sliceSize, srcf, destf, rectColors, false);
    cache->cacheResource(ResourceType::MESH, sliceRect, "");

    if (slices[y*3+x])
        cache->clearResource(ResourceType::TEXTURE, slices[y*3+x]);
    if (textures[y*3+x])
        cache->clearResource(ResourceType::MESH, slices[y*3+x]);
    slices[y*3+x] = sliceRect;
    textures[y*3+x] = sliceTex;
}

void Image2D9Slice::drawSlice(Renderer2D *rnd, u8 i)
{
    if (!slices[i] || !textures[i])
        return;

    rnd->drawImage(slices[i], textures[i]);
}
