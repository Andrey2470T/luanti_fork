#include "batcher3d.h"
#include "defaultVertexTypes.h"
#include "Utils/Plane3D.h"

void Batcher3D::appendVertex(MeshBuffer *buf, v3f pos,
    const img::color8 &color, const v3f &normal, const v2f &uv)
{
    curTransform.transformVect(pos);

    if (!addTCVT)
        appendSVT(buf, pos, color, normal, uv);
    else
        appendTCVT(buf, pos, color, normal, uv);
}

void Batcher3D::appendLine(MeshBuffer *buf, const v3f &startPos, const v3f &endPos,
    const img::color8 &color)
{
    appendVertex(buf, startPos, color);
    appendVertex(buf, endPos, color);
}

void Batcher3D::appendTriangle(MeshBuffer *buf, const std::array<v3f, 3> &positions,
    const img::color8 &color, const std::array<v2f, 3> &uvs)
{
    v3f normal = plane3f(positions[0], positions[1], positions[2]).Normal;
    appendVertex(buf, positions[0], color, normal, uvs[0]);
    appendVertex(buf, positions[1], color, normal, uvs[1]);
    appendVertex(buf, positions[2], color, normal, uvs[2]);
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
    const img::color8 &color, const std::array<v2f, 3> &uvs)
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

    appendTriangle(buf, positions3d, color, uvs);
}

void Batcher3D::appendFace(MeshBuffer *buf, const std::array<v3f, 4> &positions,
    const std::array<img::color8, 4> &colors, const rectf &uvs)
{
    v3f normal = plane3f(positions[0], positions[1], positions[2]).Normal;
    std::array<u32, 6> indices = {0, 1, 2, 2, 1, 3};

    appendVertex(buf, positions[0], colors[0], normal, uvs.ULC);
    appendVertex(buf, positions[1], colors[1], normal, v2f(uvs.LRC.X, uvs.ULC.Y));
    appendVertex(buf, positions[2], colors[2], normal, uvs.LRC);
    appendVertex(buf, positions[3], colors[3], normal, v2f(uvs.ULC.X, uvs.LRC.Y));

    for (u32 i = 0; i < 6; i++)
        appendIndex(buf, indices[i]);
}

void Batcher3D::appendFace(MeshBuffer *buf, const rectf &positions, const v3f &rotation,
    const std::array<img::color8, 4> &colors, const rectf &uvs)
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

    appendFace(buf, positions3d, colors, uvs);
}

void Batcher3D::appendBox(MeshBuffer *buf, const aabbf &box, const std::array<img::color8, 8> &colors,
    const std::array<rectf, 6> *uvs)
{
    // Up
    appendFace(
        buf,
        {
            v3f(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z), box.MaxEdge,
            v3f(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z), v3f(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z)
        },
        {colors[5], colors[6], colors[7], colors[4]}
    );
    // Down
    appendFace(
        buf,
        {
            box.MinEdge, v3f(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z),
            v3f(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z), v3f(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z)
        },
        {colors[0], colors[3], colors[2], colors[1]}
    );
    // Right
    appendFace(
        buf,
        {
            v3f(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z), box.MaxEdge,
            v3f(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z), v3f(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z)
        },
        {colors[7], colors[6], colors[2], colors[3]}
    );
    // Left
    appendFace(
        buf,
        {
            v3f(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z), v3f(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z),
            box.MinEdge, v3f(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z)
        },
        {colors[5], colors[4], colors[0], colors[1]}
    );
    // Back
    appendFace(
        buf,
        {
            box.MaxEdge, v3f(box.MinEdge.X, box.MaxEdge.Y, box.MaxEdge.Z),
            v3f(box.MinEdge.X, box.MinEdge.Y, box.MaxEdge.Z), v3f(box.MaxEdge.X, box.MinEdge.Y, box.MaxEdge.Z)
        },
        {colors[6], colors[5], colors[1], colors[2]}
    );
    // Front
    appendFace(
        buf,
        {
            v3f(box.MinEdge.X, box.MaxEdge.Y, box.MinEdge.Z), v3f(box.MaxEdge.X, box.MaxEdge.Y, box.MinEdge.Z),
            v3f(box.MaxEdge.X, box.MinEdge.Y, box.MinEdge.Z), box.MinEdge
        },
        {colors[4], colors[7], colors[3], colors[0]}
    );
}

void Batcher3D::appendLineBox(MeshBuffer *buf, const aabbf &box, const img::color8 &color)
{
    std::array<v3f, 8> edges;
    box.getEdges(edges.data());

    for (auto edge : edges)
        appendVertex(buf, edge, color);

    std::array<u32, 24> indices = {
        5, 1, 1, 3, 3, 7, 7, 5, 0, 2, 2, 6, 6, 4, 4, 0, 1, 0, 3, 2, 7, 6, 5, 4
    };

    for (u32 i : indices)
        appendIndex(buf, i);
}
