#include "sprite.h"
#include "meshcreator2d.h"
#include "settings.h"
#include "extra_images.h"

UISprite::UISprite(render::Texture2D *_tex, MeshCreator2D *_creator, Renderer2D *_renderer,
    ResourceCache *cache, const rectf &srcRect, const rectf &destRect, bool applyFilter)
    : renderer(_renderer), area(destRect)
{
	rect = std::unique_ptr<MeshBuffer>(_creator->createImageRectangle(
        _tex->getSize(), srcRect, destRect, {}, false));

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

void UISprite::flush()
{
	rect->uploadData(MeshBufferType::VERTEX);
}
void UISprite::draw() const
{
	renderer->drawImageFiltered(rect.get(), texture.get());
}
	
	
	
