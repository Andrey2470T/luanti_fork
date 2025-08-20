#include "core.h"
#include "settings.h"
#include "pipeline.h"
#include "plain.h"
#include "anaglyph.h"
#include "interlaced.h"
#include "sidebyside.h"
#include "secondstage.h"
#include "client/shadows/dynamicShadowsRender.h"
#include "log.h"
#include "client/client.h"
#include "client/ui/gameui.h"

PipelineCore::PipelineCore(Client *_client, bool enable_shadows)
    : client(_client), rndSystem(client->getRenderSystem()), pipeline(new RenderPipeline())
{
    std::string stereoMode = g_settings->get("3d_mode");

    //if (enable_shadows)
        //activePipeline->addStep<RenderShadowMapStep>();

    if (stereoMode == "none")
        populatePlainPipeline(pipeline.get());
    else if (stereoMode == "anaglyph")
        populateAnaglyphPipeline(pipeline.get());
    else if (stereoMode == "interlaced")
        populateInterlacedPipeline(pipeline.get());
    else if (stereoMode == "sidebyside" ||
            stereoMode == "topbottom" ||
            stereoMode == "crossview")
        populateSideBySidePipeline(pipeline.get());
    else {
        errorstream << "Invalid rendering mode: " << stereoMode << std::endl;
        populatePlainPipeline(pipeline.get());
    }

    v2u wndsize = rndSystem->getWindowSize();
    virtual_size = v2u(wndsize.X * virtual_size_scale.X, wndsize.Y * virtual_size_scale.Y);
}

void PipelineCore::run(img::color8 skycolor, bool show_hud,
         bool draw_wield_tool, bool draw_crosshair)
{
    Hud *hud = rndSystem->getGameUI()->getHud();
	ShadowRenderer *shadow_renderer = rndSystem->getShadowRenderer();
	v2u wndsize = rndSystem->getWindowSize();
	
    PipelineContext context(client, hud, shadow_renderer, skycolor, wndsize);
    context.draw_crosshair = draw_crosshair;
    context.draw_wield_tool = draw_wield_tool;
    context.show_hud = show_hud;

    pipeline->reset(context);
    pipeline->run(context);
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
