#include "batcher2d.h"
#include "extra_images.h"
#include "client/mesh/defaultVertexTypes.h"

void Batcher2D::vertex(MeshBuffer *buf, const v2f &pos, const img::color8 &color, const v2f &uv)
{
    VT2D(buf, pos, color, uv);
}

void Batcher2D::index(MeshBuffer *buf, u32 index)
{
    buf->setIndexAt(index);
}

void Batcher2D::line(MeshBuffer *buf, const v2f &startPos, const v2f &endPos, const img::color8 &color)
{
    if (buf->hasIBO()) {
        u32 curVIndex = buf->getVertexOffset();
        index(buf, curVIndex);
        index(buf, curVIndex+1);
    }

    vertex(buf, startPos, color);
    vertex(buf, endPos, color);
}

void Batcher2D::triangle(MeshBuffer *buf, const std::array<v2f, 3> &positions,
    const img::color8 &color, const std::array<v2f, 3> &uvs)
{
    if (buf->hasIBO()) {
        u32 curVIndex = buf->getVertexOffset();
        index(buf, curVIndex);
        index(buf, curVIndex+1);
        index(buf, curVIndex+2);
    }

    vertex(buf, positions[0], color, uvs[0]);
    vertex(buf, positions[1], color, uvs[1]);
    vertex(buf, positions[2], color, uvs[2]);
}

void Batcher2D::rectangle(MeshBuffer *buf, const rectf &rect, const std::array<img::color8, 4> &colors,
    const rectf &uv)
{
    if (buf->hasIBO()) {
        std::array<u32, 6> indices = {0, 3, 2, 0, 2, 1};

        u32 curVIndex = buf->getVertexOffset();
        for (u32 i = 0; i < 6; i++)
            index(buf, curVIndex+indices[i]);
    }

    vertex(buf, rect.ULC, colors[0], uv.ULC);
    vertex(buf, v2f(rect.LRC.X, rect.ULC.Y), colors[1], v2f(uv.LRC.X, uv.ULC.Y));
    vertex(buf, rect.LRC, colors[2], uv.LRC);
    vertex(buf, v2f(rect.ULC.X, rect.LRC.Y), colors[3], v2f(uv.ULC.X, uv.LRC.Y));
}

void Batcher2D::ellipse(MeshBuffer *buf, f32 a, f32 b, const v2u &img_size, const v2f &center,
    const img::color8 &c, u32 uv_start_angle_offset)
{
    const u32 angleStep = PI / 4;
    const u32 vertexCount = 8;

    u32 curVIndex = buf->getVertexOffset();
    vertex(buf, center, c, v2f(0.5));

    if (buf->hasIBO())
        index(buf, curVIndex);
    for (u8 i = 0; i < vertexCount; i++) {
        u32 curAngle = i * angleStep;
        v2f relPos(a * cos(curAngle), b * sin(curAngle));

        v2f uvRelPos = v2f(
            a * cos(curAngle + uv_start_angle_offset * angleStep),
            b * sin(curAngle + uv_start_angle_offset * angleStep)) / v2f(img_size.X, img_size.Y);
        vertex(buf, center + relPos, c, v2f(0.5f) + uvRelPos);

        if (buf->hasIBO())
            index(buf, curVIndex+i+1);
    }
}

void Batcher2D::imageRectangle(MeshBuffer *buf, const v2u &imgSize,
    const rectf &srcRect, const rectf &destRect, const std::array<img::color8, 4> &colors, bool flip)
{
    f32 topY = flip ? srcRect.LRC.Y : srcRect.ULC.Y;
    f32 bottomY = flip ? srcRect.ULC.Y : srcRect.LRC.Y;

    rectf tcoords;

    f32 invW = 1.0f / imgSize.X;
    f32 invH = 1.0f / imgSize.Y;

    tcoords.ULC = v2f(srcRect.ULC.X * invW, topY * invH);
    tcoords.LRC = v2f(srcRect.LRC.X * invW, bottomY * invH);

    rectangle(buf, destRect, colors, tcoords);
}
