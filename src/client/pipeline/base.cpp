// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "base.h"
#include "client/client.h"
#include "client/mesh/lighting.h"
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
#include "client/mesh/defaultVertexTypes.h"
#include "client/ui/batcher2d.h"
#include "client/mesh/meshbuffer.h"
#include "client/render/renderer.h"
#include "client/render/sky.h"

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
    base_transform = context.client->getEnv().getLocalPlayer()->getCamera()->getWorldMatrix();
}

void OffsetCameraStep::run(PipelineContext &context)
{
    context.client->getEnv().getLocalPlayer()->getCamera()->setPosition((base_transform * move).getTranslation());
}

/// Draw3D pipeline step
void Draw3D::run(PipelineContext &context)
{
    if (m_target)
        m_target->activate(context);

    auto rnd_sys = context.client->getRenderSystem();
    auto camera = context.client->getEnv().getLocalPlayer()->getCamera();

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
        rnd_sys->getGameUI()->render();

        context.client->getEnv().getLocalPlayer()->getCamera()->drawNametags();
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


ScreenQuad::ScreenQuad(RenderSystem *_rndsys, RenderSource *_textures)
    : rndsys(_rndsys), textures(_textures)
{
    v2u wnd_size = rndsys->getWindowSize();

    quad = std::make_unique<MeshBuffer>(4, 6, true, VType2D);

    rectf screen_rect(v2f(0.0f), v2f(wnd_size.X, wnd_size.Y));
    img::color8 color = img::black;
    Batcher2D::appendImageRectangle(quad.get(), wnd_size,
        rectf(v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)), screen_rect,  {color, color, color, color}, false);
    quad->uploadVertexData();

    prev_size = wnd_size;

    user_exposure_compensation = g_settings->getFloat("exposure_compensation", -1.0f, 1.0f);
    bloom_enabled = g_settings->getBool("enable_bloom");
    volumetric_light_enabled = g_settings->getBool("enable_volumetric_lighting");
}

void ScreenQuad::updateQuad(std::optional<v2u> offset, std::optional<v2u> size)
{
    v2u cur_size = rndsys->getWindowSize();

    if (size.has_value())
        cur_size = size.value();

    if (prev_size == cur_size && !offset.has_value())
        return;

    v2f pos(0.0f, cur_size.Y);

    if (offset.has_value())
        pos = v2f(offset.value().X, offset.value().Y);

    svtSetPos2D(quad.get(), pos, 0);
    svtSetPos2D(quad.get(), pos + v2f(cur_size.X, 0.0f), 1);
    svtSetPos2D(quad.get(), pos + v2f(cur_size.X, -cur_size.Y), 2);
    svtSetPos2D(quad.get(), pos - v2f(0.0f, cur_size.Y), 3);

    quad->uploadVertexData();
}

void ScreenQuad::setShader(bool set_default, std::optional<render::Shader *> new_shader)
{
    if (!set_default && new_shader.has_value()) {
        shader = new_shader.value();
        use_default = false;
        return;
    }

    use_default = true;
}

void ScreenQuad::configureTexturesSettings()
{
    render::TextureSettings settings;
    settings.isRenderTarget = false;
    settings.hasMipMaps = false;
    settings.minF = render::TMF_NEAREST_MIPMAP_NEAREST;
    settings.magF = render::TMAGF_NEAREST;
    settings.anisotropyFilter = 0;
    settings.wrapU = render::TW_CLAMP_TO_EDGE;
    settings.wrapV = render::TW_CLAMP_TO_EDGE;

    for (u8 index : texture_map)
        textures->getTexture(index)->updateParameters(settings, false, true);
}

void ScreenQuad::setBilinearFilter(u8 index, bool value)
{
    auto texture = textures->getTexture(index);

    render::TextureSettings settings;
    settings.minF = value ? render::TMF_LINEAR_MIPMAP_NEAREST : render::TMF_NEAREST_MIPMAP_NEAREST;
    settings.magF = value ? render::TMAGF_LINEAR : render::TMAGF_NEAREST;

    texture->updateParameters(settings, false, false);
}

void ScreenQuad::setPostprocessUniforms(Client *client)
{
    auto rnd = rndsys->getRenderer();
    Shader *curshader = rnd->getShader();

    u32 daynight_ratio = (f32)client->getEnv().getDayNightRatio();
    img::colorf sunlight;
    get_sunlight_color(&sunlight, daynight_ratio);
    curshader->setUniform3Float("mDayLight", v3f(sunlight.R(), sunlight.G(), sunlight.B()));

    f32 animation_timer_delta_f = (f32)client->getEnv().getFrameTimeDelta() / 100000.f;
    curshader->setUniformFloat("mAnimationTimerDelta", animation_timer_delta_f);

    auto player = client->getEnv().getLocalPlayer();
    const auto &lighting = player->getLighting();

    const AutoExposure &exposure_params = lighting.exposure;
    curshader->setUniformFloat("mExposureParams.luminanceMin", std::pow(2.0f, exposure_params.luminance_min));
    curshader->setUniformFloat("mExposureParams.luminanceMax", std::pow(2.0f, exposure_params.luminance_max));
    curshader->setUniformFloat("mExposureParams.exposureCorrection", exposure_params.exposure_correction);
    curshader->setUniformFloat("mExposureParams.speedDarkBright", exposure_params.speed_dark_bright);
    curshader->setUniformFloat("mExposureParams.speedBrightDark", exposure_params.speed_bright_dark);
    curshader->setUniformFloat("mExposureParams.centerWeightPower", exposure_params.center_weight_power);
    curshader->setUniformFloat("mExposureParams.compensationFactor", powf(2.f, user_exposure_compensation));

    if (bloom_enabled) {
        f32 intensity = std::max(lighting.bloom_intensity, 0.0f);
        curshader->setUniformFloat("mBloomIntensity", intensity);

        f32 strength_factor = std::max(lighting.bloom_strength_factor, 0.0f);
        curshader->setUniformFloat("mBloomStrength", strength_factor);
        f32 radius = std::max(lighting.bloom_radius, 0.0f);
        curshader->setUniformFloat("mBloomRadius", radius);
    }

    f32 saturation = lighting.saturation;
    curshader->setUniformFloat("mSaturation", saturation);

    if (volumetric_light_enabled) {
        // Map directional light to screen space
        auto camera = player->getCamera();
        matrix4 transform = camera->getProjectionMatrix();
        transform *= camera->getViewMatrix();

        auto sky = rndsys->getSky();
        auto skyparams = sky->getSkyParams();
        auto sun = sky->getSun();
        if (sun->getVisible()) {
            v3f sun_direction = sun->getDirection(sky->getTimeOfDay(), skyparams.body_orbit_tilt);
            v3f sun_position = camera->getPosition() + sun_direction * 10000.f;
            transform.transformVect(sun_position);
            sun_position.normalize();

            curshader->setUniform3Float("mSunPositionScreen", sun_position);

            f32 sun_brightness = std::clamp(107.143f * sun_direction.Y, 0.f, 1.f);
            curshader->setUniformFloat("mSunBrightness", sun_brightness);
        } else {
            curshader->setUniform3Float("mSunPositionScreen", v3f(0.f, 0.f, -1.f));
            curshader->setUniformFloat("mSunBrightness", 0.0f);
        }

        auto moon = sky->getMoon();
        if (moon->getVisible()) {
            v3f moon_direction = moon->getDirection(sky->getTimeOfDay(), skyparams.body_orbit_tilt);
            v3f moon_position = camera->getPosition() + moon_direction * 10000.f;
            transform.transformVect(moon_position);
            moon_position.normalize();

            curshader->setUniform3Float("mMoonPositionScreen", moon_position);

            f32 moon_brightness = std::clamp(107.143f * moon_direction.Y, 0.f, 1.f);
            curshader->setUniformFloat("mMoonBrightness", moon_brightness);
        } else {
            curshader->setUniform3Float("mMoonPositionScreen", v3f(0.f, 0.f, -1.f));
            curshader->setUniformFloat("mMoonBrightness", 0.0f);
        }

        f32 volumetric_light_strength = lighting.volumetric_light_strength;
        curshader->setUniformFloat("mVolumetricLightStrength", volumetric_light_strength);
    }
}

void ScreenQuad::render(Client *client)
{
    auto rnd = rndsys->getRenderer();
    rnd->setRenderState(false);

    auto ctxt = rnd->getContext();
    if (use_default)
        rnd->setDefaultShader(true);
    else {
        rnd->setShader(shader);
        rnd->setBlending(true);
    }

    for (u8 i = 0; i < texture_map.size(); i++)
        ctxt->setActiveUnit(i, textures->getTexture(texture_map.at(i)));

    rnd->setDefaultUniforms(1.0f, 1);

    if (set_postprocess_uniforms)
        setPostprocessUniforms(client);

    rnd->draw(quad.get());
}

RenderStep *create3DStage(Client *client, v2f scale)
{
    RenderPipeline *pipeline = new RenderPipeline();
    auto step3d = new Draw3D();
    pipeline->addStep(pipeline->own(step3d));

    auto effect = addPostProcessing(pipeline, step3d, scale, client);
    effect->setRenderTarget(pipeline->getOutput());

    return pipeline;
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
