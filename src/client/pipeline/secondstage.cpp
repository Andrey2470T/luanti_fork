// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>
// Copyright (C) 2020 appgurueu, Lars Mueller <appgurulars@gmx.de>

#include "secondstage.h"
#include "client/core/client.h"
#include "client/media/shader.h"
#include "client/render/tile.h"
#include "settings.h"
#include <Scene/ISceneManager.h>
#include <Video/RenderTarget.h>

PostProcessingStep::PostProcessingStep(u32 _shader_id, const std::vector<u8> &_texture_map, video::E_BLEND_MODE _blend_mode) :
	shader_id(_shader_id), blend_mode(_blend_mode), texture_map(_texture_map)
{
	assert(texture_map.size() <= video::MATERIAL_MAX_TEXTURES);
	configureMaterial();
}

void PostProcessingStep::configureMaterial()
{
	material.UseMipMaps = false;
	material.ZBuffer = video::ECFN_LESSEQUAL;
	material.ZWriteEnable = video::EZW_ON;
	material.BlendMode = blend_mode;
	for (u32 k = 0; k < texture_map.size(); ++k) {
		material.TextureLayers[k].AnisotropicFilter = 0;
		material.TextureLayers[k].MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;
		material.TextureLayers[k].MagFilter = video::ETMAGF_NEAREST;
		material.TextureLayers[k].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		material.TextureLayers[k].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}

void PostProcessingStep::setRenderSource(RenderSource *_source)
{
	source = _source;
}

void PostProcessingStep::setRenderTarget(RenderTarget *_target)
{
	target = _target;
}

void PostProcessingStep::overrideTextureMap(const std::vector<u8> &_texture_map)
{
	assert(_texture_map.size() <= video::MATERIAL_MAX_TEXTURES);
	texture_map = _texture_map;
	configureMaterial();
}

void PostProcessingStep::reset(PipelineContext &context)
{
}

void PostProcessingStep::run(PipelineContext &context)
{
	if (target)
		target->activate(context);

	// attach the shader
	material.MaterialType = context.client->getShaderSource()->getShaderInfo(shader_id).material;

	auto driver = context.device->getVideoDriver();

	for (u32 i = 0; i < texture_map.size(); i++)
		material.TextureLayers[i].Texture = source->getTexture(texture_map[i]);

	static const video::SColor color = video::SColor(0, 0, 0, 255);
	static const scene::Vertex3D vertices[4] = {
		{{1.0, -1.0, 0.0}, {0.0, 0.0, -1.0},
			color, {1.0, 0.0}},
		{{-1.0, -1.0, 0.0}, {0.0, 0.0, -1.0},
			color, {0.0, 0.0}},
		{{-1.0, 1.0, 0.0}, {0.0, 0.0, -1.0},
			color, {0.0, 1.0}},
		{{1.0, 1.0, 0.0}, {0.0, 0.0, -1.0},
			color, {1.0, 1.0}},
	};
	static const u16 indices[6] = {0, 1, 2, 2, 3, 0};
	driver->setMaterial(material);
	driver->drawVertexPrimitiveList(&vertices, 4, &indices, 6);
}

void PostProcessingStep::setBilinearFilter(u8 index, bool value)
{
	assert(index < video::MATERIAL_MAX_TEXTURES);
	material.TextureLayers[index].MinFilter = value ? video::ETMINF_LINEAR_MIPMAP_NEAREST : video::ETMINF_NEAREST_MIPMAP_NEAREST;
	material.TextureLayers[index].MagFilter = value ? video::ETMAGF_LINEAR : video::ETMAGF_NEAREST;
}

std::vector<std::string> PostProcessingPipeline::m_special_steps = {"Draw3D", "ResolveMSAA", "SwapTextures"};

PostProcessingStep *PostProcessingPipeline::addPostprocessStep(const std::string &name, u32 shader_id, const std::vector<u8> &texture_map, bool alpha_blend)
{
	auto step = addStep<PostProcessingStep>(
		name, shader_id, texture_map, alpha_blend ? video::EBM_ALPHA : video::EBM_NONE);

	PostProcessingStepDefinition def = {
		name, shader_id, "PostProcessing", texture_map, {}, {0.0f, 0.0f, 1.0f, 1.0f}, {},
		alpha_blend ? video::EBM_ALPHA : video::EBM_NONE, 1.0f
	};
	m_steps_defs.push_back(def);
	m_steps_state.push_back({step, name});
	return step;
}

PostProcessingStep *PostProcessingPipeline::addPostProcessStep(const PostProcessingStepDefinition &def)
{
	auto step = addStep<PostProcessingStep>(def.name, def.shader_id, def.inputs, def.blend_mode);
	m_steps_defs.push_back(def);
	m_steps_state.push_back({step, def.name});
	return step;
}

void PostProcessingPipeline::addDraw3DStep(Draw3D *step)
{
	addStep(m_special_steps[(u8)SpecialSteps::DRAW3D], own(std::unique_ptr<RenderStep>(step)));
	m_draw3d = step;
}

ResolveMSAAStep *PostProcessingPipeline::addResolveMSAAStep(TextureBufferOutput *msaa, TextureBufferOutput *normal)
{
	m_resolve_msaa = addStep<ResolveMSAAStep>(
		m_special_steps[(u8)SpecialSteps::RESOLVE_MSAA], msaa, normal);
	return m_resolve_msaa;
}

SwapTexturesStep *PostProcessingPipeline::addSwapTexturesStep(TextureBuffer *buffer, u8 texture_a, u8 texture_b)
{
	m_swap_textures = addStep<SwapTexturesStep>(m_special_steps[(u8)SpecialSteps::SWAP_TEXTURES], buffer, texture_a, texture_b);
	return m_swap_textures;
}

PostProcessingStep *PostProcessingPipeline::getPostprocessStep(const std::string &name)
{
	if (std::find(m_special_steps.begin(), m_special_steps.end(), name) != m_special_steps.end())
		return nullptr;
	auto step = getStep(name);

	if (step)
		return (PostProcessingStep *)step;
	return nullptr;
}

const PostProcessingStepDefinition &PostProcessingPipeline::getPostProcessStepDef(const std::string &name)
{
	auto found = std::find_if(m_steps_defs.begin(), m_steps_defs.end(),
		[name] (const auto &def) { return def.name == name; });

	if (found == m_steps_defs.end()) {
		static PostProcessingStepDefinition fallback;
		return fallback;
	}
	return *found;
}

void PostProcessingPipeline::overrideStepInputs(const std::string &name, const std::vector<u8> &inputs)
{
	auto &def = const_cast<PostProcessingStepDefinition &>(getPostProcessStepDef(name));
	auto step = getPostprocessStep(name);

	if (!step)
		return;
	def.inputs = inputs;
	step->overrideTextureMap(inputs);
}

void PostProcessingPipeline::overrideStepOutputs(const std::string &name, const std::vector<std::pair<u8, u8>> &outputs)
{
	auto &def = const_cast<PostProcessingStepDefinition &>(getPostProcessStepDef(name));
	auto step = getPostprocessStep(name);

	if (!step)
		return;
	def.outputs = outputs;

	auto tbuf_output = (TextureBufferOutput *)step->getRenderTarget();

	if (!tbuf_output)
		return;

	tbuf_output->overrideTextureMap(outputs);
}

void PostProcessingPipeline::run(PipelineContext &context)
{
	v2u32 original_size = context.target_size;
	context.target_size = v2u32(original_size.X * scale.X, original_size.Y * scale.Y);

	for (auto &object : m_objects)
		object->reset(context);

	m_draw3d->run(context);

	if (m_resolve_msaa)
		m_resolve_msaa->run(context);

	for (auto &step_state : m_steps_state)
	{
		if (step_state.enabled)
			step_state.step->run(context);
	}

	if (m_swap_textures)
		m_swap_textures->run(context);

	context.target_size = original_size;
}

RenderStep *addPostProcessing(PostProcessingPipeline *pipeline, RenderStep *previousStep, v2f scale, Client *client)
{
	auto buffer = pipeline->createTextureBuffer("PostProcessing");
	auto driver = client->getSceneManager()->getVideoDriver();

	// configure texture formats
	video::ECOLOR_FORMAT color_format = selectColorFormat(driver);
	video::ECOLOR_FORMAT depth_format = selectDepthFormat(driver);

	verbosestream << "addPostProcessing(): color = "
		<< video::pixelFormatsInfo[color_format].name << " depth = "
		<< video::pixelFormatsInfo[depth_format].name << std::endl;

	// init post-processing buffer
	static const u8 TEXTURE_COLOR = 0;
	static const u8 TEXTURE_DEPTH = 1;
	static const u8 TEXTURE_BLOOM_MASK = 2;
	static const u8 TEXTURE_BLOOM = 3;
	static const u8 TEXTURE_EXPOSURE_1 = 4;
	static const u8 TEXTURE_EXPOSURE_2 = 5;
	static const u8 TEXTURE_FXAA = 6;
	static const u8 TEXTURE_VOLUME = 7;

	static const u8 TEXTURE_MSAA_COLOR = 8;
	static const u8 TEXTURE_MSAA_DEPTH = 9;
	static const u8 TEXTURE_MSAA_BLOOM_MASK = 10;

	static const u8 TEXTURE_SCALE_DOWN = 11;
	static const u8 TEXTURE_SCALE_UP = 15;
	static const u8 TEXTURE_NORMAL = 19;

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

	const bool msaa_available = driver->getFeatures().TextureMultisampleSupported;
	const bool enable_msaa = antialiasing == "fsaa" && msaa_available;
	if (antialiasing == "fsaa" && !msaa_available)
		warningstream << "Ignoring configured FSAA. FSAA is not supported in "
			<< "combination with post-processing by the current video driver." << std::endl;

	const bool enable_ssaa = antialiasing == "ssaa";
	const bool enable_fxaa = antialiasing == "fxaa";

	// Super-sampling is simply rendering into a larger texture.
	// Downscaling is done by the final step when rendering to the screen.
	if (enable_ssaa) {
		scale *= antialiasing_scale;
	}

	// color_format can be a normalized integer format, but bloom requires
	// values outside of [0,1] so this needs to be a different one.
	const auto bloom_format = video::ECF_A16B16G16R16F;

	if (enable_bloom)
		buffer->setTexture(TEXTURE_BLOOM_MASK, scale, "bloom_mask", bloom_format);

	if (enable_msaa) {
		buffer->setTexture(TEXTURE_MSAA_COLOR, scale, "3d_render_msaa", color_format, false, antialiasing_scale);
		buffer->setTexture(TEXTURE_MSAA_DEPTH, scale, "3d_depthmap_msaa", depth_format, false, antialiasing_scale);

		if (enable_bloom)
			buffer->setTexture(TEXTURE_MSAA_BLOOM_MASK, scale, "bloom_mask_msaa", bloom_format, false, antialiasing_scale);
	}

	buffer->setTexture(TEXTURE_COLOR, scale, "3d_render", color_format);
	buffer->setTexture(TEXTURE_NORMAL, scale, "3d_normalmap", color_format);
	buffer->setTexture(TEXTURE_EXPOSURE_1, core::dimension2du(1,1), "exposure_1", color_format, /*clear:*/ true);
	buffer->setTexture(TEXTURE_EXPOSURE_2, core::dimension2du(1,1), "exposure_2", color_format, /*clear:*/ true);
	buffer->setTexture(TEXTURE_DEPTH, scale, "3d_depthmap", depth_format);

	std::vector<u8> outputs_draw3d = { TEXTURE_COLOR, TEXTURE_NORMAL };
	if (enable_bloom)
		outputs_draw3d.emplace_back(TEXTURE_BLOOM_MASK);

	// attach buffer to the previous step
	if (enable_msaa) {
		TextureBufferOutput *msaa = pipeline->createOwned<TextureBufferOutput>(buffer, std::vector<u8> { TEXTURE_MSAA_COLOR, TEXTURE_MSAA_BLOOM_MASK }, TEXTURE_MSAA_DEPTH);
		previousStep->setRenderTarget(msaa);
		TextureBufferOutput *normal = pipeline->createOwned<TextureBufferOutput>(buffer, outputs_draw3d, TEXTURE_DEPTH);
		pipeline->addResolveMSAAStep(msaa, normal);
	} else {
		previousStep->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, outputs_draw3d, TEXTURE_DEPTH));
	}

	// shared variables
	u32 shader_id;

	// Number of mipmap levels of the bloom downsampling texture
	const u8 MIPMAP_LEVELS = 4;

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
			u32 shader_id = client->getShaderSource()->getShader({"extract_bloom"});
			RenderStep *extract_bloom = pipeline->addPostprocessStep(
				"ExtractBloom", shader_id, std::vector<u8> { TEXTURE_BLOOM_MASK, TEXTURE_EXPOSURE_1 });
			extract_bloom->setRenderSource(buffer);
			extract_bloom->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, TEXTURE_BLOOM));
			source = TEXTURE_BLOOM;
		}

		if (enable_volumetric_light) {
			buffer->setTexture(TEXTURE_VOLUME, scale, "volume", bloom_format);

			shader_id = client->getShaderSource()->getShader({"volumetric_light"});
			auto volume = pipeline->addPostprocessStep("VolumetricLight", shader_id, std::vector<u8> { source, TEXTURE_DEPTH });
			volume->setRenderSource(buffer);
			volume->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, TEXTURE_VOLUME));
			source = TEXTURE_VOLUME;
		}

		// downsample
		shader_id = client->getShaderSource()->getShader({"bloom_downsample"});
		for (u8 i = 0; i < MIPMAP_LEVELS; i++) {
			auto step = pipeline->addPostprocessStep(
				"BloomDownsample" + std::to_string(i), shader_id, std::vector<u8> { source });
			step->setRenderSource(buffer);
			step->setBilinearFilter(0, true);
			step->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, TEXTURE_SCALE_DOWN + i));
			source = TEXTURE_SCALE_DOWN + i;
		}
	}

	// Bloom pt 2
	if (enable_bloom) {
		// upsample
		shader_id = client->getShaderSource()->getShader({"bloom_upsample"});
		for (u8 i = MIPMAP_LEVELS - 1; i > 0; i--) {
			auto step = pipeline->addPostprocessStep(
				"BloomUpsample" + std::to_string(i), shader_id, std::vector<u8> { u8(TEXTURE_SCALE_DOWN + i - 1), source });
			step->setRenderSource(buffer);
			step->setBilinearFilter(0, true);
			step->setBilinearFilter(1, true);
			step->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, u8(TEXTURE_SCALE_UP + i - 1)));
			source = TEXTURE_SCALE_UP + i - 1;
		}
	}

	// Dynamic Exposure pt2
	if (enable_auto_exposure) {
		shader_id = client->getShaderSource()->getShader({"update_exposure"});
		auto update_exposure = pipeline->addPostprocessStep(
			"UpdateExposure", shader_id, std::vector<u8> { TEXTURE_EXPOSURE_1, u8(TEXTURE_SCALE_DOWN + MIPMAP_LEVELS - 1) });
		update_exposure->setBilinearFilter(1, true);
		update_exposure->setRenderSource(buffer);
		update_exposure->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, TEXTURE_EXPOSURE_2));
	}

	// FXAA
	u8 final_stage_source = TEXTURE_COLOR;;

	if (enable_fxaa) {
		final_stage_source = TEXTURE_FXAA;

		buffer->setTexture(TEXTURE_FXAA, scale, "fxaa", color_format);
		shader_id = client->getShaderSource()->getShader({"fxaa"});
		PostProcessingStep *effect = pipeline->addPostprocessStep("FXAA", shader_id, std::vector<u8> { TEXTURE_COLOR });
		effect->setBilinearFilter(0, true);
		effect->setRenderSource(buffer);
		effect->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, TEXTURE_FXAA));
	}

	// final merge
	shader_id = client->getShaderSource()->getShader({"second_stage"});
	PostProcessingStep *effect = pipeline->addPostprocessStep("SecondStage", shader_id, std::vector<u8> { final_stage_source, TEXTURE_SCALE_UP, TEXTURE_EXPOSURE_2 });
	if (enable_ssaa)
		effect->setBilinearFilter(0, true);
	effect->setBilinearFilter(1, true);
	effect->setRenderSource(buffer);

	if (enable_auto_exposure) {
		pipeline->addSwapTexturesStep(buffer, TEXTURE_EXPOSURE_1, TEXTURE_EXPOSURE_2);
	}

	return effect;
}

void ResolveMSAAStep::run(PipelineContext &context)
{
	msaa_fbo->getIrrRenderTarget(context)->blitTo(target_fbo->getIrrRenderTarget(context));
}
