#include "rendersystem.h"
#include "settings.h"
#include "client/ui/glyph_atlas.h"
#include "loadscreen.h"
#include "renderer.h"
#include "log.h"
#include <Utils/TypeSize.h>
#include <Image/Image.h>

const img::color8 RenderSystem::menu_sky_color = img::color8(img::PF_RGBA8, 140, 186, 250, 255);

RenderSystem::RenderSystem(Client *_client, ResourceCache *_cache, MyEventReceiver *_receiver)
    : client(_client), cache(_cache), receiver(_receiver)
{
    initWindow();
    FontManager *fmgr = new FontManager(cache);

    v2u viewport = window->getViewportSize();
    auto glParams = window->getGLParams();
    renderer = std::make_unique<Renderer>(cache, recti(0, 0, viewport.X, viewport.Y), glParams.maxTextureUnits);
    load_screen = std::make_unique<LoadScreen>(cache, renderer.get(), fmgr);
}

void RenderSystem::initWindow()
{
    // Resolution selection
    bool fullscreen = g_settings->getBool("fullscreen");
#ifdef __ANDROID__
    u16 screen_w = 0, screen_h = 0;
    bool window_maximized = false;
#else
    u16 screen_w = std::max<u16>(g_settings->getU16("screen_w"), 1);
    u16 screen_h = std::max<u16>(g_settings->getU16("screen_h"), 1);
    // If I…
    // 1. … set fullscreen = true and window_maximized = true on startup
    // 2. … set fullscreen = false later
    // on Linux with SDL, everything breaks.
    // => Don't do it.
    bool window_maximized = !fullscreen && g_settings->getBool("window_maximized");
#endif

    // bpp, fsaa, vsync
    bool vsync = g_settings->getBool("vsync");
    // Don't enable MSAA in OpenGL context creation if post-processing is enabled,
    // the post-processing pipeline handles it.
    bool enable_fsaa = g_settings->get("antialiasing") == "fsaa" &&
                       !g_settings->getBool("enable_post_processing");
    u16 fsaa = enable_fsaa ? MYMAX(2, g_settings->getU16("fsaa")) : 0;

    auto configured_name = g_settings->get("video_driver");
    if (configured_name.empty())
        configured_name = "opengl3";

    main::MainWindowParameters params;
    params.Width = screen_w;
    params.Height = screen_h;

    if (configured_name == "opengl3")
        params.GLType = main::OGL_TYPE_DESKTOP;
    else if (configured_name == "gles2")
        params.GLType = main::OGL_TYPE_ES;
    else
        params.GLType = main::OGL_TYPE_WEB;

    params.FullScreen = fullscreen;
    params.AntiAlias = fsaa;
    params.Maximized = window_maximized;
    params.Resizable = 1;
    params.VSync = vsync;
    params.DriverDebug = g_settings->getBool("opengl_debug");

    verbosestream << "Using the " << configured_name << " video driver" << std::endl;

    window = std::make_unique<main::MainWindow>(params);
}

void RenderSystem::settingChangedCallback(const std::string &name, void *data)
{
    if (name == "fullscreen") {
        window->setFullscreen(g_settings->getBool("fullscreen"));

    } else if (name == "window_maximized") {
        if (!window->isFullScreen()) {
            if (g_settings->getBool("window_maximized"))
                window->maximize();
            else
                window->restore();
        }
    }
    else if (name == "gui_scaling")
        gui_scaling = g_settings->getFloat("gui_scaling", 0.5f, 20.0f);
    else if (name == "display_density_factor") {
        f32 user_factor = g_settings->getFloat("display_density_factor", 0.5f, 5.0f);
#ifndef __ANDROID__
        f32 dpi = window->getDisplayDensity();
        if (dpi == 0.0f)
            dpi = 96.0f;
        display_density = std::max(dpi / 96.0f * user_factor, 0.5f);
#else // __ANDROID__
        display_density = porting::getDisplayDensity() * user_factor;
#endif // __ANDROID__
    }

}
