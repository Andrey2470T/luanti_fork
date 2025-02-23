#pragma once

#include "BasicIncludes.h"
#include "client/render/meshbuffer.h"
#include "client/render/defaultVertexTypes.h"
#include "Utils/Line2D.h"
#include "Utils/Rect.h"

class MeshCreator2D
{
    render::VertexTypeDescriptor CurVT = VType2D;

public:
    MeshCreator2D() = default;

    render::VertexTypeDescriptor getCurrentVertexType() const
    {
        return CurVT;
    }

    void setCurrentVertexType(const render::VertexTypeDescriptor &vType)
    {
        CurVT = vType;
    }

    MeshBuffer *createLine(
        const v2f &startPos, const v2f &endPos, const img::color8 &color=img::color8());

    MeshBuffer *createLine(const line2f &line, const img::color8 &color=img::color8());

    // pos1, pos2, pos3 must be ordered counter-clockwise!
    MeshBuffer *createTriangle(
        const v2f &pos1, const v2f &pos2, const v2f &pos3, const img::color8 &color=img::color8());

    MeshBuffer *createRectangle(
        const rectf &rect, const std::array<img::color8, 4> &colors);

    MeshBuffer *createRectangle();
private:
    void appendVertex(MeshBuffer *buf, const v2f &pos, const img::color8 &color=img::color8(), const v2f &uv=v2f());
    void appendLine(MeshBuffer *buf, const v2f &startPos, const v2f &endPos, const img::color8 &color=img::color8());
    // pos1, pos2, pos3 must be ordered counter-clockwise!
    void appendTriangle(MeshBuffer *buf, const v2f &pos1, const v2f &pos2, const v2f &pos3, const img::color8 &color=img::color8());
    void appendRectangle(MeshBuffer *buf, const rectf &rect, const std::array<img::color8, 4> &colors);

};
