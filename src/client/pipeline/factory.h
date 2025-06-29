#pragma once

#include "client/render/rendersystem.h"
#include "client/render/renderer.h"


class Pipeline;

class PipelineFactory
{
    RenderSystem *rndSystem;
    std::unique_ptr<Pipeline> activePipeline;

    v2f virtual_size_scale{1.0f};
    v2u virtual_size{0, 0};
public:
    PipelineFactory(RenderSystem *rnd_system, bool enable_shadows);

    Pipeline *getActivePipeline() const
    {
        return activePipeline.get();
    }

    v2u getVirtualSize() const
    {
        return virtual_size;
    }

    //void addPipeline(const std::string &stereoMode);

    void run(img::color8 skycolor, bool show_hud,
        bool draw_wield_tool, bool draw_crosshair);
private:
    void createPlainPipeline();
    void createAnaglyphPipeline();
    void createInterlacedPipeline();
    void createSideBySidePipeline();
};
