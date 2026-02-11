#include "sprite.h"
#include "batcher2d.h"
#include "client/mesh/defaultVertexTypes.h"
#include "client/render/renderer.h"
#include <Render/Texture2D.h>
#include "client/render/rendersystem.h"
#include "gui/IGUIEnvironment.h"
#include "text_sprite.h"
#include "glyph_atlas.h"
#include "extra_images.h"

std::array<u8, 4> primVCounts = {2, 3, 4, 9};
std::array<u8, 4> primICounts = {2, 3, 6, 9};

void updateMaxArea(rectf &curMaxArea, const v2f &primULC, const v2f &primLRC, bool &init)
{
    curMaxArea.ULC.X = init ? std::min<f32>(curMaxArea.ULC.X, primULC.X) : primULC.X;
    curMaxArea.ULC.Y = init ? std::min<f32>(curMaxArea.ULC.Y, primULC.Y) : primULC.Y;
    curMaxArea.LRC.X = init ? std::max<f32>(curMaxArea.LRC.X, primLRC.X) : primLRC.X;
    curMaxArea.LRC.Y = init ? std::max<f32>(curMaxArea.LRC.Y, primLRC.Y) : primLRC.Y;

    init = true;
}

const std::array<img::color8, 4> RectColors::defaultColors = {img::white, img::white, img::white, img::white};

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
    default:
        return rectf();
    }
}

void UIShape::addLine(
        const v2f &start_p, const v2f &end_p,
        const img::color8 &start_c, const img::color8 &end_c) {
    primitives.emplace_back((Primitive *)(new Line(start_p, end_p, start_c, end_c)));
    changeFlag = ChangeFlags::COUNT_CHANGED;
    updateMaxArea(maxArea, start_p, end_p, maxAreaInit);
}
void UIShape::addTriangle(
        const v2f &p1, const v2f &p2, const v2f &p3,
        const img::color8 &c1, const img::color8 &c2, const img::color8 &c3) {
    primitives.emplace_back((Primitive *)(new Triangle(p1, p2, p3, c1, c2, c3)));
    changeFlag = ChangeFlags::COUNT_CHANGED;
    updateMaxArea(maxArea, v2f(p1.X, p3.Y), p2, maxAreaInit);
}
void UIShape::addRectangle(
    const rectf &r, const RectColors &colors,
    const rectf &texr, const v2u &imgSize) {
    f32 invW = 1.0f / imgSize.X;
    f32 invH = 1.0f / imgSize.Y;

    rectf tcoords;
    tcoords.ULC = v2f(texr.ULC.X * invW, texr.ULC.Y * invH);
    tcoords.LRC = v2f(texr.LRC.X * invW, texr.LRC.Y * invH);

    primitives.emplace_back((Primitive *)(new Rectangle(r, colors, tcoords)));
    changeFlag = ChangeFlags::COUNT_CHANGED;
    updateMaxArea(maxArea, r.ULC, r.LRC, maxAreaInit);
}
void UIShape::addEllipse(f32 a, f32 b, const v2f &center, const img::color8 &c) {
    primitives.emplace_back((Primitive *)(new Ellipse(a, b, center, c)));
    changeFlag = ChangeFlags::COUNT_CHANGED;
    updateMaxArea(maxArea, center - v2f(a/2, b/2), center+v2f(a/2, b/2), maxAreaInit);
}

void UIShape::updateLine(
    u32 n, const v2f &start_p, const v2f &end_p,
    const img::color8 &start_c, const img::color8 &end_c)
{
    auto line = dynamic_cast<Line *>(primitives.at(n).get());
    line->start_p = start_p;
    line->end_p = end_p;
    line->start_c = start_c;
    line->end_c = end_c;

    changeFlag = ChangeFlags::CHANGED;
    dirtyPrimitives.emplace(n);
    updateMaxArea(maxArea, start_p, end_p, maxAreaInit);
}
void UIShape::updateTriangle(
    u32 n, const v2f &p1, const v2f &p2, const v2f &p3,
    const img::color8 &c1, const img::color8 &c2, const img::color8 &c3)
{
    auto trig = dynamic_cast<Triangle *>(primitives.at(n).get());
    trig->p1 = p1;
    trig->p2 = p2;
    trig->p3 = p3;
    trig->c1 = c1;
    trig->c2 = c2;
    trig->c3 = c3;

    changeFlag = ChangeFlags::CHANGED;
    dirtyPrimitives.emplace(n);
    updateMaxArea(maxArea, v2f(p1.X, p3.Y), p2, maxAreaInit);
}
void UIShape::updateRectangle(
    u32 n, const rectf &r, const RectColors &colors,
    const rectf &texr, const v2u &imgSize)
{
    auto rect = dynamic_cast<Rectangle *>(primitives.at(n).get());

    f32 invW = 1.0f / imgSize.X;
    f32 invH = 1.0f / imgSize.Y;

    rectf tcoords;
    tcoords.ULC = v2f(texr.ULC.X * invW, texr.ULC.Y * invH);
    tcoords.LRC = v2f(texr.LRC.X * invW, texr.LRC.Y * invH);

    rect->r = r;
    rect->colors = colors;
    rect->texr = tcoords;

    changeFlag = ChangeFlags::CHANGED;
    dirtyPrimitives.emplace(n);
    updateMaxArea(maxArea, r.ULC, r.LRC, maxAreaInit);
}
void UIShape::updateEllipse(u32 n, f32 a, f32 b, const v2f &center, const img::color8 &c)
{
    auto ellipse = dynamic_cast<Ellipse *>(primitives.at(n).get());
    ellipse->a = a;
    ellipse->b = b;
    ellipse->center = center;
    ellipse->c = c;

    changeFlag = ChangeFlags::CHANGED;
    dirtyPrimitives.emplace(n);
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
        break;
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(prim);
        trig->p1 += shift;
        trig->p2 += shift;
        trig->p3 += shift;
        updateMaxArea(maxArea, v2f(trig->p1.X, trig->p3.Y), trig->p2, maxAreaInit);
        break;
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(prim);
        rect->r.ULC += shift;
        rect->r.LRC += shift;
        updateMaxArea(maxArea, rect->r.ULC, rect->r.LRC, maxAreaInit);
        break;
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(prim);
        ellipse->center += shift;
        updateMaxArea(maxArea, ellipse->center - v2f(ellipse->a/2,
            ellipse->b/2), ellipse->center+v2f(ellipse->a/2, ellipse->b/2), maxAreaInit);
        break;
    }
    default:
        return;
    }

    changeFlag = ChangeFlags::CHANGED;
    dirtyPrimitives.emplace(n);
}
void UIShape::scalePrimitive(u32 n, const v2f &scale, std::optional<v2f> center)
{
    auto prim = primitives.at(n).get();

    switch(prim->type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(prim);
        v2f c = center ? center.value() : (line->start_p + line->end_p) / 2;

        line->start_p = c + (line->start_p - c) * scale;
        line->end_p = c + (line->end_p - c) * scale;
        updateMaxArea(maxArea, line->start_p, line->end_p, maxAreaInit);
        break;
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(prim);
        v2f c = center ? center.value() : (trig->p1 + trig->p2 + trig->p3) / 3;

        trig->p1 = c + (trig->p1 - c) * scale;
        trig->p2 = c + (trig->p2 - c) * scale;
        trig->p3 = c + (trig->p3 - c) * scale;
        updateMaxArea(maxArea, v2f(trig->p1.X, trig->p3.Y), trig->p2, maxAreaInit);
        break;
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(prim);
        v2f c = center ? center.value() : rect->r.getCenter();

        rect->r.ULC = c + (rect->r.ULC - c) * scale;
        rect->r.LRC = c + (rect->r.LRC - c) * scale;
        updateMaxArea(maxArea, rect->r.ULC, rect->r.LRC, maxAreaInit);
        break;
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(prim);

        ellipse->a *= scale.X;
        ellipse->b *= scale.Y;
        updateMaxArea(maxArea, ellipse->center - v2f(ellipse->a/2,
            ellipse->b/2), ellipse->center+v2f(ellipse->a/2, ellipse->b/2), maxAreaInit);
        break;
    }
    default:
        return;
    }

    changeFlag = ChangeFlags::CHANGED;
    dirtyPrimitives.emplace(n);
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

void UIShape::removePrimitive(u32 i)
{
    if (i >= getPrimitiveCount())
        return;

    primitives.erase(primitives.begin()+i);
    changeFlag = ChangeFlags::COUNT_CHANGED;

    for (u32 i = 0; i < primitives.size(); i++) {
        rectf primArea = getPrimitiveArea(i);
        updateMaxArea(maxArea, primArea.ULC, primArea.LRC, maxAreaInit);
    }
}
void UIShape::clear()
{
    primitives.clear();
    dirtyPrimitives.clear();
    maxArea = rectf();
    changeFlag = ChangeFlags::COUNT_CHANGED;
    maxAreaInit = false;
}

void UIShape::move(const v2f &shift)
{
    for (u32 i = 0; i < primitives.size(); i++)
        movePrimitive(i, shift);
}

void UIShape::scale(const v2f &scale, std::optional<v2f> c)
{
    v2f center = c.has_value() ? c.value() : maxArea.getCenter();

    for (u32 i = 0; i < primitives.size(); i++)
        scalePrimitive(i, scale, center);
}

void UIShape::updateBuffer(MeshBuffer *buf, bool forceFull, u32 vertexOffset, u32 indexOffset)
{
    if (forceFull || changeFlag == ChangeFlags::COUNT_CHANGED ||
            changeFlag == ChangeFlags::COUNT_CHANGED_NOREBUILD) {
        for (u32 k = 0; k < primitives.size(); k++)
            updatePrimInBuffer(buf, k, vertexOffset, indexOffset);
    }
    else if (changeFlag == ChangeFlags::CHANGED) {
        for (u32 k : dirtyPrimitives)
            updatePrimInBuffer(buf, k, vertexOffset, indexOffset);

        dirtyPrimitives.clear();
    }

    changeFlag = ChangeFlags::NONE;
}

void UIShape::updatePrimInBuffer(MeshBuffer *buf, u32 primitiveNum, u32 vertexOffset, u32 indexOffset)
{
    Primitive *p = primitives.at(primitiveNum).get();

    u32 startV = 0;
    u32 startI = 0;

    for (u32 i = 0; i < primitiveNum; i++) {
        startV += primVCounts[(u8)primitives[i]->type];
        startI += primICounts[(u8)primitives[i]->type];
    }

    buf->setVertexOffset(vertexOffset+startV);
    buf->setIndexOffset(indexOffset+startI);

    switch (p->type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(p);
        Batcher2D::line(buf, line->start_p, line->end_p, line->start_c);
        break;
    }
    case UIPrimitiveType::TRIANGLE: {
        auto trig = dynamic_cast<Triangle *>(p);
        Batcher2D::triangle(buf, {trig->p1, trig->p2, trig->p3}, trig->c1);
        break;
    }
    case UIPrimitiveType::RECTANGLE: {
        auto rect = dynamic_cast<Rectangle *>(p);
        Batcher2D::rectangle(buf, rect->r, rect->colors.colors, rect->texr);
        break;
    }
    case UIPrimitiveType::ELLIPSE: {
        auto ellipse = dynamic_cast<Ellipse *>(p);
        Batcher2D::ellipse(buf, ellipse->a, ellipse->b, v2u(1,1), ellipse->center, ellipse->c);
        break;
    }
    default:
        break;
    }
}

SpriteDrawBatch::SpriteDrawBatch(RenderSystem *_rndsys, ResourceCache *_cache)
    : rndsys(_rndsys), cache(_cache), buffer(std::make_unique<MeshBuffer>(true, VType2D))
{}

// if auto-align is enabled, the rects areas must be absolute, otherwise - relative to the ulc
UIRects *SpriteDrawBatch::addRectsSprite(const std::vector<TexturedRect> &rects,
    u32 depthLevel,
    const recti *clipRect)
{
    UIRects *rectsSprite = new UIRects(cache, this, rndsys->getPool(false), rects, depthLevel);
    sprites.emplace_back(rectsSprite);

    updateBatch = true;

    auto rectsMaxArea = rectsSprite->getArea();
    updateMaxArea(maxArea, rectsMaxArea.ULC, rectsMaxArea.LRC, maxAreaInit);
    
    if (clipRect)
        rectsSprite->setClipRect(*clipRect);

    return rectsSprite;
}

UIRects *SpriteDrawBatch::addRectsSprite(
    u32 rectsCount,
    u32 depthLevel,
    const recti *clipRect)
{
	UIRects *rectsSprite = new UIRects(cache, this, rndsys->getPool(false), rectsCount, depthLevel);
    sprites.emplace_back(rectsSprite);

    updateBatch = true;

    auto rectsMaxArea = rectsSprite->getArea();
    updateMaxArea(maxArea, rectsMaxArea.ULC, rectsMaxArea.LRC, maxAreaInit);
    
    if (clipRect)
        rectsSprite->setClipRect(*clipRect);

    return rectsSprite;
}

Image2D9Slice *SpriteDrawBatch::addImage2D9Slice(
    const rectf &src_rect, const rectf &dest_rect,
    const rectf &middle_rect, img::Image *baseImg,
    u32 depthLevel, const RectColors &colors,
    std::optional<AtlasTileAnim> anim)
{
    Image2D9Slice *img2D9Slice = new Image2D9Slice(cache, this, rndsys->getPool(false),
        src_rect, dest_rect, middle_rect, baseImg, colors, anim, depthLevel);
    sprites.emplace_back(img2D9Slice);

    updateBatch = true;

    auto rectsMaxArea = img2D9Slice->getArea();
    updateMaxArea(maxArea, rectsMaxArea.ULC, rectsMaxArea.LRC, maxAreaInit);

    return img2D9Slice;
}

UITextSprite *SpriteDrawBatch::addTextSprite(const std::wstring &text,
    u32 depthLevel,
    std::optional<std::variant<rectf, v2f>> shift,
    const img::color8 &textColor,
    const recti *clipRect,
    bool wordWrap,
    GUIAlignment horizAlign,
    GUIAlignment vertAlign)
{
    UITextSprite *textSprite = new UITextSprite(
        rndsys->getFontManager(), rndsys->getGUIEnvironment()->getSkin(),
        cache, this, text, false, wordWrap, false, depthLevel);
    sprites.emplace_back(textSprite);

    updateBatch = true;

    auto &textObj = textSprite->getTextObj();
    textObj.setAlignment(horizAlign, vertAlign);
    textObj.setOverrideColor(textColor);

    auto font = textObj.getActiveFont();
    rectf resRect(toV2T<f32>(font->getTextSize(text)));

    if (shift) {
        if (std::holds_alternative<rectf>(shift.value()))
            resRect = std::get<rectf>(shift.value());
        else
            resRect += std::get<v2f>(shift.value());
    }
    textSprite->setBoundRect(resRect);

    updateMaxArea(maxArea, resRect.ULC, resRect.LRC, maxAreaInit);
    
    if (clipRect)
        textSprite->setClipRect(*clipRect);

    return textSprite;
}

void SpriteDrawBatch::move(const v2f &shift)
{
    for (auto &sprite : sprites) {
        sprite->getShape().move(shift);
        auto primArea = sprite->getShape().getMaxArea();
        updateMaxArea(maxArea, primArea.ULC, primArea.LRC, maxAreaInit);\

        updateBatch = true;
    }
}

void SpriteDrawBatch::scale(const v2f &scale)
{
    v2f center = maxArea.getCenter();

    for (auto &sprite : sprites) {
        sprite->getShape().scale(scale, center);
        auto primArea = sprite->getShape().getMaxArea();
        updateMaxArea(maxArea, primArea.ULC, primArea.LRC, maxAreaInit);

        updateBatch = true;
    }
}

void SpriteDrawBatch::remove(u32 n)
{
    if (n > sprites.size()-1)
        return;

    auto sprite_it = sprites.begin()+n;
    chunks.erase(chunks.find(sprite_it->get()));
    sprites.erase(sprite_it);

    updateBatch = true;
}

void SpriteDrawBatch::rebuild()
{
    bool wasPrimCountChanged = false;
    std::vector<UIPrimitiveType> prims;
    for (auto &sprite : sprites) {
        sprite->appendToBatch();

        if (sprite->getShape().changeFlag == UIShape::ChangeFlags::COUNT_CHANGED)
            wasPrimCountChanged = true;

        auto &shape = sprite->getShape();

        for (u32 i = 0; i < shape.getPrimitiveCount(); i++)
            prims.push_back(shape.getPrimitiveType(i));
    }

    buffer->reallocateData(
        UIShape::countRequiredVCount(prims),
        UIShape::countRequiredICount(prims)
    );

    u32 rectOffset = 0;
    for (auto &sprite : sprites) {
        if (sprite->getShape().changeFlag != UIShape::ChangeFlags::NONE || wasPrimCountChanged) {
            sprite->getShape().updateBuffer(buffer.get(), wasPrimCountChanged, rectOffset*4, rectOffset*6);
            updateBatch = true;
        }
        rectOffset += sprite->getShape().getPrimitiveCount();
    }

    buffer->uploadData();

    if (updateBatch) {
        batch();
        updateBatch = false;
    }
}

void SpriteDrawBatch::draw()
{
    auto rnd = rndsys->getRenderer();
    rnd->setRenderState(false);
    rnd->setDefaultShader(true, true);
    rnd->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);

    for (auto &chunksLevel : levels) {
        for (auto &chunk : chunksLevel.second.chunks) {
            rnd->setTexture(chunk.texture);
            rnd->setClipRect(chunk.clipRect);

			for (auto &group : chunk.groups) {
	            u32 indexOffset = group.first*6;
            	u32 indexCount = group.second*6;

                rnd->draw(buffer.get(), indexOffset, indexCount);
			}
        }
    }
}

void SpriteDrawBatch::batch()
{
    levels.clear();

    u32 rectsOffset = 0;

    for (auto &sprite : sprites) {
        auto &spriteToChunks = chunks[sprite.get()];
        u32 depthLevel = sprite->getDepthLevel();

        if (!sprite->isVisible()) {
            for (auto &chunk : spriteToChunks)
                rectsOffset += chunk.rectsCount;
            continue;
        }

		// The batching occurs within the certain depth level
        auto &chunksLevel = levels[depthLevel];

		// Merging the same chunks (by texture and clip rectsCount) into single batched chunk
        // It is necessary to split the merged chunks into groups since they can locate
        // in the vector unnecessarily in the "one after another" order
        for (auto &chunk : spriteToChunks) {
            auto foundBatchedChunkIt = std::find(chunksLevel.chunks.begin(), chunksLevel.chunks.end(), chunk);

            if (foundBatchedChunkIt != chunksLevel.chunks.end()) {
                u32 curBatchedChunkId = std::distance(chunksLevel.chunks.begin(), foundBatchedChunkIt);

                if (chunksLevel.lastBatchedChunkId != curBatchedChunkId) {
                    chunksLevel.lastBatchedChunkId = curBatchedChunkId;

                    auto &lastBatchedChunk = chunksLevel.chunks.at(chunksLevel.lastBatchedChunkId);
                    lastBatchedChunk.groups.emplace_back(rectsOffset, chunk.rectsCount);
            	}
                else {
                    auto &lastBatchedChunk = chunksLevel.chunks.at(chunksLevel.lastBatchedChunkId);
                    lastBatchedChunk.groups.back().second += chunk.rectsCount;
                }
        	}
            else {
                chunksLevel.chunks.emplace_back(chunk.texture, chunk.clipRect);    	
                chunksLevel.lastBatchedChunkId = chunksLevel.chunks.size()-1;

                auto &lastBatchedChunk = chunksLevel.chunks.back();
                lastBatchedChunk.groups.emplace_back(rectsOffset, chunk.rectsCount);
            }

            rectsOffset += chunk.rectsCount;
        }
    }
}
