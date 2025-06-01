#pragma once

#include "Utils/Rect.h"
#include <array>
#include <memory>
#include "Image/ImageModifier.h"

namespace render
{
    class Texture2D;
}

class MeshCreator2D;
class MeshBuffer;
class ResourceCache;
class Renderer2D;
class UISprite;

extern img::ImageModifier *g_imgmodifier;

struct ImageFiltered
{
    ResourceCache *cache;
    render::Texture2D *output_tex;
    render::Texture2D *input_tex;

    ImageFiltered(ResourceCache *resCache, render::Texture2D *tex);

    void filter(const std::string &name, const v2i &srcSize, const v2i &destSize);
};

struct Image2D9Slice
{
    MeshCreator2D *creator2D;
    ResourceCache *cache;
    Renderer2D *renderer2d;
    rectf srcRect, destRect, middleRect;
    render::Texture2D *baseTex;
    std::array<img::color8, 4> rectColors;

    std::array<std::unique_ptr<UISprite>, 9> slices;

    Image2D9Slice(MeshCreator2D *creator2d,
                  ResourceCache *resCache, Renderer2D *renderer,
                  const rectf &src_rect, const rectf &dest_rect,
                  const rectf &middle_rect, render::Texture2D *base_tex,
                  const std::array<img::color8, 4> &colors);

    void createSlice(u8 x, u8 y);
    void drawSlice(u8 i) const;
};

