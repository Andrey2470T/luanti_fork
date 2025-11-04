#pragma once

#include <memory>
#include <Image/Color.h>
#include <Render/UniformBuffer.h>
#include <Core/MainWindow.h>
#include <Utils/Matrix4.h>
#include <Render/DrawContext.h>

class Client;
class RenderPipeline;
class Hud;
//class ShadowRenderer;
class ResourceCache;
class LoadScreen;
class AtlasPool;
class FontManager;
class Renderer;
class Camera;
class PipelineCore;
class DistanceSortedDrawList;
class ParticleManager;
class Sky;
class Clouds;
class GameUI;
class TransformNodeManager;
class AnimationManager;
class Minimap;
class GameFormSpec;
class InputHandler;

namespace gui
{
class IGUIEnvironment;
}

class RenderSystem
{
    Client *client = nullptr;
	ResourceCache *cache;

    std::unique_ptr<Renderer> renderer;

    std::unique_ptr<LoadScreen> load_screen;
    std::unique_ptr<PipelineCore> pp_core;
	std::unique_ptr<core::MainWindow> window;
	
    std::unique_ptr<DistanceSortedDrawList> drawlist;
    std::unique_ptr<ParticleManager> particle_manager;
    std::unique_ptr<Sky> sky;
    std::unique_ptr<Clouds> clouds;

    std::unique_ptr<TransformNodeManager> node_mgr;
    std::unique_ptr<AnimationManager> anim_mgr;

    std::unique_ptr<GameUI> gameui;
    std::unique_ptr<gui::IGUIEnvironment> guienv;
    std::unique_ptr<GameFormSpec> gameformspec;
	//std::unique_ptr<ShadowRenderer> shadow_renderer;

    std::unique_ptr<AtlasPool> basePool;
    std::unique_ptr<AtlasPool> guiPool;
    std::unique_ptr<FontManager> fontManager;

    f32 display_density;
    f32 gui_scaling;
    bool menu_clouds;
public:
    RenderSystem(ResourceCache *_cache);
    ~RenderSystem();

    void initLoadScreen();
    void initRenderEnvironment(Client *_client);

    core::MainWindow *getWindow() const
    {
        return window.get();
    }
    Renderer *getRenderer() const
    {
        return renderer.get();
    }
    LoadScreen *getLoadScreen() const
    {
        return load_screen.get();
    }
    PipelineCore *getPipelineCore() const
    {
        return pp_core.get();
    }
    DistanceSortedDrawList *getDrawList() const
    {
        return drawlist.get();
    }
    ParticleManager *getParticleManager() const
    {
        return particle_manager.get();
    }
    Sky *getSky() const
    {
        return sky.get();
    }
    Clouds *getClouds() const
    {
        return clouds.get();
    }
    TransformNodeManager *getNodeManager() const
    {
        return node_mgr.get();
    }
    AnimationManager *getAnimationManager() const
    {
        return anim_mgr.get();
    }
    GameUI *getGameUI() const
    {
        return gameui.get();
    }
    Hud *getHud() const;
    gui::IGUIEnvironment *getGUIEnvironment() const
    {
        return guienv.get();
    }
    GameFormSpec *getGameFormSpec() const
    {
        return gameformspec.get();
    }
    /*ShadowRenderer *getShadowRenderer() const
    {
        return shadow_renderer.get();
    }*/
    AtlasPool *getPool(bool basic) const;

    FontManager *getFontManager() const
    {
        return fontManager.get();
    }
    Minimap *getDefaultMinimap() const;

    v2u getWindowSize() const
    {
        return window->getWindowSize();
    }
    f32 getDisplayDensity() const
    {
        return display_density;
    }
    f32 getGUIScaling() const
    {
    	return gui_scaling;
    }
    f32 getScaleFactor() const
    {
        return gui_scaling * display_density;
    }

    void setGUIScaling(f32 scaling)
    {
        gui_scaling = scaling;
    }
    void setDisplyDensity(f32 density)
    {
        display_density = density;
    }
    void enableMenuClouds(bool enable)
    {
        menu_clouds = enable;
    }
    void setResizable(bool resize)
    {
        window->setResizable(resize);
    }
    void setWindowIcon();

    void activateAtlas(img::Image *img, bool basic_pool=true);

    void buildGUIAtlas();

    void beginDraw(u16 flags, img::color8 color=img::black, f32 depth=1.0f, u8 stencil=0);
    void endDraw();

    bool run();

    void render();

    void autosaveScreensizeAndCo(v2u initial_screen_size, bool initial_wnd_maximized);

    void updateDisplayDensity();
private:
    void initWindow();

    static void settingChangedCallback(const std::string &name, void *data);
};
