#pragma once

#include "BasicIncludes.h"
#include "Utils/Rect.h"
#include "meshbuffer.h"

// Class creating meshbuffers of various 3D shapes (point, line, quad, cube and etc)
class MeshCreator3D
{
    render::VertexTypeDescriptor CurVT = render::DefaultVType;

public:
    MeshCreator3D() = default;

    MeshBuffer *createPoint(
        const v3f &pos, const img::color8 &color);

    MeshBuffer *createLine(
        const v3f &startPos, const v3f &endPos, const img::color8 &color);

    MeshBuffer *createLine(
        const line3f &line, const img::color8 &color);

    MeshBuffer *createTriangle(
        const v3f &pos1, const v3f &pos2, const v3f &pos3, const img::color8 &color8);

    MeshBuffer *createRectangle(
        const v3f &ulPos, const v3f &lrPos, const img::color8 &color8);

    MeshBuffer *createRectangle(
        const rectf &rect, const img::color8 &color8);

    MeshBuffer *createBox(
        const v3f &minPos, const v3f &maxPos, const img::color8 &color8);

    MeshBuffer *createBox(
        const aabbf &box, const img::color8 &color8);

    render::VertexTypeDescriptor getCurrentVertexType() const
    {
        return CurVT;
    }

    void setCurrentVertexType(const render::VertexTypeDescriptor &newVT)
    {
        CurVT = newVT;
    }
};
