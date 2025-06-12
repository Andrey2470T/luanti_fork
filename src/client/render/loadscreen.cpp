#include "loadscreen.h"
//#include "client/ui/fontengine.h"
#include "Utils/Rect.h"
#include "Render/Texture2D.h"
#include "client/media/resource.h"
//#include "client/render/clouds.h"
#include "client/ui/extra_images.h"
#include "client/ui/sprite.h"
#include "client/ui/renderer2d.h"

LoadScreen::LoadScreen(ResourceCache *_cache, Renderer2D *_renderer, MeshCreator2D *_creator, IGUIEnvironment *_guienv)
    : cache(_cache), renderer(_renderer)
{
    //guitext = std::make_unique<GUIStaticText>(_guienv, L"", recti(), false, false);
   // guitext->setTextAlignment(GUIAlignment::Center, GUIAlignment::UpperLeft);

    auto progress_img = cache->getOrLoad<render::Texture2D>(ResourceType::TEXTURE, "progress_bar.png")->data.get();
    auto progress_bg_img = cache->getOrLoad<render::Texture2D>(ResourceType::TEXTURE, "progress_bar_bg.png")->data.get();

    auto progress_img_size = progress_img->getSize();
    auto progress_bg_img_size = progress_bg_img->getSize();

    rectf progress_img_size_f(v2f(progress_img_size.X, progress_img_size.Y));
    rectf progress_bg_img_size_f(v2f(progress_bg_img_size.X, progress_bg_img_size.Y));

    progress_rect = std::make_unique<UISprite>(
        progress_img, _renderer, _cache, progress_img_size_f, progress_img_size_f, std::array<img::color8, 4>(), true);
    progress_bg_rect = std::make_unique<UISprite>(
        progress_bg_img, _renderer, _cache, progress_bg_img_size_f, progress_bg_img_size_f, std::array<img::color8, 4>(), true);
}

void LoadScreen::updateText(v2u screensize, const std::wstring &text, f32 dtime, bool menu_clouds,
    s32 percent, f32 scale_f, f32 *shutdown_progress, MeshCreator2D *creator)
{
    if (percent == last_percent)
        return;

    last_percent = percent;
    draw_clouds = menu_clouds;

    //v2i center((s32)screensize.X/2, (s32)screensize.Y/2);
    //v2i textsize(g_fontengine->getTextWidth(text), g_fontengine->getTextHeight());

    //recti textarea(center-textsize/2, center+textsize/2);

    //guitext->setRelativePosition(textarea);
    //guitext->setText(text);

    if (draw_clouds)
        g_menuclouds->step(dtime*3);

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
        auto img_size = progress_bg_rect->getSize();
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

        progress_bg_rect->getShape()->updateRectangle(0, new_progress_bg_size, {});
        progress_bg_rect->flush();
        progress_rect->getShape()->updateRectangle(0, new_progress_size, {});
        progress_rect->flush();

        renderer->setClipRect(
            recti(percent_min * img_size.X / 100, 0, percent_max * img_size.X / 100, img_size.Y));
    }
}

void LoadScreen::draw() const
{
    //driver->setFog(RenderingEngine::MENU_SKY_COLOR);
    //driver->beginScene(true, true, RenderingEngine::MENU_SKY_COLOR);

    if (draw_clouds)
        g_menucloudsmgr->drawAll();

    progress_bg_rect->draw();
    progress_rect->draw();
}
