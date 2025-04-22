#include "loadscreen.h"
#include "client/ui/fontengine.h"
#include "Utils/Rect.h"
#include "Render/Texture2D.h"
#include "irrlicht_gui/GUIEnums.h"
#include "client/media/resource.h"
#include "settings.h"
#include "client/render/clouds.h"

LoadScreen::LoadScreen(ResourceCache *_cache, IGUIEnvironment *_guienv)
    : cache(_cache)
{
    guitext = std::make_unique<GUIStaticText>(_guienv, L"", recti(), false, false);
    guitext->setTextAlignment(GUIAlignment::Center, GUIAlignment::UpperLeft);

    ResourceInfo *progress_info = cache->getOrLoad(ResourceType::TEXTURE, "progress_bar.png");
    ResourceInfo *progress_bg_info = cache->getOrLoad(ResourceType::TEXTURE, "progress_bar_bg.png");

    progress_img = dynamic_cast<TextureResourceInfo*>(progress_info)->data.get();
    progress_bg_img = dynamic_cast<TextureResourceInfo*>(progress_bg_info)->data.get();

    g_settings->registerChangedCallback("menu_clouds", settingChangedCallback, this);
	g_settings->registerChangedCallback("display_density", settingChangedCallback, this);
}

void LoadScreen::updateText(v2u screensize, const std::wstring &text, f32 dtime,
    s32 percent, f32 display_density, f32 *shutdown_progress)
{
    v2i center((s32)screensize.X/2, (s32)screensize.Y/2);
    v2i textsize(g_fontengine->getTextWidth(text), g_fontengine->getTextHeight());

    recti textarea(center-textsize/2, center+textsize/2);

    guitext->setRelativePosition(textarea);
    guitext->setText(text);

    if (menu_clouds)
        g_menuclouds->step(dtime*3);

    int percent_min = 0;
    int percent_max = percent;
    if (shutdown_progress) {
        *shutdown_progress = fmodf(*shutdown_progress + (dtime * 50.0f), 140.0f);
        percent_max = std::min((int) *shutdown_progress, 100);
        percent_min = std::max((int) *shutdown_progress - 40, 0);
    }
    // draw progress bar
    if ((percent_min >= 0) && (percent_max <= 100)) {
#ifndef __ANDROID__
        const v2u &img_size = progress_bg_img->getSize();
        f32 density = gui_scaling * display_density;
        u32 imgW = std::clamp(img_size.X, 200u, 600u) * density;
        u32 imgH = std::clamp(img_size.Y, 24u, 72u) * density;
#else
        const v2u img_size(256, 48);
        f32 imgRatio = (f32)img_size.Y / img_size.X;
        u32 imgW = screensize.X / 2.2f;
        u32 imgH = floor(imgW * imgRatio);
#endif
        v2i img_pos((screensize.X - imgW) / 2, (screensize.Y - imgH) / 2);

    }
}

void LoadScreen::draw() const
{
    //driver->setFog(RenderingEngine::MENU_SKY_COLOR);
    //driver->beginScene(true, true, RenderingEngine::MENU_SKY_COLOR);

    if (menu_clouds)
        g_menucloudsmgr->drawAll();

    /*draw2DImageFilterScaled(get_video_driver(), progress_img_bg,
                                    core::rect<s32>(img_pos.X, img_pos.Y,
                                                    img_pos.X + imgW,
                                                    img_pos.Y + imgH),
                                    core::rect<s32>(0, 0, img_size.Width,
                                                    img_size.Height),
                                    0, 0, true);

            draw2DImageFilterScaled(get_video_driver(), progress_img,
                                    core::rect<s32>(img_pos.X + (percent_min * imgW) / 100, img_pos.Y,
                                                    img_pos.X + (percent_max * imgW) / 100,
                                                    img_pos.Y + imgH),
                                    core::rect<s32>(percent_min * img_size.Width / 100, 0,
                                                    percent_max * img_size.Width / 100,
                                                    img_size.Height),
                                    0, 0, true);*/
}

void LoadScreen::settingChangedCallback(const std::string &name, void *data)
{
    if (name == "menu_clouds")
        menu_clouds = g_settings->getBool(name);
    else if (name == "gui_scaling")
        gui_scaling = g_settings->getFloat(name, 0.5f, 20.0f);
}
