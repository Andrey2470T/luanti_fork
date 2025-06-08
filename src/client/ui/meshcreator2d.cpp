#include "meshcreator2d.h"
#include "extra_images.h"

MeshBuffer *MeshCreator2D::createLine(
    const v2f &startPos, const v2f &endPos, const img::color8 &color)
{
    MeshBuffer *buf = new MeshBuffer(false, CurVT);

    appendLine(buf, startPos, endPos, color);

    return buf;
}

MeshBuffer *MeshCreator2D::createLine(const line2f &line, const img::color8 &color)
{
    return createLine(line.Start, line.End, color);
}

MeshBuffer *MeshCreator2D::createTriangle(
    const v2f &pos1, const v2f &pos2, const v2f &pos3, const img::color8 &color,
    const v2f &uv1, const v2f &uv2, const v2f &uv3)
{
    MeshBuffer *buf = new MeshBuffer(false, CurVT);

    appendTriangle(buf, pos1, pos2, pos3, color);

    return buf;
}

MeshBuffer *MeshCreator2D::createRectangle(
    const rectf &rect, const std::array<img::color8, 4> &colors,
    const std::array<v2f, 4> &uvs)
{
    MeshBuffer *buf = new MeshBuffer(true, CurVT);

    appendRectangle(buf, rect, colors, uvs);

    return buf;
}

MeshBuffer *MeshCreator2D::createUnitRectangle()
{
    return createRectangle(
        rectf(v2f(-1.0f, 1.0f), v2f(1.0f, -1.0f)), {});
}

MeshBuffer *MeshCreator2D::createImageRectangle(
    const v2u &imgSize, const rectf &srcRect, const rectf &destRect,
    const std::array<img::color8, 4> &colors, bool flip)
{
    f32 invW = 1.0f / imgSize.X;
    f32 invH = 1.0f / imgSize.Y;

    f32 topY = flip ? srcRect.LRC.Y : srcRect.ULC.Y;
    f32 bottomY = flip ? srcRect.ULC.Y : srcRect.LRC.Y;

    std::array<v2f, 4> tcoords{
        v2f(srcRect.ULC.X * invW, topY * invH),
        v2f(srcRect.LRC.X * invW, topY * invH),
        v2f(srcRect.LRC.X * invW, bottomY * invH),
        v2f(srcRect.ULC.X * invW, bottomY * invH)
    };

    return createRectangle(destRect, colors, tcoords);
}

MeshBuffer *MeshCreator2D::createImageUnitRectangle(bool flip)
{
    return createImageRectangle(v2u(1), rectf(v2f(1.0f)),
        rectf(v2f(-1.0f, 1.0f), v2f(1.0f, -1.0f)), {img::color8(), img::color8(), img::color8(), img::color8()}, flip);
}

Image2D9Slice *MeshCreator2D::createImage2D9Slice(
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

void MeshCreator2D::appendVertex(MeshBuffer *buf, const v2f &pos, const img::color8 &color, const v2f &uv)
{
    appendVT2D(buf, pos, color, uv);
}

void MeshCreator2D::appendLine(MeshBuffer *buf, const v2f &startPos, const v2f &endPos, const img::color8 &color)
{
    appendVertex(buf, startPos, color);
    appendVertex(buf, endPos, color);
}

void MeshCreator2D::appendTriangle(MeshBuffer *buf, const v2f &pos1, const v2f &pos2, const v2f &pos3, const img::color8 &color,
    const v2f &uv1, const v2f &uv2, const v2f &uv3)
{
    appendVertex(buf, pos1, color, uv1);
    appendVertex(buf, pos2, color, uv2);
    appendVertex(buf, pos3, color, uv3);
}

void MeshCreator2D::appendRectangle(MeshBuffer *buf, const rectf &rect, const std::array<img::color8, 4> &colors,
    const std::array<v2f, 4> &uvs)
{
    std::array<u32, 6> indices = {0, 1, 2, 2, 3, 0};

    appendVertex(buf, rect.ULC, colors[0], uvs[0]);
    appendVertex(buf, v2f(rect.LRC.X, rect.ULC.Y), colors[1], uvs[1]);
    appendVertex(buf, rect.LRC, colors[2], uvs[2]);
    appendVertex(buf, v2f(rect.ULC.X, rect.LRC.Y), colors[3], uvs[3]);

    for (u32 i = 0; i < 6; i++)
        buf->setIndexAt(indices[i]);
}
