// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "base.h"
#include "client/client.h"
#include "client/player/playercamera.h"
#include "constants.h"
#include "settings.h"
#include "postprocess.h"
#include "client/render/rendersystem.h"
#include "client/render/sky.h"
#include "client/render/clouds.h"
#include "client/render/drawlist.h"
#include "client/render/particles.h"
#include "client/ui/gameui.h"
#include "client/ui/hud.h"

OffsetCameraStep::OffsetCameraStep(f32 eye_offset)
{
    move.setTranslation(v3f(eye_offset, 0.0f, 0.0f));
}


OffsetCameraStep::OffsetCameraStep(bool right_eye)
{
	f32 eye_offset = BS * g_settings->getFloat("3d_paralax_strength", -0.087f, 0.087f) * (right_eye ? 1 : -1);
    move.setTranslation(v3f(eye_offset, 0.0f, 0.0f));
}

void OffsetCameraStep::reset(PipelineContext &context)
{
    base_transform = context.client->getCamera()->getWorldMatrix();
}

void OffsetCameraStep::run(PipelineContext &context)
{
	context.client->getCamera()->setPosition((base_transform * move).getTranslation());
}

/// Draw3D pipeline step
void Draw3D::run(PipelineContext &context)
{
    if (m_target)
        m_target->activate(context);

    auto rnd_sys = context.client->getRenderSystem();
    auto camera = context.client->getCamera();

    rnd_sys->getSky()->render(camera);
    rnd_sys->getClouds()->render();
    rnd_sys->getDrawList()->render();
    rnd_sys->getParticleManager()->renderParticles();
}

void DrawHUD::run(PipelineContext &context)
{
    if (context.show_hud) {
       // if (context.shadow_renderer)
        //    context.shadow_renderer->drawDebug();

        auto rnd_sys = context.client->getRenderSystem();
        rnd_sys->getGameUI()->getHud()->render();

        context.client->getCamera()->drawNametags();
    }
        /*context.hud->resizeHotbar();

        if (context.draw_crosshair)
            context.hud->drawCrosshair();

        context.hud->drawLuaElements(context.client->getCamera()->getOffset());
        context.client->getCamera()->drawNametags();
    }
    context.device->getGUIEnvironment()->drawAll();*/
}

/*void RenderShadowMapStep::run(PipelineContext &context)
{
    // This is necessary to render shadows for animations correctly
    context.device->getSceneManager()->getRootSceneNode()->OnAnimate(context.device->getTimer()->getTime());
    context.shadow_renderer->update();
}*/

std::unique_ptr<RenderStep> create3DStage(Client *client, v2f scale)
{
    RenderPipeline *pipeline = new RenderPipeline();
    auto step3d = new Draw3D();
    pipeline->addStep(step3d);

    auto effect = addPostProcessing(pipeline, step3d, scale, client);
    effect->setRenderTarget(pipeline->getOutput());

    return std::unique_ptr<RenderStep>(pipeline);
}

img::PixelFormat selectColorFormat()
{
    u32 bits = g_settings->getU32("post_processing_texture_bits");

    if (bits == 32)
        return img::PF_RGBA32F;
    if (bits == 16)
        return img::PF_RGBA16F;
    return img::PF_RGBA8;
}

img::PixelFormat selectDepthFormat(u32 bits, bool stencil)
{
    if (stencil)
        return img::PF_D24S8;
    if (bits == 32)
        return img::PF_D32;
    return img::PF_D16;
}
