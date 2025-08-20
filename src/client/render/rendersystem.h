#pragma once

#include <memory>
#include <Image/Color.h>
#include <Render/UniformBuffer.h>
#include <Main/MainWindow.h>
#include <Utils/Matrix4.h>
#include <Render/DrawContext.h>

class Client;
class RenderPipeline;
class Hud;
class GUIEnvironment;
class ShadowRenderer;
class ResourceCache;
class LoadScreen;
class MyEventReceiver;
class AtlasPool;
class FontManager;
class Renderer;
class Camera;
class PipelineCore;
//class CameraManager;
class GameUI;
class Inventory;
	
class RenderSystem
{
	static const img::color8 menu_sky_color;

	Client *client;
	ResourceCache *cache;

    std::unique_ptr<Renderer> renderer;

    std::unique_ptr<LoadScreen> load_screen;
    std::unique_ptr<PipelineCore> pp_core;
    //std::unique_ptr<CameraManager> cam_mgr;
	std::unique_ptr<main::MainWindow> window;
	
    std::unique_ptr<GameUI> gameui;
	//std::unique_ptr<GUIEnvironment> guienv;
	std::unique_ptr<ShadowRenderer> shadow_renderer;

    std::unique_ptr<AtlasPool> basePool;
    std::unique_ptr<AtlasPool> guiPool;
    std::unique_ptr<FontManager> fontManager;
	
    MyEventReceiver *receiver;

    f32 display_density;
    f32 gui_scaling;
    bool menu_clouds;
public:
    RenderSystem(Client *_client, ResourceCache *_cache, MyEventReceiver *_receiver, Inventory *inv);
    ~RenderSystem();

    main::MainWindow *getWindow() const
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
    GameUI *getGameUI() const
    {
        return gameui.get();
    }
    /*CameraManager *ge tCameraManager() const
    {
        return cam_mgr.get();
    }*/
    /*GUIEnvironment *getGUIEnvironment() const
    {
        return guienv.get();
    }*/
    ShadowRenderer *getShadowRenderer() const
    {
        return shadow_renderer.get();
    }
    AtlasPool *getPool(bool basic) const;

    FontManager *getFontManager() const
    {
        return fontManager.get();
    }

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

    void setPipeline(RenderPipeline *pipeline);
    //void setActiveCamera(Camera *camera);

	void drawScene(bool show_hud,
        bool draw_wield_tool, bool draw_crosshair);
private:
    void initWindow();

    static void settingChangedCallback(const std::string &name, void *data);
};
