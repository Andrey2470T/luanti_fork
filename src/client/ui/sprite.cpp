#include "sprite.h"
#include "batcher2d.h"
#include "settings.h"
#include "client/render/defaultVertexTypes.h"

void UIShape::updatePrimitive(MeshBuffer *buf, u32 primitiveNum, bool pos_or_colors)
{
    if (primitiveNum > primitives.size()-1)
        return;

    Primitive *p = primitives.at(primitiveNum).get();

    u32 startV = 0;

    for (u32 i = 0; i < primitiveNum; i++)
        startV += primitives[i]->primVCounts[(u8)primitives[i]->type];

    switch (p->type) {
    case UIPrimitiveType::LINE: {
        auto line = dynamic_cast<Line *>(p);
        if (pos_or_colors) {
            svtSetPos2D(buf, line->start_p, startV);
            svtSetPos2D(buf, line->end_p, startV+1);
        }
        else {
            svtSetColor(buf, line->start_c, startV);
            svtSetColor(buf, line->end_c, startV);
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

        for (u8 i = 0; i < Primitive::primVCounts[(u8)UIPrimitiveType::ELLIPSE]-1; i++) {
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

UISprite::UISprite(render::Texture2D *_tex, Renderer2D *_renderer,
    ResourceCache *cache, const rectf &srcRect, const rectf &destRect,
    const std::array<img::color8, 4> &colors, bool applyFilter)
    : renderer(_renderer), area(destRect)
{
    rect = std::make_unique<MeshBuffer>(true);

    addRect(srcRect, destRect, colors);

    texture = std::make_unique<ImageFiltered>(cache, _tex);
    if (applyFilter && g_settings->getBool("gui_scaling_filter")) {
        v2f srcSize = srcRect.getSize();
        v2f destSize = destRect.getSize();
        texture->filter(_tex->getName(),
            v2i(srcSize.X, srcSize.Y), v2i(destSize.X, destSize.Y));
    }
}

v2u UISprite::getSize() const
{
    return texture->output_tex->getSize();
}

void UISprite::updateRect(u32 n, const rectf &r)
{
    updateRect(n, r.ULC, r.LRC);
}

void UISprite::updateRect(u32 n, const v2f &ulc, const v2f &lrc)
{
    u32 vertexShift = n * 4;
    rect->setAttrAt<v2f>(ulc, 0, vertexShift);
    rect->setAttrAt<v2f>(v2f(lrc.X, ulc.Y), 0, vertexShift+1);
    rect->setAttrAt<v2f>(lrc, 0, vertexShift+2);
    rect->setAttrAt<v2f>(v2f(ulc.X, lrc.Y), 0, vertexShift+3);

    auto &subRect = subRects.at(n);
    subRect.ULC = ulc;
    subRect.LRC = lrc;

    for (auto &r : subRects) {
        area.ULC.X = std::min<f32>(area.ULC.X, r.ULC.X);
        area.ULC.Y = std::min<f32>(area.ULC.Y, r.ULC.Y);
        area.LRC.X = std::max<f32>(area.LRC.X, r.LRC.X);
        area.LRC.Y = std::max<f32>(area.LRC.Y, r.LRC.Y);
    }
	
	dirty = true;
}

void UISprite::setClipRect(const recti &r)
{
	renderer->setClipRect(r);
}

void UISprite::move(u32 n, const v2f &shift)
{
    updateRect(n, area.ULC + shift, area.LRC + shift);
}

void UISprite::scale(u32 n, const v2f &scale)
{
    auto subRect = subRects.at(n);
    v2f size = subRect.getSize();
	size *= scale;
	v2f center = area.getCenter();
    updateRect(n, center-size/2, center+size/2);
}

void UISprite::setColors(u32 n, const std::array<img::color8, 4> &colors)
{
    u32 vertexShift = n * 4;
    rect->setAttrAt<img::color8>(colors.at(0), 1, vertexShift);
    rect->setAttrAt<img::color8>(colors.at(1), 1, vertexShift+1);
    rect->setAttrAt<img::color8>(colors.at(2), 1, vertexShift+2);
    rect->setAttrAt<img::color8>(colors.at(3), 1, vertexShift+3);
	
	dirty = true;
}
	
void UISprite::flush()
{
	if (!dirty)
	    return;
    rect->uploadVertexData();
	dirty = false;
}

void UISprite::draw()
{
	renderer->drawImageFiltered(rect.get(), texture.get());
}


UIAnimatedSprite::UIAnimatedSprite(v2u texSize, u32 _frameLength, Renderer2D *_renderer,
    ResourceCache *cache, const std::array<img::color8, 4> &colors, bool applyFilter)
    : UISprite(new render::Texture2D("AnimatedSprite", texSize.X, texSize.Y, img::PF_RGBA8), _renderer,
        cache, rectf(v2f(texSize.X, texSize.Y)), rectf(v2f(texSize.X, texSize.Y)), colors, applyFilter), frameLength(_frameLength)
{}

void UIAnimatedSprite::addFrame(img::Image *img, const rectf &r)
{
    auto texSize = getSize();

    if (texSize.getLengthSQ() < img->getSize().getLengthSQ())
        return;

    if (!area.isRectInside(r))
        return;

    auto imgIt = std::find(framesImages.begin(), framesImages.end(), img);

    u32 imgIndex;
    if (imgIt != framesImages.end())
        imgIndex = std::distance(framesImages.begin(), imgIt);
    else {
        framesImages.push_back(img);
        imgIndex = framesImages.size()-1;
    }

    auto rectIt = std::find(framesRects.begin(), framesRects.end(), r);

    u32 rectIndex;
    if (rectIt != framesRects.end())
        rectIndex = std::distance(framesRects.begin(), rectIt);
    else {
        framesRects.push_back(r);
        rectIndex = framesRects.size()-1;
    }

    frames.emplace_back(imgIndex, rectIndex);
}

std::vector<UISpriteFrame *> UIAnimatedSprite::getFrames() const
{
    std::vector<UISpriteFrame *> resFrames(frames.size());

    for (u32 i = 0; i < frames.size(); i++)
        resFrames[i] = frames[i].get();

    return resFrames;
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

    UISpriteFrame *f = frames.at(newFrameNum).get();
    auto tex = texture->output_tex;
    auto size = tex->getSize();

    if (newFrameNum != curFrameNum) {
        updateRect(0, framesRects.at(f->rectIndex));

        tex->uploadData(framesImages.at(f->imgIndex));
        texture->filter(tex->getName(), v2i(size.X, size.Y), v2i(size.X, size.Y));

        flush();
        curFrameNum = newFrameNum;
    }

    renderer->drawImageFiltered(rect.get(), texture.get());
}

std::vector<UIAnimatedSprite *> UISpriteBank::getSprites() const
{
    std::vector<UIAnimatedSprite *> spritesRes(sprites.size());

    for (u32 i = 0; i < sprites.size(); i++)
        spritesRes.at(i) = sprites.at(i).get();

    return spritesRes;
}

void UISpriteBank::drawSprite(u32 index, u32 curTime, bool loop)
{
    if (index > sprites.size()-1)
        return;

    sprites.at(index)->drawFrame(curTime, loop);
}
