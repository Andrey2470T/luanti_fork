#include "meshcreator.h"
#include "defaultVertexTypes.h"
#include "Utils/Plane3D.h"

MeshBuffer *MeshCreator3D::createPoint(const v3f &pos)
{
    MeshBuffer *buf = new MeshBuffer(MeshBufferType::VERTEX_INDEX, State.CurVT);

    appendVertex(buf, pos);

    return buf;
}

MeshBuffer *MeshCreator3D::createLine(
    const v3f &startPos, const v3f &endPos)
{
    MeshBuffer *buf = new MeshBuffer(MeshBufferType::VERTEX_INDEX, State.CurVT);

    appendLine(buf, startPos, endPos);

    return buf;
}

MeshBuffer *MeshCreator3D::createLine(const line3f &line)
{
    return createLine(line.Start, line.End);
}

MeshBuffer *MeshCreator3D::createTriangle(
    const v3f &pos1, const v3f &pos2, const v3f &pos3)
{
    MeshBuffer *buf = new MeshBuffer(MeshBufferType::VERTEX_INDEX, State.CurVT);

    appendTriangle(buf, pos1, pos2, pos3);

    return buf;
}

MeshBuffer *MeshCreator3D::createFace(
    const v3f &pos1, const v3f &pos2, const v3f &pos3, const v3f &pos4)
{
    MeshBuffer *buf = new MeshBuffer(MeshBufferType::VERTEX_INDEX, State.CurVT);

    appendFace(buf, pos1, pos2, pos3, pos4);

    return buf;
}

MeshBuffer *MeshCreator3D::createFace()
{
    return createFace(
        v3f(-0.5f, -0.5f, 0.0f), v3f(0.5f, -0.5f, 0.0f),
        v3f(-0.5f, 0.5f, 0.0f), v3f(0.5f, 0.5f, 0.0f));
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

MeshBuffer *MeshCreator3D::createBox()
{
    MeshBuffer *buf = new MeshBuffer(MeshBufferType::VERTEX_INDEX, State.CurVT);

    v3f pos1(-0.5f, -0.5f, -0.5f);
    v3f pos2(0.5f, -0.5f, -0.5f);
    v3f pos3(-0.5f, 0.5f, -0.5f);
    v3f pos4(0.5f, 0.5f, -0.5f);

    // Up
    rotateFace(pos1, pos2, pos3, pos4, RotationAxis::X, -90.0);
    appendFace(buf, pos1, pos2, pos3, pos4);

    // Down
    rotateFace(pos1, pos2, pos3, pos4, RotationAxis::X, 180.0);
    appendFace(buf, pos1, pos2, pos3, pos4);

    rotateFace(pos1, pos2, pos3, pos4, RotationAxis::X, -90.0);

    // Right
    rotateFace(pos1, pos2, pos3, pos4, RotationAxis::Y, 90.0);
    appendFace(buf, pos1, pos2, pos3, pos4);

    // Left
    rotateFace(pos1, pos2, pos3, pos4, RotationAxis::Y, 180.0);
    appendFace(buf, pos1, pos2, pos3, pos4);

    // Back
    rotateFace(pos1, pos2, pos3, pos4, RotationAxis::Y, -90.0);
    appendFace(buf, pos1, pos2, pos3, pos4);

    // Front
    rotateFace(pos1, pos2, pos3, pos4, RotationAxis::Y, -180.0);
    appendFace(buf, pos1, pos2, pos3, pos4);

    return buf;
}

void MeshCreator3D::appendVertex(MeshBuffer *buf, const v3f &pos,
    v3f normal, v2f uv)
{
    v3f pos_c = pos;
    State.CurTransform.transformVect(pos_c);

    if (!State.ContainsHW)
        appendSVT(buf, pos_c, State.CurColor, normal, uv);
    else
        appendTCVT(buf, pos_c, State.CurColor, normal, uv);
}

void MeshCreator3D::appendLine(MeshBuffer *buf, const v3f &startPos, const v3f &endPos)
{
    appendVertex(buf, startPos);
    appendVertex(buf, endPos);
}

void MeshCreator3D::appendTriangle(MeshBuffer *buf, const v3f &pos1, const v3f &pos2, const v3f &pos3)
{
    v3f normal = plane3f(pos1, pos2, pos3).Normal;
    appendVertex(buf, pos1, normal, v2f(0.0f, 0.0f));
    appendVertex(buf, pos2, normal, v2f(1.0f, 0.0f));
    appendVertex(buf, pos3, normal, v2f(0.5f, 1.0f));
}

void MeshCreator3D::appendFace(MeshBuffer *buf, const v3f &pos1, const v3f &pos2, const v3f &pos3, const v3f &pos4)
{
    v3f normal = plane3f(pos1, pos2, pos3).Normal;
    std::array<u32, 6> indices = {0, 1, 2, 2, 1, 3};

    appendVertex(buf, pos1, normal, v2f(0.0f, 0.0f));
    appendVertex(buf, pos2, normal, v2f(1.0f, 0.0f));
    appendVertex(buf, pos3, normal, v2f(0.0f, 1.0f));
    appendVertex(buf, pos4, normal, v2f(1.0f, 1.0f));

    for (u32 i = 0; i < 6; i++)
        buf->setIndexAt(indices[i]);
}
