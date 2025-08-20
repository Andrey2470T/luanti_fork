#pragma once

#include "client/render/rendersystem.h"

class RenderPipeline;

class PipelineCore
{
	Client *client;
    RenderSystem *rndSystem;
    std::unique_ptr<RenderPipeline> pipeline;

    v2f virtual_size_scale{1.0f};
    v2u virtual_size{0, 0};
public:
    PipelineCore(Client *_client, bool enable_shadows);

    RenderPipeline *getPipeline() const
    {
        return pipeline.get();
    }

    v2u getVirtualSize() const
    {
        return virtual_size;
    }

    void run(img::color8 skycolor, bool show_hud,
        bool draw_wield_tool, bool draw_crosshair);
};
