#pragma once

#include "BasicIncludes.h"
#include "meshbuffer.h"
#include "Utils/Matrix4.h"

struct MeshCreator3DState
{
    render::VertexTypeDescriptor CurVT = render::DefaultVType;
    bool ContainsHW = false;

    matrix4 CurTransform;

    img::color8 CurColor;
};
// Class creating meshbuffers of various 3D shapes (point, line, quad, cube and etc)
class MeshCreator3D
{
    MeshCreator3DState State;

public:
    MeshCreator3D() = default;

    MeshBuffer *createPoint(const v3f &pos);

    MeshBuffer *createLine(
        const v3f &startPos, const v3f &endPos);

    MeshBuffer *createLine(const line3f &line);

    // pos1, pos2, pos3 must be ordered counter-clockwise!
    MeshBuffer *createTriangle(
        const v3f &pos1, const v3f &pos2, const v3f &pos3);

    MeshBuffer *createFace(
        const v3f &pos1, const v3f &pos2, const v3f &pos3, const v3f &pos4);

    MeshBuffer *createFace();

    MeshBuffer *createBox();

    MeshCreator3DState getState() const
    {
        return State;
    }

    void setCurrentVertexType(const render::VertexTypeDescriptor &newVT)
    {
        State.CurVT = newVT;
    }

    void markContainsHW(bool yes)
    {
        State.ContainsHW = yes;
    }

    void setCurrentTransform(const matrix4 &newTransform)
    {
        State.CurTransform = newTransform;
    }

    void setCurrentColor(const img::color8 &color)
    {
        State.CurColor = color;
    }

private:
    void appendVertex(MeshBuffer *buf, const v3f &pos,
        v3f normal=v3f(), v2f uv=v2f());
    void appendLine(MeshBuffer *buf, const v3f &startPos, const v3f &endPos);
    // pos1, pos2, pos3 must be ordered counter-clockwise!
    void appendTriangle(MeshBuffer *buf, const v3f &pos1, const v3f &pos2, const v3f &pos3);
    void appendFace(MeshBuffer *buf, const v3f &pos1, const v3f &pos2, const v3f &pos3, const v3f &pos4);
};
