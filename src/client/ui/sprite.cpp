#include "sprite.h"
#include "batcher2d.h"
#include "client/render/defaultVertexTypes.h"
#include "client/render/renderer.h"
#include <Render/StreamTexture2D.h>

std::array<u8, 4> primVCounts = {2, 3, 4, 9};

void UIShape::updateLine(u32 n, const v2f &start_p, const v2f &end_p, const img::color8 &start_c, const img::color8 &end_c)
{
    auto line = dynamic_cast<Line *>(primitives.at(n).get());
    line->start_p = start_p;
    line->end_p = end_p;
    line->start_c = start_c;
    line->end_c = end_c;
    updateMaxArea(start_p, end_p);
}
void UIShape::updateTriangle(u32 n, const v2f &p1, const v2f &p2, const v2f &p3, const img::color8 &c1, const img::color8 &c2, const img::color8 &c3)
{
    auto trig = dynamic_cast<Triangle *>(primitives.at(n).get());
    trig->p1 = p1;
    trig->p2 = p2;
    trig->p3 = p3;
    trig->c1 = c1;
    trig->c2 = c2;
    trig->c3 = c3;
    updateMaxArea(v2f(p1.X, p3.Y), p2);
}
void UIShape::updateRectangle(u32 n, const rectf &r, const std::array<img::color8, 4> &colors)
{
    auto rect = dynamic_cast<Rectangle *>(primitives.at(n).get());
    rect->r = r;
    rect->colors = colors;
    updateMaxArea(r.ULC, r.LRC);
}
void UIShape::updateEllipse(u32 n, f32 a, f32 b, const v2f &center, const img::color8 &c)
{
    auto ellipse = dynamic_cast<Ellipse *>(primitives.at(n).get());
    ellipse->a = a;
    ellipse->b = b;
    ellipse->center = center;
    ellipse->c = c;
    updateMaxArea(center - v2f(a/2, b/2), center+v2f(a/2, b/2));
}

void UIShape::movePrimitive(u32 n, const v2f &shift)
{
    auto prim = primitives.at(n).get();

    switch(prim->type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(prim);
        line->start_p += shift;
        line->end_p += shift;
        updateMaxArea(line->start_p, line->end_p);
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(prim);
        trig->p1 += shift;
        trig->p2 += shift;
        trig->p3 += shift;
        updateMaxArea(v2f(trig->p1.X, trig->p3.Y), trig->p2);
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(prim);
        rect->r.ULC += shift;
        rect->r.LRC += shift;
        updateMaxArea(rect->r.ULC, rect->r.LRC);
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(prim);
        ellipse->center += shift;
        updateMaxArea(ellipse->center - v2f(ellipse->a/2,
            ellipse->b/2), ellipse->center+v2f(ellipse->a/2, ellipse->b/2));
    }
    }
}
void UIShape::scalePrimitive(u32 n, const v2f &scale)
{
    auto prim = primitives.at(n).get();

    switch(prim->type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(prim);
        v2f c = (line->start_p + line->end_p) / 2;

        line->start_p = c + (line->start_p - c) * scale;
        line->end_p = c + (line->end_p - c) * scale;
        updateMaxArea(line->start_p, line->end_p);
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(prim);
        v2f c = (trig->p1 + trig->p2 + trig->p3) / 3;

        trig->p1 = c + (trig->p1 - c) * scale;
        trig->p2 = c + (trig->p2 - c) * scale;
        trig->p3 = c + (trig->p3 - c) * scale;
        updateMaxArea(v2f(trig->p1.X, trig->p3.Y), trig->p2);
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(prim);
        v2f c = rect->r.getCenter();

        rect->r.ULC = c + (rect->r.ULC - c) * scale;
        rect->r.LRC = c + (rect->r.LRC - c) * scale;
        updateMaxArea(rect->r.ULC, rect->r.LRC);
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(prim);
        ellipse->a *= scale.X;
        ellipse->b *= scale.Y;
        updateMaxArea(ellipse->center - v2f(ellipse->a/2,
            ellipse->b/2), ellipse->center+v2f(ellipse->a/2, ellipse->b/2));
    }
    }
}

u32 UIShape::countRequiredVCount(const std::vector<UIPrimitiveType> &primitives)
{
    u32 count = 0;

    for (u32 i = 0; i < primitives.size(); i++)
        count += primVCounts[(u8)primitives[i]];

    return count;
}

u32 UIShape::countRequiredICount(const std::vector<UIPrimitiveType> &primitives)
{
    u32 count = 0;

    for (u32 i = 0; i < primitives.size(); i++) {
        if (primitives[i] == UIPrimitiveType::RECTANGLE)
            count += 6;
    }

    return count;
}

void UIShape::updateBuffer(MeshBuffer *buf, u32 primitiveNum, bool pos_or_colors)
{
    if (primitiveNum > primitives.size()-1)
        return;

    Primitive *p = primitives.at(primitiveNum).get();

    u32 startV = 0;

    for (u32 i = 0; i < primitiveNum; i++)
        startV += primVCounts[(u8)primitives[i]->type];

    switch (p->type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(p);
        if (pos_or_colors) {
            svtSetPos2D(buf, line->start_p, startV);
            svtSetPos2D(buf, line->end_p, startV+1);
        }
        else {
            svtSetColor(buf, line->start_c, startV);
            svtSetColor(buf, line->end_c, startV+1);
        }
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(p);
        if (pos_or_colors) {
            svtSetPos2D(buf, trig->p1, startV);
            svtSetPos2D(buf, trig->p2, startV+1);
            svtSetPos2D(buf, trig->p3, startV+2);
        }
        else {
            svtSetColor(buf, trig->c1, startV);
            svtSetColor(buf, trig->c2, startV+1);
            svtSetColor(buf, trig->c3, startV+2);
        }
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(p);
        if (pos_or_colors) {
            svtSetPos2D(buf, rect->r.ULC, startV);
            svtSetPos2D(buf, v2f(rect->r.LRC.X, rect->r.ULC.Y), startV+1);
            svtSetPos2D(buf, rect->r.LRC, startV+2);
            svtSetPos2D(buf, v2f(rect->r.ULC.X, rect->r.LRC.Y), startV+3);
        }
        else {
            svtSetColor(buf, rect->colors[0], startV);
            svtSetColor(buf, rect->colors[1], startV+1);
            svtSetColor(buf, rect->colors[2], startV+2);
            svtSetColor(buf, rect->colors[3], startV+3);
        }
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(p);

        if (pos_or_colors)
            svtSetPos2D(buf, ellipse->center, startV);
        else
            svtSetColor(buf, ellipse->c, startV);

        for (u8 i = 0; i < primVCounts[(u8)UIPrimitiveType::ELLIPSE]-1; i++) {
            u32 curAngle = i * PI / 4;
            v2f relPos(ellipse->a * cos(curAngle), ellipse->b * sin(curAngle));
            if (pos_or_colors)
                svtSetPos2D(buf, ellipse->center + relPos, startV+i+1);
            else
                svtSetColor(buf, ellipse->c, startV+i+1);
        }
    }
    }
}

void UIShape::updateMaxArea(const v2f &ulc, const v2f &lrc)
{
    maxArea.ULC.X = maxAreaInit ? std::min<f32>(maxArea.ULC.X, ulc.X) : ulc.X;
    maxArea.ULC.Y = maxAreaInit ? std::min<f32>(maxArea.ULC.Y, ulc.Y) : ulc.Y;
    maxArea.LRC.X = maxAreaInit ? std::max<f32>(maxArea.LRC.X, lrc.X) : lrc.X;
    maxArea.LRC.Y = maxAreaInit ? std::max<f32>(maxArea.LRC.Y, lrc.Y) : lrc.Y;

    maxAreaInit = true;
}

UISprite::UISprite(render::Texture2D *tex, Renderer *_renderer, ResourceCache *_cache,
    bool streamTexture, bool staticUsage)
    : renderer(_renderer), cache(_cache), mesh(std::make_unique<MeshBuffer>(true, VType2D,
        staticUsage ? render::MeshUsage::STATIC : render::MeshUsage::DYNAMIC)),
      shape(std::make_unique<UIShape>()), texture(tex), streamTex(streamTexture)
{}

// Creates a single rectangle mesh
UISprite::UISprite(render::Texture2D *tex, Renderer *_renderer,
    ResourceCache *_cache, const rectf &uvRect, const rectf &posRect,
    const std::array<img::color8, 4> &colors, bool streamTexture, bool staticUsage)
    : renderer(_renderer), cache(_cache),
    mesh(std::make_unique<MeshBuffer>(UIShape::countRequiredVCount({UIPrimitiveType::RECTANGLE}),
        UIShape::countRequiredICount({UIPrimitiveType::RECTANGLE}), true, VType2D, staticUsage ? render::MeshUsage::STATIC : render::MeshUsage::DYNAMIC)),
    shape(std::make_unique<UIShape>()), texture(tex), streamTex(streamTexture)
{
    shape->addRectangle(posRect, colors);
    Batcher2D::appendImageRectangle(mesh.get(), tex->getSize(), uvRect, posRect, colors, false);
}

// Creates (without buffer filling) multiple-primitive mesh
UISprite::UISprite(render::Texture2D *tex, Renderer *_renderer, ResourceCache *_cache,
    const std::vector<UIPrimitiveType> &primitives, bool streamTexture, bool staticUsage)
    : renderer(_renderer), cache(_cache),
    mesh(std::make_unique<MeshBuffer>(UIShape::countRequiredVCount(primitives),
        UIShape::countRequiredICount(primitives), true, VType2D, staticUsage ? render::MeshUsage::STATIC : render::MeshUsage::DYNAMIC)),
    shape(std::make_unique<UIShape>()), texture(tex), streamTex(streamTexture)
{}

void UISprite::reallocateBuffer()
{
    std::vector<UIPrimitiveType> prims(shape->getPrimitiveCount());

    for (u32 i = 0; shape->getPrimitiveCount(); i++)
        prims[i] = shape->getPrimitiveType(i);
    mesh->reallocateData(
        UIShape::countRequiredVCount(prims),
        UIShape::countRequiredICount(prims)
    );
}

void UISprite::setClipRect(const recti &r)
{
    renderer->setClipRect(r);
}

void UISprite::draw()
{
    if (shape->getPrimitiveCount() == 0 || !visible)
        return;

    renderer->setDefaultShader(true, true);
    renderer->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);

    if (texture)
        renderer->setTexture(texture);

    auto prevType = shape->getPrimitiveType(0);
    u32 pOffset = 0;
    u32 pCount = 1;
    for (u32 i = 0; i < shape->getPrimitiveCount(); i++) {
        auto curType = shape->getPrimitiveType(i);

        if (curType == prevType)
            ++pCount;
        else {
            drawPart(pOffset, pCount);

            pCount = 0;
            pOffset = i;
            prevType = curType;
        }
    }
}

void UISprite::drawPart(u32 pOffset, u32 pCount)
{
    auto vao = mesh->getVAO();

    u32 startVCount = 0;

    for (u32 i = 0; i < pOffset; i++)
        startVCount += primVCounts[(u8)shape->getPrimitiveType(i)];

    auto pType = shape->getPrimitiveType(pOffset);
    u8 vcount = primVCounts[(u8)pType] * pCount;
    switch(pType) {
    case UIPrimitiveType::LINE:
        vao->draw(render::PT_LINES, vcount, startVCount);
        break;
    case UIPrimitiveType::TRIANGLE:
        vao->draw(render::PT_TRIANGLES, vcount, startVCount);
        break;
    case UIPrimitiveType::RECTANGLE:
        vao->draw(render::PT_TRIANGLE_FAN, vcount, startVCount);
        break;
    case UIPrimitiveType::ELLIPSE:
        vao->draw(render::PT_TRIANGLE_FAN, vcount, startVCount);
        break;
    }
}

UIAnimatedSprite::UIAnimatedSprite(render::Texture2D *texture, u32 _frameLength, Renderer *_renderer,
    ResourceCache *cache, const std::array<img::color8, 4> &colors)
    : UISprite(texture, _renderer, cache, rectf(v2f(texture->getSize().X, texture->getSize().Y)),
        rectf(v2f(texture->getSize().X, texture->getSize().Y)), colors, true), frameLength(_frameLength)
{
    visible = true;
}

u32 UIAnimatedSprite::addImage(img::Image *img)
{
    auto imgIt = std::find(framesImages.begin(), framesImages.end(), img);

    if (imgIt == framesImages.end()) {
        framesImages.push_back(img);
        return framesImages.size()-1;
    }

    return std::distance(framesImages.begin(), imgIt);
}

void UIAnimatedSprite::drawFrame(u32 time, bool loop)
{
    if (frameLength == 0)
        return;

    u32 n = time / frameLength;
    u32 newFrameNum;
    if (loop)
        newFrameNum = n % frames.size();
    else
        newFrameNum = (n >= frames.size()) ? frames.size() - 1 : n;

    auto frameImg = framesImages.at(frames.at(newFrameNum));

    if (newFrameNum != curFrameNum) {
        texture->uploadData(frameImg);
        curFrameNum = newFrameNum;
        texture->bind();
        dynamic_cast<render::StreamTexture2D *>(texture)->flush();
    }

    draw();
}
