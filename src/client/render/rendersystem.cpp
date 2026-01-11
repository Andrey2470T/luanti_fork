#include "rendersystem.h"
#include "Core/TimeCounter.h"
#include "client/event/inputhandler.h"
#include "gui/touchcontrols.h"
#include "settings.h"
#include "client/ui/glyph_atlas.h"
#include "loadscreen.h"
#include "renderer.h"
#include "log.h"
#include <Image/Image.h>
#include "porting.h"
#include "client/media/resource.h"
#include <FilesystemVersions.h>
#include "client/core/client.h"
#include "client/pipeline/core.h"
#include "client/ui/gameui.h"
#include "client/render/drawlist.h"
#include "client/render/particles.h"
#include "client/render/sky.h"
#include "client/render/clouds.h"
#include "client/ao/transformNode.h"
#include "client/ao/animation.h"
#include "client/ui/hud.h"
#include "client/ui/minimap.h"
#include "gui/guiEnvironment.h"
#include "client/ui/gameformspec.h"
#include "client/ui/profilergraph.h"
#include "util/tracy_wrapper.h"
#include "client/ao/nametag.h"
#include "client/render/datatexture.h"
#include "client/pipeline/pipeline.h"

RenderSystem::RenderSystem(ResourceCache *_cache)
    : cache(_cache)
{
    initWindow();

    fontManager = std::make_unique<FontManager>(this, cache);

    v2u viewport = window->getViewportSize();
    auto glParams = window->getGLParams();
    renderer = std::make_unique<Renderer>(cache, recti(0, 0, viewport.X, viewport.Y), glParams->maxTextureUnits);

    guienv = std::make_unique<gui::CGUIEnvironment>(this, window->getWindowSize(), cache);

    guiPool = std::make_unique<AtlasPool>(AtlasType::RECTPACK2D, "GUI", cache,
        window->getGLParams()->maxTextureSize, false, false);

    gui_scaling = g_settings->getFloat("gui_scaling", 0.5f, 20.0f);
    updateDisplayDensity();

    g_settings->registerChangedCallback("fullscreen", settingChangedCallback, this);
    g_settings->registerChangedCallback("window_maximized", settingChangedCallback, this);
    g_settings->registerChangedCallback("menu_clouds", settingChangedCallback, this);
}

RenderSystem::~RenderSystem()
{
    g_settings->deregisterAllChangedCallbacks(this);
}

void RenderSystem::initLoadScreen()
{
    load_screen = std::make_unique<LoadScreen>(cache, this, fontManager.get());
}

void RenderSystem::initRenderEnvironment(Client *_client)
{
    client = _client;

    basePool = std::make_unique<AtlasPool>(AtlasType::RECTPACK2D, "Basic", cache,
        window->getGLParams()->maxTextureSize, true, true);

    pp_core = std::make_unique<PipelineCore>(client, g_settings->getBool("enable_dynamic_shadows"));

    drawlist = std::make_unique<DistanceSortedDrawList>(client);
    particle_manager = std::make_unique<ParticleManager>(this, cache, &client->getEnv());

    node_mgr = std::make_unique<TransformNodeManager>();
    anim_mgr = std::make_unique<AnimationManager>(node_mgr.get());

    gameui = std::make_unique<GameUI>(client);
    gameformspec = std::make_unique<GameFormSpec>();

    fullyInit = true;
}

void RenderSystem::initSkybox()
{
    assert(client != nullptr);

    sky = std::make_unique<Sky>(this, cache);
    clouds = std::make_unique<Clouds>(this, cache, myrand());
}

void RenderSystem::deinitRenderEnvironment()
{
    client = nullptr;
}

Hud *RenderSystem::getHud() const
{
    return gameui->getHud();
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
    fs::path icon_name = "logo.png";
    img::Image *icon = cache->get<img::Image>(ResourceType::IMAGE, icon_name);

    if (!icon) {
        warningstream << "RenderSystem::setWindowIcon(): Could not load the window icon:" << icon_name << std::endl;
        return;
    }

    window->setIcon(icon);
}

Minimap *RenderSystem::getDefaultMinimap() const
{
    return gameui->getHud()->getMinimap();
}

void RenderSystem::activateAtlas(img::Image *img, bool basic_pool)
{
    auto pool = getPool(basic_pool);
    render::Texture2D *texture = nullptr;

    if (img)
        texture = pool->getAtlasByTile(img)->getTexture();
    renderer->setTexture(texture);
}

void RenderSystem::buildGUIAtlas()
{
    auto texpaths = getTexturesDefaultPaths();

    for (auto &path : texpaths) {
        for (auto &entry : fs::directory_iterator(path))
            guiPool->addTile(cache->getOrLoad<img::Image>(
                ResourceType::IMAGE, entry.path().filename()));
    }

    guiPool->buildRectpack2DAtlas();
}

void RenderSystem::beginDraw(u16 flags, img::color8 color, f32 depth, u8 stencil)
{
    renderer->getContext()->clearBuffers(flags, color, depth, stencil);
}

void RenderSystem::endDraw()
{
    window->SwapWindow();
}

bool RenderSystem::run()
{
    auto open = window->pollEventsFromQueue();

    if (open) {
        while (true) {
            auto event = window->popEvent();

            if (!event)
                break;

            bool processed = false;
            if (client)
                processed = client->getInputHandler()->getReceiver()->OnEvent(*event);

            if (!processed)
                guienv->postEventFromUser(*event);

            if (event->Type == ET_STRING_INPUT_EVENT && event->StringInput.Str) {
                delete event->StringInput.Str;
            }
        }

        if (window->shouldUpdateViewport) {
            v2u wndSize = window->getWindowSize();
            recti newViewport(0, 0, wndSize.X, wndSize.Y);
            renderer->getContext()->setViewportSize(newViewport);
            window->shouldUpdateViewport = false;
        }
        if (auto focusElem = guienv->getFocus()) {
            window->resetReceiveTextInputEvents(
                focusElem->getAbsolutePosition(), focusElem->acceptsIME());
        }
    }

    window->clearEventQueue();

    return open;
}

void RenderSystem::render()
{
    ZoneScoped;

    auto fog_color = sky->getFogColor();
    auto sky_color = sky->getSkyColor();

    /*
        Fog
    */
    auto draw_control = drawlist->getDrawControl();

    if (renderer->fogEnabled()) {
        renderer->setFogParams(
            FogType::Linear,
            img::color8ToColorf(fog_color),
            draw_control.fog_range * sky->getFogStart(),
            draw_control.fog_range * 1.0f,
            0.0f // unused
        );
    } else {
        renderer->setFogParams(
            FogType::Linear,
            img::color8ToColorf(fog_color),
            FOG_RANGE_ALL,
            FOG_RANGE_ALL + 100 * BS,
            0.0f // unused
        );
    }

    /*
        Drawing
    */
    TimeTaker tt_draw("Draw scene", nullptr, PRECISION_MICRO);

    beginDraw(render::CBF_COLOR | render::CBF_DEPTH, sky_color);

    pp_core->run(sky_color, gameui->getFlags() & GUIF_SHOW_HUD);

    /*
        Profiler graph
    */
    gameui->getProfilerGraphs()->draw();

    endDraw();

    auto &drawstats = renderer->getDrawStats();
    drawstats.drawtime = tt_draw.stop(true);
    g_profiler->graphAdd("Draw scene [us]", drawstats.drawtime);

    drawstats.drawcalls = 0;
    drawstats.drawn_primitives = 0;
}

void RenderSystem::autosaveScreensizeAndCo(v2u initial_screen_size, bool initial_wnd_maximized)
{
    if (!g_settings->getBool("autosave_screensize"))
        return;

    // Note: If the screensize or similar hasn't changed (i.e. it's the same as
    // the setting was when minetest started, as given by the initial_* parameters),
    // we do not want to save the thing. This allows users to also manually change
    // the settings.

    // Don't save the fullscreen size, we want the windowed size.
    bool fullscreen = window->isFullScreen();
    // Screen size
    const v2u current_wnd_size = window->getWindowSize();
    // Don't replace good value with (0, 0)
    if (!fullscreen &&
        current_wnd_size != v2u(0, 0) &&
        current_wnd_size != initial_screen_size) {
        g_settings->setU16("screen_w", current_wnd_size.X);
        g_settings->setU16("screen_h", current_wnd_size.Y);
    }

    // Window maximized
    const bool is_window_maximized = window->isMaximized();
    if (is_window_maximized != initial_wnd_maximized)
        g_settings->setBool("window_maximized", is_window_maximized);
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

    core::MainWindowParameters params;
    params.Width = screen_w;
    params.Height = screen_h;

    if (configured_name == "opengl3")
        params.GLType = core::OGL_TYPE_DESKTOP;
    else if (configured_name == "gles2")
        params.GLType = core::OGL_TYPE_ES;
    else
        params.GLType = core::OGL_TYPE_WEB;

    params.FullScreen = fullscreen;
    params.AntiAlias = fsaa;
    params.Maximized = window_maximized;
    params.Resizable = 1;
    params.VSync = vsync;
    params.DriverDebug = g_settings->getBool("opengl_debug");

    verbosestream << "Using the " << configured_name << " video driver" << std::endl;

    window = std::make_unique<core::MainWindow>(params);
}

void RenderSystem::updateDisplayDensity()
{
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
        system->updateDisplayDensity();
    }
    else if (name == "menu_clouds")
        system->enableMenuClouds(g_settings->getBool("menu_clouds"));
}
