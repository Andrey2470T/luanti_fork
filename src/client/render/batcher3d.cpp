#include "batcher3d.h"
#include "client/mesh/defaultVertexTypes.h"
#include "Utils/Plane3D.h"
#include "client/mesh/lighting.h"
#include "client/mesh/meshoperations.h"

/*bool Batcher3D::applyFaceShading = false;
bool Batcher3D::smoothLighting = false;
img::color8 Batcher3D::curLightColor = img::white;
LightFrame Batcher3D::curLightFrame;
u8 Batcher3D::curLightSource;*/

void Batcher3D::appendVertex(MeshBuffer *buf, v3f pos,
    const img::color8 &color, const v3f &normal, v2f uv)
{

    /*img::color8 newColor = color;
    if (smoothLighting)
        newColor = blendLightColor(pos, normal, curLightFrame, curLightSource);
    else
        newColor *= curLightColor;

    if (applyFaceShading)
        MeshOperations::applyFacesShading(newColor, normal);*/

    auto vType = buf->getVAO()->getVertexType();
    if (vType.Name == "Node3D")
        appendNVT(buf, pos, color, normal, uv);
    else if (vType.Name == "TwoColorNode3D")
        appendTCNVT(buf, pos, color, normal, uv);
    else if (vType.Name == "AnimatedObject3D")
        appendAOVT(buf, pos, color, normal, uv);
    else if (vType.Name == "Skybox3D")
        appendSBVT(buf, pos, color, normal, uv);
    else
        appendSVT(buf, pos, color, normal, uv);
}

void Batcher3D::appendLine(MeshBuffer *buf, const v3f &startPos, const v3f &endPos,
    const img::color8 &color)
{
    appendVertex(buf, startPos, color);
    appendVertex(buf, endPos, color);
}

void Batcher3D::appendTriangle(MeshBuffer *buf, const std::array<v3f, 3> &positions,
    const img::color8 &color, const std::array<v2f, 3> &uvs, const std::array<v3f, 4> &normals)
{
    std::array<v3f, 4> used_normals;

    /*if (normals.has_value())
        used_normals = normals.value();
    else {
        v3f normal = plane3f(positions[0], positions[1], positions[2]).Normal;
        used_normals = {normal, normal, normal, normal};
    }*/
    appendVertex(buf, positions[0], color, used_normals[0], uvs[0]);
    appendVertex(buf, positions[1], color, used_normals[1], uvs[1]);
    appendVertex(buf, positions[2], color, used_normals[2], uvs[2]);
}

enum class RotationAxis : u8
{
    X,
    Y,
    Z
};

inline void rotateFace(v3f &pos1, v3f &pos2, v3f &pos3, v3f &pos4, RotationAxis axis, f64 degrees)
{
    switch(axis) {
    case RotationAxis::X: {
        pos1.rotateYZBy(degrees);
        pos2.rotateYZBy(degrees);
        pos3.rotateYZBy(degrees);
        pos4.rotateYZBy(degrees);
        break;
    }
    case RotationAxis::Y: {
        pos1.rotateXZBy(degrees);
        pos2.rotateXZBy(degrees);
        pos3.rotateXZBy(degrees);
        pos4.rotateXZBy(degrees);
        break;
    }
    case RotationAxis::Z: {
        pos1.rotateXYBy(degrees);
        pos2.rotateXYBy(degrees);
        pos3.rotateXYBy(degrees);
        pos4.rotateXYBy(degrees);
        break;
    }
    }
}

void Batcher3D::appendTriangle(MeshBuffer *buf, const std::array<v2f, 3> &positions, const v3f &rotation,
    const img::color8 &color, const std::array<v2f, 3> &uvs, const std::array<v3f, 4> &normals)
{
    std::array<v3f, 3> positions3d = {
        v3f(positions[0].X, positions[0].Y, 0.0f),
        v3f(positions[1].X, positions[1].Y, 0.0f),
        v3f(positions[2].X, positions[2].Y, 0.0f)
    };

    v3f fakePos;

    if (rotation.X != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], fakePos, RotationAxis::X, rotation.X);
    if (rotation.Y != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], fakePos, RotationAxis::Y, rotation.Y);
    if (rotation.Z != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], fakePos, RotationAxis::Z, rotation.Z);

    appendTriangle(buf, positions3d, color, uvs, normals);
}

void Batcher3D::appendFace(MeshBuffer *buf, const std::array<v3f, 4> &positions,
    const std::array<img::color8, 4> &colors, const rectf &uvs, const std::array<v3f, 4> &normals)
{
    //std::array<v3f, 4> used_normals;

    /* (normals.has_value())
        used_normals = normals.value();
    else {
        v3f normal = plane3f(positions[0], positions[1], positions[2]).Normal;
        used_normals = {normal, normal, normal, normal};
    }*/

    std::array<u32, 6> indices = {0, 3, 2, 0, 2, 1};

    u32 curVCount = buf->getVertexCount();
    appendVertex(buf, positions[0], colors[0], normals[0], uvs.ULC);
    appendVertex(buf, positions[1], colors[1], normals[1], v2f(uvs.LRC.X, uvs.ULC.Y));
    appendVertex(buf, positions[2], colors[2], normals[2], uvs.LRC);
    appendVertex(buf, positions[3], colors[3], normals[3], v2f(uvs.ULC.X, uvs.LRC.Y));

    if (buf->hasIBO()) {
        for (u32 i = 0; i < 6; i++)
            appendIndex(buf, curVCount+indices[i]);
    }
}

void Batcher3D::appendFace(MeshBuffer *buf, const rectf &positions, const v3f &rotation,
    const std::array<img::color8, 4> &colors, const rectf &uvs, const std::array<v3f, 4> &normals)
{
    std::array<v3f, 4> positions3d = {
        v3f(positions.ULC.X, positions.ULC.Y, 0.0f),
        v3f(positions.LRC.X, positions.ULC.Y, 0.0f),
        v3f(positions.LRC.X, positions.LRC.Y, 0.0f),
        v3f(positions.ULC.X, positions.LRC.Y, 0.0f)
    };

    if (rotation.X != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], positions3d[3], RotationAxis::X, rotation.X);
    if (rotation.Y != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], positions3d[3], RotationAxis::Y, rotation.Y);
    if (rotation.Z != 0.0f)
        rotateFace(positions3d[0], positions3d[1], positions3d[2], positions3d[3], RotationAxis::Z, rotation.Z);

    appendFace(buf, positions3d, colors, uvs, normals);
}

void Batcher3D::appendBox(MeshBuffer *buf, const aabbf &box, const std::array<img::color8, 24> &colors,
    const std::array<rectf, 6> *uvs, u8 mask)
{
    // Auto calculation of uvs
    auto calc_uv = [] (u32 face_w, u32 face_h)
    {
        v2f uv_size{1.0f, 1.0f};
        if (face_w > face_h) {
            uv_size.X = 1.0f;
            uv_size.Y = (f32)face_h / face_w;
        }
        else if (face_w < face_h) {
            uv_size.Y = 1.0f;
            uv_size.X = (f32)face_w / face_h;
        }

        rectf uv(uv_size);

        v2f uv_center = uv.getCenter();
        v2f uv_shift = v2f(0.5f) - uv_center;

        uv += uv_shift;

        return uv;
    };

    v3f box_size = box.getExtent();

    // Up
    if (mask & (1 << 0)) {
        rectf up_uv = uvs ? (*uvs)[0] : calc_uv(box_size.X, box_size.Z);
        appendFace(
            buf,
            {
                v3f(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z), box.MaxEdge,
                v3f(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z), v3f(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z)
            },
            {colors[0], colors[1], colors[2], colors[3]}, up_uv
        );
    }
    // Down
    if (mask & (1 << 1)) {
        rectf down_uv = uvs ? (*uvs)[1] : calc_uv(box_size.X, box_size.Z);
        appendFace(
            buf,
            {
                box.MinEdge, v3f(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z),
                v3f(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z), v3f(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z)
            },
            {colors[4], colors[5], colors[6], colors[7]}, down_uv
        );
    }
    // Right
    if (mask & (1 << 2)) {
        rectf right_uv = uvs ? (*uvs)[2] : calc_uv(box_size.Z, box_size.Y);
        appendFace(
            buf,
            {
                v3f(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z), box.MaxEdge,
                v3f(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z), v3f(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z)
            },
            {colors[8], colors[9], colors[10], colors[11]}, right_uv
        );
    }
    // Left
    if (mask & (1 << 3)) {
        rectf left_uv = uvs ? (*uvs)[3] : calc_uv(box_size.Z, box_size.Y);
        appendFace(
            buf,
            {
                v3f(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z), v3f(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z),
                box.MinEdge, v3f(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z)
            },
            {colors[12], colors[13], colors[14], colors[15]}, left_uv
        );
    }
    // Back
    if (mask & (1 << 4)) {
        rectf back_uv = uvs ? (*uvs)[4] : calc_uv(box_size.X, box_size.Y);
        appendFace(
            buf,
            {
                box.MaxEdge, v3f(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z),
                v3f(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z), v3f(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z)
            },
            {colors[16], colors[17], colors[18], colors[19]}, back_uv
        );
    }
    // Front
    if (mask & (1 << 5)) {
        rectf front_uv = uvs ? (*uvs)[5] : calc_uv(box_size.X, box_size.Y);
        appendFace(
            buf,
            {
                v3f(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z), v3f(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z),
                v3f(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z), box.MinEdge
            },
            {colors[20], colors[21], colors[22], colors[23]}, front_uv
        );
    }
}

void Batcher3D::appendLineBox(MeshBuffer *buf, const aabbf &box, const img::color8 &color)
{
    assert(buf->hasIBO());

    std::array<v3f, 8> edges;
    box.getEdges(edges.data());

    u32 curVCount = buf->getVertexCount();
    for (auto edge : edges)
        appendVertex(buf, edge, color);

    // Draws the lines as flattened triangles to be transparenct-sorted in the drawlist
    std::array<u32, 36> indices = {
        5, 1, 1,    1, 3, 3,    3, 7, 7,    7, 5, 5,
        0, 2, 2,    2, 6, 6,    6, 4, 4,    4, 0, 0,
        1, 0, 0,    3, 2, 2,    7, 6, 6,    5, 4, 4
    };

    for (u32 i : indices)
        appendIndex(buf, curVCount+i);
}
