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
        appendIndex(buf, indices[i]);
}

void Batcher2D::appendEllipse(MeshBuffer *buf, f32 a, f32 b, const v2u &img_size, const v2f &center,
    const img::color8 &c, u32 uv_start_angle_offset)
{
    const u32 angleStep = PI / 4;
    const u32 vertexCount = 8;

    appendVertex(buf, center, c, v2f(0.5));
    for (u8 i = 0; i < vertexCount; i++) {
        u32 curAngle = i * angleStep;
        v2f relPos(a * cos(curAngle), b * sin(curAngle));

        v2f uvRelPos = v2f(
            a * cos(curAngle + uv_start_angle_offset * angleStep),
            b * sin(curAngle + uv_start_angle_offset * angleStep)) / v2f(img_size.X, img_size.Y);
        appendVertex(buf, center + relPos, c, v2f(0.5f) + uvRelPos);
    }
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
