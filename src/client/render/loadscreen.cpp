#include "loadscreen.h"
#include "Utils/Rect.h"
#include "Render/Texture2D.h"
#include "client/media/resource.h"
#include "client/render/clouds.h"
#include "client/ui/extra_images.h"
#include "client/ui/sprite.h"
#include "gui/mainmenumanager.h"
#include "rendersystem.h"
#include "client/ui/text_sprite.h"
#include "client/render/atlas.h"
#include "client/ui/batcher2d.h"
#include "client/render/renderer.h"

LoadScreen::LoadScreen(ResourceCache *_cache, RenderSystem *_system, FontManager *_mgr)
    : cache(_cache), renderer(_system->getRenderer())
{
    guitext = std::make_unique<UITextSprite>(_mgr, _system->getGUIEnvironment()->getSkin(), EnrichedString(L""), renderer, cache);
    guitext->setAlignment(GUIAlignment::Center, GUIAlignment::UpperLeft);

    progress_img = cache->get<img::Image>(ResourceType::IMAGE, "progress_bar.png");
    progress_bg_img = cache->get<img::Image>(ResourceType::IMAGE, "progress_bar_bg.png");

    auto progress_img_size = progress_img->getSize();
    auto progress_bg_img_size = progress_bg_img->getSize();

    rectf progress_img_size_f(v2f(progress_img_size.X, progress_img_size.Y));
    rectf progress_bg_img_size_f(v2f(progress_bg_img_size.X, progress_bg_img_size.Y));

    auto basicPool = _system->getPool(true);
    render::Texture2D *tex = basicPool->getAtlasByTile(progress_img)->getTexture();
    progress_rect = std::make_unique<UISprite>(
        tex, renderer, cache, std::vector{UIPrimitiveType::RECTANGLE, UIPrimitiveType::RECTANGLE}, true);

    auto shape = progress_rect->getShape();
    shape->addRectangle(progress_img_size_f, {});
    shape->addRectangle(progress_bg_img_size_f, {});

    Batcher2D::appendImageRectangle(progress_rect->getBuffer(),
        tex->getSize(), basicPool->getTileRect(progress_img), progress_img_size_f, {}, false);
    Batcher2D::appendImageRectangle(progress_rect->getBuffer(),
        tex->getSize(), basicPool->getTileRect(progress_bg_img), progress_bg_img_size_f, {}, false);
}

void LoadScreen::draw(v2u screensize, const std::wstring &text, f32 dtime, bool menu_clouds,
    s32 percent, f32 scale_f, f32 *shutdown_progress)
{
    if (percent == last_percent)
        return;

    last_percent = percent;
    draw_clouds = menu_clouds;

    v2f center(screensize.X/2, screensize.Y/2);

    auto font = guitext->getActiveFont();
    v2f textsize(font->getTextWidth(text), font->getTextHeight(text));

    guitext->setText(text);
    guitext->updateBuffer(rectf(center-textsize/2, center+textsize/2));

    s32 percent_min = 0;
    s32 percent_max = percent;
    if (shutdown_progress) {
        *shutdown_progress = fmodf(*shutdown_progress + (dtime * 50.0f), 140.0f);
        percent_max = std::min((s32)*shutdown_progress, 100);
        percent_min = std::max((s32)*shutdown_progress - 40, 0);
    }
    // draw progress bar
    if ((percent_min >= 0) && (percent_max <= 100)) {
#ifndef __ANDROID__
        auto img_size = progress_bg_img->getSize();
        u32 imgW = std::clamp(img_size.X, 200u, 600u) * scale_f;
        u32 imgH = std::clamp(img_size.Y, 24u, 72u) * scale_f;
#else
        const v2u img_size(256, 48);
        f32 imgRatio = (f32)img_size.Y / img_size.X;
        u32 imgW = screensize.X / 2.2f;
        u32 imgH = floor(imgW * imgRatio);
#endif
        v2f img_pos((screensize.X - imgW) / 2, (screensize.Y - imgH) / 2);

        rectf new_progress_bg_size(img_pos, img_pos + v2f(imgW, imgH));
        rectf new_progress_size(
            v2f(img_pos.X + (percent_min * imgW) / 100, img_pos.Y),
            v2f(img_pos.X + (percent_max * imgW) / 100, img_pos.Y + imgH));

        progress_rect->getShape()->updateRectangle(0, new_progress_size, {});
        progress_rect->getShape()->updateRectangle(1, new_progress_bg_size, {});
        progress_rect->updateMesh(true);

        progress_cliprect = recti(percent_min * img_size.X / 100, 0, percent_max * img_size.X / 100, img_size.Y);
    }

    FogType fogtype;
    img::color8 fogcolor;
    f32 fogstart, fogend, density;
    renderer->getFogParams(fogtype, fogcolor, fogstart, fogend, density);
    renderer->setFogParams(fogtype, Renderer::menu_sky_color, fogstart, fogend, density);

    renderer->getContext()->clearBuffers(render::CBF_COLOR, Renderer::menu_sky_color);

    if (draw_clouds)
        g_menumgr->drawClouds(dtime);

    progress_rect->drawPart(0, 1);

    progress_rect->setClipRect(progress_cliprect);
    progress_rect->drawPart(1, 1);
    progress_rect->setClipRect(recti());

    guitext->draw();
}
