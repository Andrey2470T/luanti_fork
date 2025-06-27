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
class Renderer2D;

enum class FogType : u8
{
	Exp,
	Linear,
	Exp2
};

enum class TMatrix : u8
{
    World,
    View,
    Projection,
    Texture0
};
	
class RenderSystem
{
	static const img::color8 menu_sky_color;

	Client *client;
    RenderPipeline *active_pipeline = nullptr;
	ResourceCache *cache;

    std::unique_ptr<render::DrawContext> context;
    std::unique_ptr<Renderer2D> renderer2D;

    std::unique_ptr<LoadScreen> load_screen;
	std::vector<std::unique_ptr<RenderPipeline>> pipelines;
	std::unique_ptr<main::MainWindow> window;
	
	std::unique_ptr<GUIEnvironment> guienv;
	std::unique_ptr<Hud> hud;
	std::unique_ptr<ShadowRenderer> shadow_renderer;

    std::unique_ptr<render::UniformBuffer> fog_buffer;
    std::unique_ptr<render::UniformBuffer> matrix_buffer;

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
        return gui_scaling * display_density;
    }

    void setResizable(bool resize)
    {
        window->setResizable(resize);
    }
    void setWindowIcon();

    void setActivePipeline(RenderPipeline *pipeline);
    
    // Fog UBO settings
    bool fogEnabled() const;
    void getFogParams(FogType &type, img::color8 &color, f32 &start, f32 &end, f32 &density) const;
    void enableFog(bool enable);
    void setFogParams(FogType type, img::color8 color, f32 start, f32 end, f32 density);


    // Transformation matrices UBO settings
    matrix4 getTransformMatrix(TMatrix type);
    void setTransformMatrix(TMatrix type, const matrix4 &mat);

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
