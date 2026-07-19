// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#pragma once
#include "stereo.h"
#include "pipeline.h"

/**
 *  Step to apply post-processing filter to the rendered image
 */
class PostProcessingStep : public RenderStep
{
public:
	/**
	 * Construct a new PostProcessingStep object
	 *
	 * @param shader_id ID of the shader in ShaderSource
	 * @param texture_map Map of textures to be chosen from the render source
	 */
	PostProcessingStep(u32 shader_id, const std::vector<u8> &texture_map, bool alpha_blend=false);

	virtual RenderSource *getRenderSource() override { return source; }
	virtual RenderTarget *getRenderTarget() override { return target; }
	void setRenderSource(RenderSource *source) override;
	void setRenderTarget(RenderTarget *target) override;
	void reset(PipelineContext &context) override;
	void run(PipelineContext &context) override;

	/**
	 * Configure bilinear filtering for a specific texture layer
	 *
	 * @param index Index of the texture layer
	 * @param value true to enable the bilinear filter, false to disable
	 */
	void setBilinearFilter(u8 index, bool value);
private:
	u32 shader_id;
	bool alpha_blend {false};
	std::vector<u8> texture_map;
	RenderSource *source { nullptr };
	RenderTarget *target { nullptr };
	video::SMaterial material;

	void configureMaterial();
};

struct PostProcessingStepState
{
	PostProcessingStep *step {nullptr};
	std::string name;
	bool enabled {true};
};

class ResolveMSAAStep;
class SwapTexturesStep;

class PostProcessingPipeline : public RenderPipeline
{
public:
	PostProcessingStep *addPostprocessStep(
		const std::string &name, u32 shader_id,
		const std::vector<u8> &texture_map, bool alpha_blend=false
	);

	void addDraw3DStep(Draw3D *step);
	ResolveMSAAStep *addResolveMSAAStep(TextureBufferOutput *msaa, TextureBufferOutput *normal);
	SwapTexturesStep *addSwapTexturesStep(TextureBuffer *buffer, u8 texture_a, u8 texture_b);

	PostProcessingStep *getPostprocessStep(const std::string &name);

	const std::vector<PostProcessingStepState> &getStepsState() const { return m_steps_state; }
	void setStepsState(const std::vector<PostProcessingStepState> &state) { m_steps_state = state; }

	void run(PipelineContext &context) override;
private:
	enum class SpecialSteps { DRAW3D, RESOLVE_MSAA, SWAP_TEXTURES };
	static std::vector<std::string> m_special_steps;
	Draw3D *m_draw3d {nullptr};
	ResolveMSAAStep *m_resolve_msaa {nullptr};
	SwapTexturesStep *m_swap_textures {nullptr};
	std::vector<PostProcessingStepState> m_steps_state;
};


class ResolveMSAAStep : public TrivialRenderStep
{
public:
	ResolveMSAAStep(TextureBufferOutput *_msaa_fbo, TextureBufferOutput *_target_fbo) :
			msaa_fbo(_msaa_fbo), target_fbo(_target_fbo) {};
	void run(PipelineContext &context) override;

private:
	TextureBufferOutput *msaa_fbo;
	TextureBufferOutput *target_fbo;
};


RenderStep *addPostProcessing(PostProcessingPipeline *pipeline, RenderStep *previousStep, v2f scale, Client *client);
