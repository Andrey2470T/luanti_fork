#pragma once

#include "client/render/meshbuffer.h"
#include "client/render/defaultVertexTypes.h"
#include "Utils/Line2D.h"
#include "Utils/Rect.h"
#include "Render/Texture2D.h"
#include "renderer2d.h"

typedef std::array<img::color8, 4> RectColors;
class MeshCreator2D;
class ResourceCache;
struct Image2D9Slice;

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
        const v2f &pos1, const v2f &pos2, const v2f &pos3, const img::color8 &color=img::color8(),
        const v2f &uv1=v2f(0.0f, 0.0f), const v2f &uv2=v2f(0.5f, 1.0f), const v2f &uv3=v2f(1.0f, 0.0f));

    // vertices are filled in the clockwise order!
    MeshBuffer *createRectangle(
        const rectf &rect, const RectColors &colors,
        const std::array<v2f, 4> &uvs={v2f(0.0f,1.0f),v2f(1.0f,1.0f),v2f(1.0f,0.0f),v2f(0.0f,0.0f)});

    MeshBuffer *createUnitRectangle();

    MeshBuffer *createImageRectangle(
        const v2u &imgSize, const rectf &srcRect, const rectf &destRect,
        const RectColors &colors, bool flip);

    MeshBuffer *createImageUnitRectangle(bool flip);

    Image2D9Slice *createImage2D9Slice(
        ResourceCache *res, Renderer2D *rnd,
        const rectf &src_rect, const rectf &dest_rect,
        const rectf &middle_rect, render::Texture2D *base_tex,
        const RectColors &colors);
private:
    void appendVertex(MeshBuffer *buf, const v2f &pos, const img::color8 &color=img::color8(), const v2f &uv=v2f());
    void appendLine(MeshBuffer *buf, const v2f &startPos, const v2f &endPos, const img::color8 &color=img::color8());
    // pos1, pos2, pos3 must be ordered counter-clockwise!
    void appendTriangle(MeshBuffer *buf, const v2f &pos1, const v2f &pos2, const v2f &pos3, const img::color8 &color=img::color8(),
        const v2f &uv1=v2f(0.0f, 0.0f), const v2f &uv2=v2f(0.5f, 1.0f), const v2f &uv3=v2f(1.0f, 0.0f));
    void appendRectangle(MeshBuffer *buf, const rectf &rect, const RectColors &colors,
        const std::array<v2f, 4> &uvs={v2f(0.0f,1.0f),v2f(1.0f,1.0f),v2f(1.0f,0.0f),v2f(0.0f,0.0f)});

};
