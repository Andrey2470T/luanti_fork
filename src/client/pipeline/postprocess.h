// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#pragma once
#include "base.h"
#include "pipeline.h"
#include "client/player/playercamera.h"

/**
 *  Step to apply post-processing filter to the rendered image
 */
class PostProcessingStep : public RenderStep
{
public:
	/**
	 * Construct a new PostProcessingStep object
	 *
	 * @param shader_id ID of the shader in IShaderSource
	 * @param texture_map Map of textures to be chosen from the render source
	 */
    PostProcessingStep(RenderSystem *_rnd_sys, RenderSource *_source, render::Shader *shader, const std::vector<u8> &texture_map);

	void setRenderSource(RenderSource *source) override;
	void setRenderTarget(RenderTarget *target) override;
	void reset(PipelineContext &context) override;
	void run(PipelineContext &context) override;

    ScreenQuad *getQuad() const
    {
        return quad.get();
    }
private:
    RenderSystem *rnd_sys;

	RenderSource *source { nullptr };
	RenderTarget *target { nullptr };

    std::unique_ptr<ScreenQuad> quad;
};

class ClientMap;
class PlayerCamera;

class MapPostFxStep : public TrivialRenderStep
{
public:
    MapPostFxStep(RenderSource *_source, u8 _texture_index)
        : source(_source), texture_index(_texture_index)
    {}
    virtual void setRenderTarget(RenderTarget *) override;
    virtual void run(PipelineContext &context) override;
private:
    RenderTarget *target;

    RenderSource *source;
    u8 texture_index;
    std::unique_ptr<ScreenQuad> quad;
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


RenderStep *addPostProcessing(RenderPipeline *pipeline, RenderStep *previousStep, v2f scale, Client *client);
