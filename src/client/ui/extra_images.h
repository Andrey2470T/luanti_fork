#pragma once

#include "Utils/Rect.h"
#include <array>
#include "Image/ImageModifier.h"

namespace render
{
    class Texture2D;
}

class MeshCreator2D;
class MeshBuffer;
class ResourceCache;
class Renderer2D;


extern img::ImageModifier *g_imgmodifier;

struct ImageFiltered
{
    render::Texture2D *output_tex;
    render::Texture2D *input_tex;

    ImageFiltered(render::Texture2D *tex);

    void filter(const std::string &name, const v2i &srcSize, const v2i &destSize);
};

struct Image2D9Slice
{
    MeshCreator2D *creator2D;
    ResourceCache *cache;
    rectu srcRect, destRect, middleRect;
    render::Texture2D *baseTex;
    std::array<img::color8, 4> rectColors;

    std::array<MeshBuffer*, 9> slices;
    std::array<render::Texture2D*, 9> textures;

    Image2D9Slice(MeshCreator2D *creator2d,
                  ResourceCache *resCache, const rectu &src_rect, const rectu &dest_rect,
                  const rectu &middle_rect, render::Texture2D *base_tex,
                  const std::array<img::color8, 4> &colors);

    void createSlice(u8 x, u8 y);
    void drawSlice(Renderer2D *rnd, u8 i);
};

