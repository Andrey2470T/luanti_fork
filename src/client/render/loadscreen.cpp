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
    : cache(_cache), rndsys(_system)
{
    guitext = std::make_unique<UITextSprite>(_mgr, _system->getGUIEnvironment()->getSkin(),
        EnrichedString(L""), rndsys->getRenderer(), cache, false, false);
    guitext->setAlignment(GUIAlignment::Center, GUIAlignment::UpperLeft);

    auto progress_bg_img = cache->get<img::Image>(ResourceType::IMAGE, "progress_bar_bg.png");
    auto progress_img = cache->get<img::Image>(ResourceType::IMAGE, "progress_bar.png");

    auto progress_bg_img_size = rectf(v2f(), toV2T<f32>(progress_bg_img->getSize()));
    auto progress_img_size = rectf(v2f(), toV2T<f32>(progress_img->getSize()));

    auto guiPool = _system->getPool(false);
    render::Texture2D *tex = guiPool->getAtlasByTile(progress_img)->getTexture();
    progress_rect = std::make_unique<UISprite>(tex, rndsys->getRenderer(), cache, true);

    progress_bg_img_rect = guiPool->getTileRect(progress_bg_img);
    progress_img_rect = guiPool->getTileRect(progress_img);

    auto shape = progress_rect->getShape();
    shape->addRectangle(progress_bg_img_size, UISprite::defaultColors, progress_bg_img_rect);
    shape->addRectangle(progress_img_size, UISprite::defaultColors, progress_img_rect);

    progress_rect->rebuildMesh();
}

void LoadScreen::draw(v2u screensize, const std::wstring &text, f32 dtime, bool menu_clouds,
    s32 percent, f32 scale_f, f32 *shutdown_progress)
{
    rndsys->beginDraw(render::CBF_COLOR | render::CBF_DEPTH, Renderer::menu_sky_color);

    if (draw_clouds)
        g_menumgr->drawClouds(dtime);

    FogType fogtype;
    img::colorf fogcolor;
    f32 fogstart, fogend, density;

    auto rnd = rndsys->getRenderer();
    rnd->getFogParams(fogtype, fogcolor, fogstart, fogend, density);
    rnd->setFogParams(fogtype, img::color8ToColorf(Renderer::menu_sky_color), fogstart, fogend, density);

    if (percent != last_percent) {
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
            auto pb_size = progress_bg_img_rect.getSize() * scale_f;
            auto p_size = progress_img_rect.getSize() * scale_f;

            v2f pb_pos(center.X - pb_size.X / 2, center.Y - pb_size.Y / 2);\
            v2f p_pos(center.X - p_size.X / 2, center.Y - p_size.Y / 2);

            rectf new_progress_bg_size(pb_pos, pb_pos + v2f(pb_size.X, pb_size.Y));
            rectf new_progress_size(p_pos, p_pos + v2f(p_size.X, p_size.Y));

            progress_rect->getShape()->updateRectangle(0, new_progress_bg_size, UISprite::defaultColors, progress_bg_img_rect);
            progress_rect->getShape()->updateRectangle(1, new_progress_size, UISprite::defaultColors, progress_img_rect);
            progress_rect->updateMesh(true, false);

            progress_cliprect = recti(0, p_size.Y, percent_max * p_size.X / 100, 0);
            progress_cliprect += toV2T<s32>(p_pos);
        }
    }

    progress_rect->draw(0, 1);

    progress_rect->setClipRect(progress_cliprect);
    progress_rect->draw(1, 1);
    progress_rect->setClipRect(recti());

    guitext->draw();

    rndsys->endDraw();
}
