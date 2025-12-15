// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>
// Copyright (C) 2020 appgurueu, Lars Mueller <appgurulars@gmx.de>

#include "postprocess.h"
#include "client/core/client.h"
#include "client/media/resource.h"
#include "client/render/rendersystem.h"
#include "settings.h"
#include <Render/FrameBuffer.h>
#include "client/map/clientmap.h"
#include "client/player/playercamera.h"
#include "client/render/drawlist.h"
#include "nodedef.h"

PostProcessingStep::PostProcessingStep(RenderSystem *_rnd_sys, RenderSource *_source, render::Shader *shader, const std::vector<u8> &_texture_map)
    : rnd_sys(_rnd_sys)
{
    setRenderSource(_source);
    quad = std::make_unique<ScreenQuad>(rnd_sys, source);
    quad->setTextureMap(_texture_map);
    quad->setShader(false, shader);
    quad->set_postprocess_uniforms = true;
}

void PostProcessingStep::setRenderSource(RenderSource *_source)
{
	source = _source;
}

void PostProcessingStep::setRenderTarget(RenderTarget *_target)
{
	target = _target;
}

void PostProcessingStep::reset(PipelineContext &context)
{
    quad->configureTexturesSettings();
}

void PostProcessingStep::run(PipelineContext &context)
{
	if (target)
		target->activate(context);

    quad->render(context.client);
}

RenderStep *addPostProcessing(RenderPipeline *pipeline, RenderStep *previousStep, v2f scale, Client *client)
{
	auto buffer = pipeline->createOwned<TextureBuffer>();

	// configure texture formats
    auto color_format = selectColorFormat();
    auto depth_format = selectDepthFormat(32, false);

    /*verbosestream << "addPostProcessing(): color = "
		<< video::ColorFormatNames[color_format] << " depth = "
        << video::ColorFormatNames[depth_format] << std::endl;*/

	// init post-processing buffer
	static const u8 TEXTURE_COLOR = 0;
	static const u8 TEXTURE_DEPTH = 1;
	static const u8 TEXTURE_BLOOM = 2;
	static const u8 TEXTURE_EXPOSURE_1 = 3;
	static const u8 TEXTURE_EXPOSURE_2 = 4;
	static const u8 TEXTURE_FXAA = 5;
	static const u8 TEXTURE_VOLUME = 6;

	static const u8 TEXTURE_MSAA_COLOR = 7;
	static const u8 TEXTURE_MSAA_DEPTH = 8;

	static const u8 TEXTURE_SCALE_DOWN = 10;
	static const u8 TEXTURE_SCALE_UP = 20;

	const bool enable_bloom = g_settings->getBool("enable_bloom");
	const bool enable_volumetric_light = g_settings->getBool("enable_volumetric_lighting") && enable_bloom;
	const bool enable_auto_exposure = g_settings->getBool("enable_auto_exposure");

	const std::string antialiasing = g_settings->get("antialiasing");
	const u16 antialiasing_scale = MYMAX(2, g_settings->getU16("fsaa"));

	// This code only deals with MSAA in combination with post-processing. MSAA without
	// post-processing works via a flag at OpenGL context creation instead.
	// To make MSAA work with post-processing, we need multisample texture support,
	// which has higher OpenGL (ES) version requirements.
	// Note: This is not about renderbuffer objects, but about textures,
	// since that's what we use and what Irrlicht allows us to use.

    //const bool msaa_available = driver->queryFeature(video::EVDF_TEXTURE_MULTISAMPLE);
    const bool enable_msaa = antialiasing == "fsaa";// && msaa_available;
    /*if (antialiasing == "fsaa" && !msaa_available)
		warningstream << "Ignoring configured FSAA. FSAA is not supported in "
            << "combination with post-processing by the current video driver." << std::endl;*/

	const bool enable_ssaa = antialiasing == "ssaa";
	const bool enable_fxaa = antialiasing == "fxaa";

	// Super-sampling is simply rendering into a larger texture.
	// Downscaling is done by the final step when rendering to the screen.
	if (enable_ssaa) {
		scale *= antialiasing_scale;
	}

	if (enable_msaa) {
		buffer->setTexture(TEXTURE_MSAA_COLOR, scale, "3d_render_msaa", color_format, false, antialiasing_scale);
		buffer->setTexture(TEXTURE_MSAA_DEPTH, scale, "3d_depthmap_msaa", depth_format, false, antialiasing_scale);
	}

	buffer->setTexture(TEXTURE_COLOR, scale, "3d_render", color_format);
    buffer->setTexture(TEXTURE_EXPOSURE_1, v2u(1), "exposure_1", color_format, /*clear:*/ true);
    buffer->setTexture(TEXTURE_EXPOSURE_2, v2u(1), "exposure_2", color_format, /*clear:*/ true);
	buffer->setTexture(TEXTURE_DEPTH, scale, "3d_depthmap", depth_format);

	// attach buffer to the previous step
	if (enable_msaa) {
        TextureBufferOutput *msaa = pipeline->createOwned<TextureBufferOutput>(
            buffer, std::unordered_map<u8, u8> {{TEXTURE_MSAA_COLOR, 0}}, std::pair<u8, u8> {TEXTURE_MSAA_DEPTH, 0});
		previousStep->setRenderTarget(msaa);
        TextureBufferOutput *normal = pipeline->createOwned<TextureBufferOutput>(
            buffer, std::unordered_map<u8, u8> {{TEXTURE_COLOR, 0}}, std::pair<u8, u8> {TEXTURE_DEPTH, 0});
		pipeline->addStep<ResolveMSAAStep>(msaa, normal);
	} else {
        previousStep->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(
            buffer, std::unordered_map<u8, u8> {{TEXTURE_COLOR, 0}}, std::pair<u8, u8> {TEXTURE_DEPTH, 0}));
	}

    render::Shader *shader;

    auto cache = client->getResourceCache();
    auto rndsys = client->getRenderSystem();

    u8 final_stage_source = TEXTURE_COLOR;

    if (g_settings->getBool("enable_post_processing")) {

        // Number of mipmap levels of the bloom downsampling texture
        const u8 MIPMAP_LEVELS = 4;

        // color_format can be a normalized integer format, but bloom requires
        // values outside of [0,1] so this needs to be a different one.
        const auto bloom_format = img::PF_RGBA16F;

        // post-processing stage

        u8 source = TEXTURE_COLOR;

        // common downsampling step for bloom or autoexposure
        if (enable_bloom || enable_auto_exposure) {

            v2f downscale = scale * 0.5f;
            for (u8 i = 0; i < MIPMAP_LEVELS; i++) {
                buffer->setTexture(TEXTURE_SCALE_DOWN + i, downscale, std::string("downsample") + std::to_string(i), bloom_format);
                if (enable_bloom)
                    buffer->setTexture(TEXTURE_SCALE_UP + i, downscale, std::string("upsample") + std::to_string(i), bloom_format);
                downscale *= 0.5f;
            }

            if (enable_bloom) {
                buffer->setTexture(TEXTURE_BLOOM, scale, "bloom", bloom_format);

                // get bright spots
                auto shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "extract_bloom");
                RenderStep *extract_bloom = pipeline->addStep<PostProcessingStep>(rndsys, buffer, shader, std::vector<u8> { source, TEXTURE_EXPOSURE_1 });
                extract_bloom->setRenderSource(buffer);
                extract_bloom->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, std::unordered_map<u8, u8>{{TEXTURE_BLOOM, 0}}));
                source = TEXTURE_BLOOM;
            }

            if (enable_volumetric_light) {
                buffer->setTexture(TEXTURE_VOLUME, scale, "volume", bloom_format);

                shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "volumetric_light");
                auto volume = pipeline->addStep<PostProcessingStep>(rndsys, buffer, shader, std::vector<u8> { source, TEXTURE_DEPTH });
                volume->setRenderSource(buffer);
                volume->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, std::unordered_map<u8, u8>{{TEXTURE_VOLUME, 0}}));
                source = TEXTURE_VOLUME;
            }

            // downsample
            shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "bloom_downsample");
            for (u8 i = 0; i < MIPMAP_LEVELS; i++) {
                auto step = pipeline->addStep<PostProcessingStep>(rndsys, buffer, shader, std::vector<u8> { source });
                step->setRenderSource(buffer);
                step->getQuad()->setBilinearFilter(0, true);
                step->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, std::unordered_map<u8, u8>{{TEXTURE_SCALE_DOWN + i, 0}}));
                source = TEXTURE_SCALE_DOWN + i;
            }
        }

        // Bloom pt 2
        if (enable_bloom) {
            // upsample
            shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "bloom_upsample");
            for (u8 i = MIPMAP_LEVELS - 1; i > 0; i--) {
                auto step = pipeline->addStep<PostProcessingStep>(rndsys, buffer, shader, std::vector<u8> { u8(TEXTURE_SCALE_DOWN + i - 1), source });
                step->setRenderSource(buffer);
                step->getQuad()->setBilinearFilter(0, true);
                step->getQuad()->setBilinearFilter(1, true);
                step->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, std::unordered_map<u8, u8>{{u8(TEXTURE_SCALE_UP + i - 1), 0}}));
                source = TEXTURE_SCALE_UP + i - 1;
            }
        }

        // Dynamic Exposure pt2
        if (enable_auto_exposure) {
            shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "update_exposure");
            auto update_exposure = pipeline->addStep<PostProcessingStep>(rndsys, buffer, shader, std::vector<u8> { TEXTURE_EXPOSURE_1, u8(TEXTURE_SCALE_DOWN + MIPMAP_LEVELS - 1) });
            update_exposure->getQuad()->setBilinearFilter(1, true);
            update_exposure->setRenderSource(buffer);
            update_exposure->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, std::unordered_map<u8, u8>{{TEXTURE_EXPOSURE_2, 0}}));
        }

        // FXAA
        if (enable_fxaa) {
            final_stage_source = TEXTURE_FXAA;

            buffer->setTexture(TEXTURE_FXAA, scale, "fxaa", color_format);
            shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "fxaa");
            PostProcessingStep *effect = pipeline->createOwned<PostProcessingStep>(rndsys, buffer, shader, std::vector<u8> { TEXTURE_COLOR });
            pipeline->addStep(effect);
            effect->getQuad()->setBilinearFilter(0, true);
            effect->setRenderSource(buffer);
            effect->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, std::unordered_map<u8, u8>{{TEXTURE_FXAA, 0}}));
        }
    }

	// final merge
    shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "second_stage");
    PostProcessingStep *effect = pipeline->createOwned<PostProcessingStep>(rndsys, buffer, shader, std::vector<u8> { final_stage_source, TEXTURE_SCALE_UP, TEXTURE_EXPOSURE_2 });
	pipeline->addStep(effect);

    auto screen_quad = effect->getQuad();
	if (enable_ssaa)
        screen_quad->setBilinearFilter(0, true);
    screen_quad->setBilinearFilter(1, true);
	effect->setRenderSource(buffer);

	if (enable_auto_exposure) {
		pipeline->addStep<SwapTexturesStep>(buffer, TEXTURE_EXPOSURE_1, TEXTURE_EXPOSURE_2);
	}

    pipeline->addStep<MapPostFxStep>(buffer, final_stage_source);
    pipeline->addStep<DamageFlashStep>(buffer, final_stage_source);

	return effect;
}

void MapPostFxStep::setRenderTarget(RenderTarget * _target)
{
    target = _target;
}

void MapPostFxStep::run(PipelineContext &context)
{
    if (target)
        target->activate(context);

    Map *map = dynamic_cast<Map *>(&context.client->getEnv().getClientMap());
    auto camera = context.client->getEnv().getLocalPlayer()->getCamera();

    MapNode n = map->getNode(floatToInt(camera->getPosition(), BS));

    const ContentFeatures& features = map->getNodeDefManager()->get(n);
    img::color8 post_color = features.post_effect_color;

    if (features.post_effect_color_shaded) {
        auto apply_light = [] (u32 color, u32 light) {
            return std::clamp(round32(color * light / 255.0f), 0, 255);
        };
        auto light_color = camera->getLightColor();
        post_color.R(apply_light(post_color.R(), light_color.R()));
        post_color.G(apply_light(post_color.G(), light_color.G()));
        post_color.B(apply_light(post_color.B(), light_color.B()));
    }

    auto rnd_sys = context.client->getRenderSystem();
    auto drawlist = rnd_sys->getDrawList();
    // If the camera is in a solid node, make everything black.
    // (first person mode only)
    if (features.solidness == 2 && camera->getCameraMode() == CAMERA_MODE_FIRST &&
            !drawlist->getDrawControl().allow_noclip) {
        post_color = img::black;
    }

    if (post_color.A() != 0) {
        if (!quad) {
            quad = std::make_unique<ScreenQuad>(rnd_sys, source);
            quad->setTextureMap({texture_index});
        }

        quad->updateQuad();
        // Draw a full-screen rectangle
        quad->render(context.client);
    }
}

void DamageFlashStep::setRenderTarget(RenderTarget * _target)
{
    target = _target;
}

void DamageFlashStep::run(PipelineContext &context)
{
    if (target)
        target->activate(context);

    f32 currentDamage = context.client->getEnv().damage_flash;

    if (currentDamage <= 0.0f)
        return;

    img::color8 color(img::PF_RGBA8, 180, 0, 0, currentDamage);

    auto rnd_sys = context.client->getRenderSystem();
    if (!quad) {
        quad = std::make_unique<ScreenQuad>(rnd_sys, source);
        quad->setTextureMap({texture_index});
        quad->updateQuad();
    }

    if (currentDamage != prevDamage) {
        auto buffer = quad->getBuffer();
        MeshOperations::colorizeMesh(buffer, color);
        buffer->uploadVertexData();

        prevDamage = currentDamage;
    }

    // Draw a full-screen rectangle
    quad->render(context.client);
}

void ResolveMSAAStep::run(PipelineContext &context)
{
    msaa_fbo->getRenderTarget(context)->blitTo(target_fbo->getRenderTarget(context));
}
