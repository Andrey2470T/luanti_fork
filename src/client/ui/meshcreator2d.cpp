#include "meshcreator2d.h"

MeshBuffer *MeshCreator2D::createLine(
    const v2f &startPos, const v2f &endPos, const img::color8 &color)
{
    MeshBuffer *buf = new MeshBuffer(MeshBufferType::VERTEX_INDEX, CurVT);

    appendLine(buf, startPos, endPos, color);

    return buf;
}

MeshBuffer *MeshCreator2D::createLine(const line2f &line, const img::color8 &color)
{
    return createLine(line.Start, line.End, color);
}

MeshBuffer *MeshCreator2D::createTriangle(
    const v2f &pos1, const v2f &pos2, const v2f &pos3, const img::color8 &color)
{
    MeshBuffer *buf = new MeshBuffer(MeshBufferType::VERTEX_INDEX, CurVT);

    appendTriangle(buf, pos1, pos2, pos3, color);

    return buf;
}

MeshBuffer *MeshCreator2D::createRectangle(
    const rectf &rect, const std::array<img::color8, 4> &colors)
{
    MeshBuffer *buf = new MeshBuffer(MeshBufferType::VERTEX_INDEX, CurVT);

    appendRectangle(buf, rect, colors);

    return buf;
}

MeshBuffer *MeshCreator2D::createRectangle()
{
    return createRectangle(
        rectf(v2f(-0.5f, 0.5f), v2f(0.5f, -0.5f)), {});
}

void MeshCreator2D::appendVertex(MeshBuffer *buf, const v2f &pos, const img::color8 &color, const v2f &uv)
{
    appendVT2D(buf, pos, color, uv);
}

void MeshCreator2D::appendLine(MeshBuffer *buf, const v2f &startPos, const v2f &endPos, const img::color8 &color)
{
    appendVertex(buf, startPos, color);
    appendVertex(buf, endPos, color);
}

void MeshCreator2D::appendTriangle(MeshBuffer *buf, const v2f &pos1, const v2f &pos2, const v2f &pos3, const img::color8 &color)
{
    appendVertex(buf, pos1, color, v2f(0.0f, 0.0f));
    appendVertex(buf, pos2, color, v2f(1.0f, 0.0f));
    appendVertex(buf, pos3, color, v2f(0.5f, 1.0f));
}

void MeshCreator2D::appendRectangle(MeshBuffer *buf, const rectf &rect, const std::array<img::color8, 4> &colors)
{
    std::array<u32, 6> indices = {0, 1, 2, 2, 1, 3};

    appendVertex(buf, rect.ULC, colors[0], v2f(0.0f, 0.0f));
    appendVertex(buf, v2f(rect.ULC.X, rect.LRC.Y), colors[1], v2f(1.0f, 0.0f));
    appendVertex(buf, rect.LRC, colors[2], v2f(0.0f, 1.0f));
    appendVertex(buf, v2f(rect.LRC.X, rect.ULC.Y), colors[3], v2f(1.0f, 1.0f));

    for (u32 i = 0; i < 6; i++)
        buf->setIndexAt(indices[i]);
}
