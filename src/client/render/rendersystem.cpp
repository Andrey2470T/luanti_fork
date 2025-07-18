#include "rendersystem.h"
#include "settings.h"
#include "client/ui/glyph_atlas.h"
#include "loadscreen.h"
#include "renderer.h"
#include "log.h"
#include <Utils/TypeSize.h>
#include <Image/Image.h>
#include "porting.h"
#include "client/media/resource.h"
#include <FilesystemVersions.h>
#include "client/pipeline/factory.h"

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
    pp_factory = std::make_unique<PipelineFactory>(this, g_settings->getBool("enable_dynamic_shadows"));

    basePool = std::make_unique<AtlasPool>(AtlasType::RECTPACK2D, "Basic", cache, glParams.maxTextureSize, true);
    guiPool = std::make_unique<AtlasPool>(AtlasType::RECTPACK2D, "GUI", cache, glParams.maxTextureSize, false);
    fontManager = std::unique_ptr<FontManager>(fmgr);

    g_settings->registerChangedCallback("fullscreen", settingChangedCallback, this);
    g_settings->registerChangedCallback("window_maximized", settingChangedCallback, this);
    g_settings->registerChangedCallback("menu_clouds", settingChangedCallback, this);
}

RenderSystem::~RenderSystem()
{
    g_settings->deregisterAllChangedCallbacks(this);
}

AtlasPool *RenderSystem::getPool(bool basic) const
{
    if (basic)
        return basePool.get();
    else
        return guiPool.get();
}

void RenderSystem::setWindowIcon()
{
    fs::path icon_path = porting::path_share + "/textures/base/pack/logo.png";
    img::Image *icon = cache->getOrLoad<img::Image>(ResourceType::IMAGE, icon_path.string())->data.get();

    if (!icon) {
        warningstream << "RenderSystem::setWindowIcon(): Could not load the window icon:" << icon_path << std::endl;
        return;
    }

    window->setIcon(std::shared_ptr<img::Image>(icon), g_imgmodifier);
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
    auto system = static_cast<RenderSystem*>(data);
    auto wnd = system->getWindow();
    if (name == "fullscreen") {
        wnd->setFullscreen(g_settings->getBool("fullscreen"));

    } else if (name == "window_maximized") {
        if (!wnd->isFullScreen()) {
            if (g_settings->getBool("window_maximized"))
                wnd->maximize();
            else
                wnd->restore();
        }
    }
    else if (name == "gui_scaling")
        system->setGUIScaling(g_settings->getFloat("gui_scaling", 0.5f, 20.0f));
    else if (name == "display_density_factor") {
        f32 user_factor = g_settings->getFloat("display_density_factor", 0.5f, 5.0f);
#ifndef __ANDROID__
        f32 dpi = wnd->getDisplayDensity();
        if (dpi == 0.0f)
            dpi = 96.0f;
        system->setDisplyDensity(std::max(dpi / 96.0f * user_factor, 0.5f));
#else // __ANDROID__
        system->setDisplayDensity(porting::getDisplayDensity() * user_factor);
#endif // __ANDROID__
    }
    else if (name == "menu_clouds")
        system->enableMenuClouds(g_settings->getBool("menu_clouds"));
}
