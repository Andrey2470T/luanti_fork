#pragma once

#include "client/mesh/meshbuffer.h"
#include "Utils/Line2D.h"
#include "Utils/Rect.h"
#include "Render/Texture2D.h"

class ResourceCache;
struct Image2D9Slice;

class Batcher2D
{
public:
    static void appendVertex(
        MeshBuffer *buf, const v2f &pos,
        const img::color8 &color=img::color8(), const v2f &uv=v2f());
    static void appendLine(MeshBuffer *buf, const v2f &startPos, const v2f &endPos,
        const img::color8 &color=img::color8());
    static void appendLine(MeshBuffer *buf, const line2f &line,
        const img::color8 &color=img::color8())
    {
        appendLine(buf, line.Start, line.End, color);
    }
    // pos1, pos2, pos3 must be ordered counter-clockwise!
    static void appendTriangle(MeshBuffer *buf, const std::array<v2f, 3> &positions,
        const img::color8 &color=img::color8(), const std::array<v2f, 3> &uvs={v2f(0.0f, 0.0f), v2f(0.5f, 1.0f), v2f(1.0f, 0.0f)});
    static void appendRectangle(MeshBuffer *buf, const rectf &rect, const std::array<img::color8, 4> &colors,
        const rectf &uv={v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)});
    static void appendUnitRectangle(MeshBuffer *buf)
    {
        appendRectangle(buf, rectf(v2f(-1.0f, 1.0f), v2f(1.0f, -1.0f)), {});
    }
    static void appendEllipse(MeshBuffer *buf, f32 a, f32 b, const v2u &img_size, const v2f &center,
        const img::color8 &c, u32 uv_start_angle_offset=0);

    static void appendImageRectangle(MeshBuffer *buf, const v2u &imgSize,
        const rectf &srcRect, const rectf &destRect, const std::array<img::color8, 4> &colors, bool flip, bool toUV=false);
    static void appendImageUnitRectangle(MeshBuffer *buf, bool flip)
    {
        appendImageRectangle(buf, v2u(1), rectf(v2f(1.0f)),
            rectf(v2f(-1.0f, 1.0f), v2f(1.0f, -1.0f)), {}, flip);
    }
};
