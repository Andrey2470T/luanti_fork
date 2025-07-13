#pragma once

#include "client/mesh/meshbuffer.h"
#include "Utils/Matrix4.h"

// Class creating meshbuffers of various 3D shapes (point, line, quad, cube and etc)
class Batcher3D
{
public:
    static bool addTCVT;
    static matrix4 curTransform;

    static void appendVertex(MeshBuffer *buf, v3f pos,
        const img::color8 &color=img::color8(), const v3f &normal=v3f(), const v2f &uv=v2f());

    static void appendLine(MeshBuffer *buf, const v3f &startPos, const v3f &endPos,
        const img::color8 &color=img::color8());
    static void appendLine(MeshBuffer *buf, const line3f &line,
        const img::color8 &color=img::color8())
    {
        appendLine(buf, line.Start, line.End, color);
    }

    // Vertices attributes must be ordered clockwise!
    // The first triangle vertex is the left
    // The first face vertex is the upper left
    static void appendTriangle(MeshBuffer *buf, const std::array<v3f, 3> &positions,
        const img::color8 &color=img::color8(), const std::array<v2f, 3> &uvs={v2f(0.0f, 0.0f), v2f(0.5f, 1.0f), v2f(1.0f, 0.0f)});
    // 'rotation' in degrees!
    static void appendTriangle(MeshBuffer *buf, const std::array<v2f, 3> &positions, const v3f &rotation,
        const img::color8 &color=img::color8(), const std::array<v2f, 3> &uvs={v2f(0.0f, 0.0f), v2f(0.5f, 1.0f), v2f(1.0f, 0.0f)});

    static void appendFace(MeshBuffer *buf, const std::array<v3f, 4> &positions,
        const std::array<img::color8, 4> &colors, const rectf &uvs={v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)});
    static void appendFace(MeshBuffer *buf, const rectf &positions, const v3f &rotation,
        const std::array<img::color8, 4> &colors, const rectf &uvs={v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)});
    static void appendUnitFace(MeshBuffer *buf, const std::array<img::color8, 4> &colors)
    {
        appendFace(buf, {v3f(-1.0f, 1.0f, 0.0f), v3f(1.0f, 1.0f, 0.0f), v3f(1.0f, -1.0f, 0.0f), v3f(-1.0f, -1.0f, 0.0f)}, colors);
    }

    static void appendBox(MeshBuffer *buf, const aabbf &box, const std::array<img::color8, 8> &colors,
        const std::array<rectf, 6> *uvs=nullptr);
    static void appendUnitBox(MeshBuffer *buf, const std::array<img::color8, 8> &colors)
    {
        appendBox(buf, {v3f(-1.0f, -1.0f, -1.0f), v3f(1.0f, 1.0f, 1.0f)}, colors);
    }
    static void appendLineBox(MeshBuffer *buf, const aabbf &box, const img::color8 &color=img::color8());
};
