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

class Image2D9Slice : public UISprite
{
    rectf srcRect, destRect, middleRect;
    std::array<img::color8, 4> rectColors;
public:
    Image2D9Slice(ResourceCache *resCache, Renderer *renderer,
                  const rectf &src_rect, const rectf &dest_rect,
                  const rectf &middle_rect, render::Texture2D *base_tex,
                  const std::array<img::color8, 4> &colors);
private:
    void createSlice(u8 x, u8 y);
    void createSlices();
};

