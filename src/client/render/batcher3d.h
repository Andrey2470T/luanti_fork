#pragma once

#include "client/mesh/meshbuffer.h"
#include "Utils/Matrix4.h"

struct LightFrame;

enum Batcher3DVertexType
{
    B3DVT_SVT = 0,
    B3DVT_NVT,
    B3DVT_TCNVT,
    B3DVT_AOVT
};

enum class BoxFaces
{
    TOP = 0,
    BOTTOM,
    RIGHT,
    LEFT,
    BACK,
    FRONT
};

// Class creating meshbuffers of various 3D shapes (point, line, quad, cube and etc)
class Batcher3D
{
public:
    // lighting params
    /*static bool applyFaceShading;
    static bool smoothLighting;
    static img::color8 curLightColor;
    static LightFrame curLightFrame;
    static u8 curLightSource;*/

    static void vertex(
        MeshBuffer *buf,
        v3f pos,
        const img::color8 &color=img::color8(),
        const v3f &normal=v3f(),
        v2f uv=v2f());
    static void index(MeshBuffer *buf, u32 index);
    static void line(
        MeshBuffer *buf,
        const v3f &startPos, const v3f &endPos,
        const img::color8 &color=img::color8());
    static void line(
        MeshBuffer *buf,
        const line3f &pos,
        const img::color8 &color=img::color8())
    {
        line(buf, pos.Start, pos.End, color);
    }
    // Vertices attributes must be ordered clockwise!
    // The first triangle vertex is the left
    // The first face vertex is the upper left
    static void triangle(
        MeshBuffer *buf,
        const std::array<v3f, 3> &positions,
        const img::color8 &color=img::color8(),
        const std::array<v2f, 3> &uvs={v2f(0.0f, 0.0f), v2f(0.5f, 1.0f), v2f(1.0f, 0.0f)},
        const std::array<v3f, 4> &normals={});
    // 'rotation' in degrees!
    static void triangle(
        MeshBuffer *buf,
        const std::array<v2f, 3> &positions,
        const v3f &rotation,
        const img::color8 &color=img::color8(),
        const std::array<v2f, 3> &uvs={v2f(0.0f, 0.0f), v2f(0.5f, 1.0f), v2f(1.0f, 0.0f)},
        const std::array<v3f, 4> &normals={});
    static void face(
        MeshBuffer *buf,
        const std::array<v3f, 4> &positions,
        const std::array<img::color8, 4> &colors,
        const rectf &uvs={v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)},
        const std::array<v3f, 4> &normals={});
    static void face(
        MeshBuffer *buf,
        const rectf &positions,
        const v3f &rotation,
        const std::array<img::color8, 4> &colors,
        const rectf &uvs={v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)},
        const std::array<v3f, 4> &normals={});
    static void unitFace(MeshBuffer *buf, const std::array<img::color8, 4> &colors)
    {
        face(buf, {v3f(-1.0f, 1.0f, 0.0f), v3f(1.0f, 1.0f, 0.0f), v3f(1.0f, -1.0f, 0.0f), v3f(-1.0f, -1.0f, 0.0f)}, colors);
    }

    static void box(
        MeshBuffer *buf,
        const aabbf &box,
        const std::array<img::color8, 24> &colors,
        const std::array<rectf, 6> *uvs=nullptr,
        u8 mask=0xff);
    static void unitBox(MeshBuffer *buf, const std::array<img::color8, 24> &colors)
    {
        box(buf, {v3f(-1.0f, -1.0f, -1.0f), v3f(1.0f, 1.0f, 1.0f)}, colors);
    }
    static void boxFace(
        MeshBuffer *buf,
        BoxFaces faceNum,
        const aabbf &box,
        const std::array<img::color8, 24> &colors,
        const std::array<rectf, 6> *uvs=nullptr,
        u8 mask=0xff);
    static void lineBox(
        MeshBuffer *buf,
        const aabbf &box,
        const img::color8 &color=img::color8());
};
