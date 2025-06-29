#include "factory.h"
#include "settings.h"
#include "pipeline.h"
#include "client/shadows/dynamicShadowsRender.h"
#include "log.h"

PipelineFactory::PipelineFactory(RenderSystem *rnd_system, bool enable_shadows)
    : rndSystem(rnd_system), activePipeline(new Pipeline())
{
    std::string stereoMode = g_settings->get("3d_mode");

    if (enable_shadows)
        activePipeline->addStep<RenderShadowMapStep>();

    if (stereoMode == "none")
        createPlainPipeline();
    else if (stereoMode == "anaglyph")
        createAnaglyphPipeline();
    else if (stereoMode == "interlaced")
        createInterlacedPipeline();
    else if (stereoMode == "sidebyside")
        createSideBySidePipeline();
    else if (stereoMode == "topbottom")
        createSideBySidePipeline();
    else if (stereoMode == "crossview")
        createSideBySidePipeline();
    else {
        errorstream << "Invalid rendering mode: " << stereoMode << std::endl;
        createPlainPipeline();
    }

    v2u wndsize = rndSystem->getWindowSize();
    virtual_size = v2u(wndsize.X * virtual_size_scale.X, wndsize.Y * virtual_size_scale.Y);
}

void PipelineFactory::run(img::color8 skycolor, bool show_hud,
         bool draw_wield_tool, bool draw_crosshair)
{
    /*PipelineContext context(device, client, hud, shadow_renderer, _skycolor, screensize);
    context.draw_crosshair = _draw_crosshair;
    context.draw_wield_tool = _draw_wield_tool;
    context.show_hud = _show_hud;

    pipeline->reset(context);
    pipeline->run(context);*/
}
/*#include "log.h"
#include "plain.h"
#include "anaglyph.h"
#include "interlaced.h"
#include "sidebyside.h"
#include "secondstage.h"
#include "client/shadows/dynamicshadowsrender.h"

struct CreatePipelineResult
{
	v2f virtual_size_scale;
	ShadowRenderer *shadow_renderer { nullptr };
	RenderPipeline *pipeline { nullptr };
};

void createPipeline(const std::string &stereo_mode, IrrlichtDevice *device, Client *client, Hud *hud, CreatePipelineResult &result);

RenderingCore *createRenderingCore(const std::string &stereo_mode, IrrlichtDevice *device,
		Client *client, Hud *hud)
{
	CreatePipelineResult created_pipeline;
	createPipeline(stereo_mode, device, client, hud, created_pipeline);
	return new RenderingCore(device, client, hud,
			created_pipeline.shadow_renderer, created_pipeline.pipeline, created_pipeline.virtual_size_scale);
}

void createPipeline(const std::string &stereo_mode, IrrlichtDevice *device, Client *client, Hud *hud, CreatePipelineResult &result)
{
	result.shadow_renderer = createShadowRenderer(device, client);
	result.virtual_size_scale = v2f(1.0f);
	result.pipeline = new RenderPipeline();

	if (result.shadow_renderer)
		result.pipeline->addStep<RenderShadowMapStep>();

	if (stereo_mode == "none") {
		populatePlainPipeline(result.pipeline, client);
		return;
	}
	if (stereo_mode == "anaglyph") {
		populateAnaglyphPipeline(result.pipeline, client);
		return;
	}
	if (stereo_mode == "interlaced") {
		populateInterlacedPipeline(result.pipeline, client);
		return;
	}
	if (stereo_mode == "sidebyside") {
		populateSideBySidePipeline(result.pipeline, client, false, false, result.virtual_size_scale);
		return;
	}
	if (stereo_mode == "topbottom") {
		populateSideBySidePipeline(result.pipeline, client, true, false, result.virtual_size_scale);
		return;
	}
	if (stereo_mode == "crossview") {
		populateSideBySidePipeline(result.pipeline, client, false, true, result.virtual_size_scale);
		return;
	}

	// fallback to plain renderer
	errorstream << "Invalid rendering mode: " << stereo_mode << std::endl;
	populatePlainPipeline(result.pipeline, client);
}*/
