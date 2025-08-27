#include "sprite.h"
#include "batcher2d.h"
#include "client/mesh/defaultVertexTypes.h"
#include "client/render/renderer.h"
#include <Render/StreamTexture2D.h>
#include "text_sprite.h"
#include "glyph_atlas.h"

std::array<u8, 4> primVCounts = {2, 3, 4, 9};

void updateMaxArea(rectf &curMaxArea, const v2f &primULC, const v2f &primLRC, bool &init)
{
    curMaxArea.ULC.X = init ? std::min<f32>(curMaxArea.ULC.X, primULC.X) : primULC.X;
    curMaxArea.ULC.Y = init ? std::min<f32>(curMaxArea.ULC.Y, primULC.Y) : primULC.Y;
    curMaxArea.LRC.X = init ? std::max<f32>(curMaxArea.LRC.X, primLRC.X) : primLRC.X;
    curMaxArea.LRC.Y = init ? std::max<f32>(curMaxArea.LRC.Y, primLRC.Y) : primLRC.Y;

    init = true;
}

rectf UIShape::getPrimitiveArea(u32 n) const
{
    UIPrimitiveType type = getPrimitiveType(n);
    auto prim = primitives.at(n).get();

    switch(type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(prim);
        return rectf(line->start_p, line->end_p);
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(prim);

        v2f c = (trig->p1 + trig->p2 + trig->p3) / 3.0f;

        std::array<v2f, 3> p = {trig->p1 - c, trig->p2 -c, trig->p3 - c};
        std::sort(p.begin(), p.end(), [] (const v2f &p1, const v2f &p2)
        {
            return p1.getLengthSQ() < p2.getLengthSQ();
        });
        v2f areaCorner = v2f(-p.at(2).getLength(), 0.0f).rotateBy(45);
        v2f areaCorner2 = areaCorner.rotateBy(180);

        return rectf(c + areaCorner, c + areaCorner2);
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(prim);
        return rect->r;
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(prim);
        v2f areaCorner = v2f(ellipse->a, ellipse->b);
        return rectf(ellipse->center - areaCorner, ellipse->center + areaCorner);
    }
    }
}

void UIShape::updateLine(u32 n, const v2f &start_p, const v2f &end_p, const img::color8 &start_c, const img::color8 &end_c)
{
    auto line = dynamic_cast<Line *>(primitives.at(n).get());
    line->start_p = start_p;
    line->end_p = end_p;
    line->start_c = start_c;
    line->end_c = end_c;
    updateMaxArea(maxArea, start_p, end_p, maxAreaInit);
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
    updateMaxArea(maxArea, v2f(p1.X, p3.Y), p2, maxAreaInit);
}
void UIShape::updateRectangle(u32 n, const rectf &r, const std::array<img::color8, 4> &colors, const rectf &texr)
{
    auto rect = dynamic_cast<Rectangle *>(primitives.at(n).get());
    rect->r = r;
    rect->colors = colors;
    rect->texr = texr;
    updateMaxArea(maxArea, r.ULC, r.LRC, maxAreaInit);
}
void UIShape::updateEllipse(u32 n, f32 a, f32 b, const v2f &center, const img::color8 &c)
{
    auto ellipse = dynamic_cast<Ellipse *>(primitives.at(n).get());
    ellipse->a = a;
    ellipse->b = b;
    ellipse->center = center;
    ellipse->c = c;
    updateMaxArea(maxArea, center - v2f(a/2, b/2), center+v2f(a/2, b/2), maxAreaInit);
}

void UIShape::movePrimitive(u32 n, const v2f &shift)
{
    auto prim = primitives.at(n).get();

    switch(prim->type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(prim);
        line->start_p += shift;
        line->end_p += shift;
        updateMaxArea(maxArea, line->start_p, line->end_p, maxAreaInit);
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(prim);
        trig->p1 += shift;
        trig->p2 += shift;
        trig->p3 += shift;
        updateMaxArea(maxArea, v2f(trig->p1.X, trig->p3.Y), trig->p2, maxAreaInit);
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(prim);
        rect->r.ULC += shift;
        rect->r.LRC += shift;
        updateMaxArea(maxArea, rect->r.ULC, rect->r.LRC, maxAreaInit);
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(prim);
        ellipse->center += shift;
        updateMaxArea(maxArea, ellipse->center - v2f(ellipse->a/2,
            ellipse->b/2), ellipse->center+v2f(ellipse->a/2, ellipse->b/2), maxAreaInit);
    }
    }
}
void UIShape::scalePrimitive(u32 n, const v2f &scale, std::optional<v2f> center)
{
    auto prim = primitives.at(n).get();

    switch(prim->type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(prim);
        v2f c = center.has_value() ? center.value() : (line->start_p + line->end_p) / 2;

        line->start_p = c + (line->start_p - c) * scale;
        line->end_p = c + (line->end_p - c) * scale;
        updateMaxArea(maxArea, line->start_p, line->end_p, maxAreaInit);
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(prim);
        v2f c = center.has_value() ? center.value() : (trig->p1 + trig->p2 + trig->p3) / 3;

        trig->p1 = c + (trig->p1 - c) * scale;
        trig->p2 = c + (trig->p2 - c) * scale;
        trig->p3 = c + (trig->p3 - c) * scale;
        updateMaxArea(maxArea, v2f(trig->p1.X, trig->p3.Y), trig->p2, maxAreaInit);
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(prim);
        v2f c = center.has_value() ? center.value() : rect->r.getCenter();

        rect->r.ULC = c + (rect->r.ULC - c) * scale;
        rect->r.LRC = c + (rect->r.LRC - c) * scale;
        updateMaxArea(maxArea, rect->r.ULC, rect->r.LRC, maxAreaInit);
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(prim);

        ellipse->a *= scale.X;
        ellipse->b *= scale.Y;
        updateMaxArea(maxArea, ellipse->center - v2f(ellipse->a/2,
            ellipse->b/2), ellipse->center+v2f(ellipse->a/2, ellipse->b/2), maxAreaInit);
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

void UIShape::appendToBuffer(MeshBuffer *buf, v2u imgSize)
{
    for (u32 i = 0; i < primitives.size(); i++)
        appendToBuffer(buf, i, imgSize);
}

void UIShape::updateBuffer(MeshBuffer *buf, u32 primitiveNum, bool pos_or_colors, v2u imgSize)
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
        break;
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
        break;
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

        f32 invW = 1.0f / imgSize.X;
        f32 invH = 1.0f / imgSize.Y;
        rectf uv(
            rect->texr.ULC.X * invW, rect->texr.ULC.Y * invH,
            rect->texr.LRC.X * invW, rect->texr.LRC.Y * invH
        );
        svtSetUV(buf, uv.ULC, startV);
        svtSetUV(buf, v2f(uv.LRC.X, uv.ULC.Y), startV+1);
        svtSetUV(buf, uv.LRC, startV+2);
        svtSetUV(buf, v2f(uv.ULC.X, uv.LRC.Y), startV+3);

        break;
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
        break;
    }
    }
}

void UIShape::appendToBuffer(MeshBuffer *buf, u32 primitiveNum, v2u imgSize)
{
    if (primitiveNum > primitives.size()-1)
        return;

    Primitive *p = primitives.at(primitiveNum).get();

    switch (p->type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(p);
        Batcher2D::appendLine(buf, line->start_p, line->end_p, line->start_c);
        break;
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(p);
        Batcher2D::appendTriangle(buf, {trig->p1, trig->p2, trig->p3}, trig->c1);
        break;
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(p);
        if (rect->texr != rectf())
            Batcher2D::appendImageRectangle(buf, imgSize, rect->texr, rect->r, rect->colors, false);
        else
            Batcher2D::appendRectangle(buf, rect->r, rect->colors);
        break;
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(p);
        Batcher2D::appendEllipse(buf, ellipse->a, ellipse->b, imgSize, ellipse->center, ellipse->c);
        break;
    }
    }
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
    shape->addRectangle(posRect, colors, uvRect);
    rebuildMesh();
}

// Creates (without buffer filling) multiple-primitive mesh
UISprite::UISprite(render::Texture2D *tex, Renderer *_renderer, ResourceCache *_cache,
    const std::vector<UIPrimitiveType> &primitives, bool streamTexture, bool staticUsage)
    : renderer(_renderer), cache(_cache),
    mesh(std::make_unique<MeshBuffer>(UIShape::countRequiredVCount(primitives),
        UIShape::countRequiredICount(primitives), true, VType2D, staticUsage ? render::MeshUsage::STATIC : render::MeshUsage::DYNAMIC)),
    shape(std::make_unique<UIShape>()), texture(tex), streamTex(streamTexture)
{
    for (auto primType : primitives) {
        switch(primType) {
        case UIPrimitiveType::LINE:
            shape->addLine(v2f(), v2f(), img::black, img::black);
            break;
        case UIPrimitiveType::TRIANGLE:
            shape->addTriangle(v2f(), v2f(), v2f(), img::black, img::black, img::black);
            break;
        case UIPrimitiveType::RECTANGLE:
            shape->addRectangle(rectf(), {});
            break;
        case UIPrimitiveType::ELLIPSE:
            shape->addEllipse(0.0f, 0.0f, v2f(), img::black);
            break;
        }
    }
    rebuildMesh();
}

void UISprite::rebuildMesh()
{
    std::vector<UIPrimitiveType> prims(shape->getPrimitiveCount());

    for (u32 i = 0; shape->getPrimitiveCount(); i++)
        prims[i] = shape->getPrimitiveType(i);
    mesh->reallocateData(
        UIShape::countRequiredVCount(prims),
        UIShape::countRequiredICount(prims)
    );

    shape->appendToBuffer(mesh.get(), texture->getSize());
    mesh->uploadData();
}

void UISprite::updateMesh(const std::vector<u32> &dirtyPrimitives, bool pos_or_colors)
{
    for (auto n : dirtyPrimitives)
        shape->updateBuffer(mesh.get(), n, pos_or_colors, texture->getSize());
    mesh->uploadVertexData();
}

void UISprite::updateMesh(bool pos_or_colors)
{
    for (u32 i = 0; i < shape->getPrimitiveCount(); i++)
        shape->updateBuffer(mesh.get(), i, pos_or_colors, texture->getSize());
    mesh->uploadVertexData();
}

void UISprite::draw()
{
    if (shape->getPrimitiveCount() == 0 || !visible)
        return;

    renderer->setDefaultShader(true, true);
    renderer->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);

    if (texture)
        renderer->setTexture(texture);

    renderer->setClipRect(clipRect);

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
    if (!visible)
        return;

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

void UISpriteBank::addSprite(const rectf &r, u8 shift, const std::array<img::color8, 4> &colors)
{
    rectf resRect = r;
    shiftRectByLastSprite(resRect, shift);
    sprites.emplace_back(nullptr, rnd, cache, rectf(), resRect, colors, true);
    updateMaxArea(maxArea, resRect.ULC, resRect.LRC, maxAreaInit);
}

void UISpriteBank::addImageSprite(render::Texture2D *tex, const rectf &texPart, u8 shift)
{
    rectf resRect(0, 0, texPart.getWidth(), texPart.getHeight());
    shiftRectByLastSprite(resRect, shift);
    std::array<img::color8, 4> colors = {img::white, img::white, img::white, img::white};
    sprites.emplace_back(tex, rnd, cache, texPart, resRect, colors, true);
    updateMaxArea(maxArea, resRect.ULC, resRect.LRC, maxAreaInit);
}

void UISpriteBank::addTextSprite(FontManager *mgr, const EnrichedString &text, u8 shift)
{
    UITextSprite *textSprite = new UITextSprite(mgr, text, rnd, cache, {});
    textSprite->setAlignment(GUIAlignment::Center, GUIAlignment::Center);

    auto font = textSprite->getActiveFont();
    rectf resRect(0, 0, font->getTextWidth(text.getString()), font->getTextHeight(text.getString()));
    shiftRectByLastSprite(resRect, shift);
    textSprite->updateBuffer(rectf(resRect));
    sprites.emplace_back(std::unique_ptr<UISprite>(textSprite));
    updateMaxArea(maxArea, resRect.ULC, resRect.LRC, maxAreaInit);
}

void UISpriteBank::shiftRectByLastSprite(rectf &r, u8 shift)
{
    if (!auto_align)
        return;

    if (!sprites.empty()) {
        v2f rSize = r.getSize();

        if (shift == 0) {
            auto lastRow = sprites_grid.back();
            auto lastSpriteArea = sprites.at(lastRow.back()).get()->getShape()->getMaxArea();
            r.ULC.X = lastSpriteArea.LRC.X;
            r.ULC.Y = lastSpriteArea.ULC.Y;
            r.LRC = r.ULC + rSize;
            lastRow.emplace_back(sprites.size());
        }
        else if (shift == 1) {
            r.ULC.X = maxArea.getCenter().X-rSize.X/2.0f;
            r.ULC.Y = maxArea.LRC.Y;
            r.LRC = r.ULC + rSize;
            sprites_grid.emplace_back(std::vector{sprites.size()});
        }
    }
    else {
        auto size = r.getSize();
        r.ULC = center - size/2.0f;
        r.LRC = center + size/2.0f;
        sprites_grid.emplace_back(std::vector{0});
    }
}

void UISpriteBank::alignSpritesByCenter()
{
    if (!auto_align)
        return;

    v2f center_shift = -(maxArea.getCenter() - center);
    move(center_shift);
}
