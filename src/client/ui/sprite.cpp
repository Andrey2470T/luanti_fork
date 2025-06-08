#include "sprite.h"
#include "batcher2d.h"
#include "settings.h"
#include "extra_images.h"

UISprite::UISprite(render::Texture2D *_tex, Renderer2D *_renderer,
    ResourceCache *cache, const rectf &srcRect, const rectf &destRect,
    const std::array<img::color8, 4> &colors, bool applyFilter)
    : renderer(_renderer), area(destRect)
{
    rect = std::make_unique<MeshBuffer>(true);

    Batcher2D::appendImageRectangle(rect.get(),
        _tex->getSize(), srcRect, destRect, colors, false);

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

void UISprite::updateRect(const rectf &r)
{
	updateRect(r.ULC, r.LRC);
}

void UISprite::updateRect(const v2f &ulc, const v2f &lrc)
{
	rect->setAttrAt<v2f>(ulc, 0, 0);
	rect->setAttrAt<v2f>(v2f(lrc.X, ulc.Y), 0, 1);
	rect->setAttrAt<v2f>(lrc, 0, 2);
	rect->setAttrAt<v2f>(v2f(ulc.X, lrc.Y), 0, 3);
	
	area.ULC = ulc;
	area.LRC = lrc;
	
	dirty = true;
}

void UISprite::setClipRect(const recti &r)
{
	renderer->setClipRect(r);
}

void UISprite::move(const v2f &shift)
{
	updateRect(area.ULC + shift, area.LRC + shift);
}

void UISprite::scale(const v2f &scale)
{
	v2f size = area.getSize();
	size *= scale;
	v2f center = area.getCenter();
	updateRect(center-size/2, center+size/2);
}

void UISprite::setColors(const std::array<img::color8, 4> &colors)
{
	rect->setAttrAt<img::color8>(colors.at(0), 1, 0);
	rect->setAttrAt<img::color8>(colors.at(1), 1, 1);
	rect->setAttrAt<img::color8>(colors.at(2), 1, 2);
	rect->setAttrAt<img::color8>(colors.at(3), 1, 3);
	
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
        updateRect(framesRects.at(f->rectIndex));

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
