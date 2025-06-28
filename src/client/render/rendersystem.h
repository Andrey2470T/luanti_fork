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
class PipelineFactory;
class CameraManager;
	
class RenderSystem
{
	static const img::color8 menu_sky_color;

	Client *client;
	ResourceCache *cache;

    std::unique_ptr<Renderer> renderer;

    std::unique_ptr<LoadScreen> load_screen;
    std::unique_ptr<PipelineFactory> pp_factory;
    std::unique_ptr<CameraManager> cam_mgr;
	std::unique_ptr<main::MainWindow> window;
	
	std::unique_ptr<GUIEnvironment> guienv;
	std::unique_ptr<Hud> hud;
	std::unique_ptr<ShadowRenderer> shadow_renderer;

    std::unique_ptr<AtlasPool> basePool;
    std::unique_ptr<AtlasPool> guiPool;
    std::unique_ptr<FontManager> fontManager;
	
    MyEventReceiver *receiver;

	v2f virtual_size_scale;
    v2u virtual_size{0, 0};

    f32 display_density;
    f32 gui_scaling;
public:
    RenderSystem(Client *_client, ResourceCache *_cache, MyEventReceiver *_receiver);

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

    void setResizable(bool resize)
    {
        window->setResizable(resize);
    }
    void setWindowIcon();

    void setActivePipeline(RenderPipeline *pipeline);
    void setActiveCamera(Camera *camera);

    // If "indef_pos" is given, the value of "percent" is ignored and an indefinite
	// progress bar is drawn.
	void drawLoadScreen(const std::wstring &text,
        f32 dtime = 0, s32 percent = 0, f32 *indef_pos = nullptr);

	void drawScene(bool show_hud,
        bool draw_wield_tool, bool draw_crosshair);
private:
    void initWindow();

    void settingChangedCallback(const std::string &name, void *data);
};
