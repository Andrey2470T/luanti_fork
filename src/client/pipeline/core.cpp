#include "core.h"
#include "settings.h"
#include "pipeline.h"
#include "plain.h"
#include "anaglyph.h"
#include "interlaced.h"
#include "sidebyside.h"
#include "postprocess.h"
//#include "client/shadows/dynamicShadowsRender.h"
#include "log.h"
#include "client/core/client.h"
#include "client/ui/gameui.h"

PipelineCore::PipelineCore(Client *_client, bool enable_shadows)
    : client(_client), rndSystem(client->getRenderSystem()), pipeline(new RenderPipeline())
{
    std::string stereoMode = g_settings->get("3d_mode");

    //if (enable_shadows)
        //activePipeline->addStep<RenderShadowMapStep>();

    if (stereoMode == "none")
        populatePlainPipeline(pipeline.get(), client);
    else if (stereoMode == "anaglyph")
        populateAnaglyphPipeline(pipeline.get(), client);
    else if (stereoMode == "interlaced")
        populateInterlacedPipeline(pipeline.get(), client);
    else if (stereoMode == "sidebyside")
        populateSideBySidePipeline(pipeline.get(), client, false, false, virtual_size_scale);
    else if (stereoMode == "topbottom")
        populateSideBySidePipeline(pipeline.get(), client, true, false, virtual_size_scale);
    else if (stereoMode == "crossview")
        populateSideBySidePipeline(pipeline.get(), client, false, true, virtual_size_scale);
    else {
        errorstream << "Invalid rendering mode: " << stereoMode << std::endl;
        populatePlainPipeline(pipeline.get(), client);
    }

    v2u wndsize = rndSystem->getWindowSize();
    virtual_size = v2u(wndsize.X * virtual_size_scale.X, wndsize.Y * virtual_size_scale.Y);
}

void PipelineCore::run(img::color8 skycolor, bool show_hud)
{
    Hud *hud = rndSystem->getGameUI()->getHud();
    //ShadowRenderer *shadow_renderer = rndSystem->getShadowRenderer();
	v2u wndsize = rndSystem->getWindowSize();
	
    PipelineContext context(client, hud, nullptr, skycolor, wndsize);
    context.show_hud = show_hud;

    pipeline->reset(context);
    pipeline->run(context);
}
