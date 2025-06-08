#include "batcher2d.h"
#include "extra_images.h"
#include "client/render/defaultVertexTypes.h"

void Batcher2D::appendVertex(MeshBuffer *buf, const v2f &pos, const img::color8 &color, const v2f &uv)
{
    appendVT2D(buf, pos, color, uv);
}

void Batcher2D::appendLine(MeshBuffer *buf, const v2f &startPos, const v2f &endPos, const img::color8 &color)
{
    appendVertex(buf, startPos, color);
    appendVertex(buf, endPos, color);
}

void Batcher2D::appendTriangle(MeshBuffer *buf, const std::array<v2f, 3> &positions,
    const img::color8 &color, const std::array<v2f, 3> &uvs)
{
    appendVertex(buf, positions[0], color, uvs[0]);
    appendVertex(buf, positions[1], color, uvs[1]);
    appendVertex(buf, positions[2], color, uvs[2]);
}

void Batcher2D::appendRectangle(MeshBuffer *buf, const rectf &rect, const std::array<img::color8, 4> &colors,
    const rectf &uv)
{
    std::array<u32, 6> indices = {0, 1, 2, 2, 3, 0};

    appendVertex(buf, rect.ULC, colors[0], uv.ULC);
    appendVertex(buf, v2f(rect.LRC.X, rect.ULC.Y), colors[1], v2f(uv.LRC.X, uv.ULC.Y));
    appendVertex(buf, rect.LRC, colors[2], uv.LRC);
    appendVertex(buf, v2f(rect.ULC.X, rect.LRC.Y), colors[3], v2f(uv.ULC.X, uv.LRC.Y));

    for (u32 i = 0; i < 6; i++)
        buf->setIndexAt(indices[i]);
}

void Batcher2D::appendImageRectangle(MeshBuffer *buf, const v2u &imgSize,
    const rectf &srcRect, const rectf &destRect, const std::array<img::color8, 4> &colors, bool flip)
{
    f32 invW = 1.0f / imgSize.X;
    f32 invH = 1.0f / imgSize.Y;

    f32 topY = flip ? srcRect.LRC.Y : srcRect.ULC.Y;
    f32 bottomY = flip ? srcRect.ULC.Y : srcRect.LRC.Y;

    rectf tcoords{
        v2f(srcRect.ULC.X * invW, topY * invH),
        v2f(srcRect.LRC.X * invW, bottomY * invH)
    };

    appendRectangle(buf, destRect, colors, tcoords);
}

Image2D9Slice *Batcher2D::createImage2D9Slice(
    ResourceCache *res, Renderer2D *rnd,
    const rectf &src_rect, const rectf &dest_rect,
    const rectf &middle_rect, render::Texture2D *base_tex,
    const RectColors &colors)
{
    Image2D9Slice *slicedImg = new Image2D9Slice(this, res, rnd, src_rect, dest_rect, middle_rect, base_tex, colors);

    for (u8 x = 0; x < 3; x++)
        for (u8 y = 0; y < 3; y++)
            slicedImg->createSlice(x, y);

    return slicedImg;
}
