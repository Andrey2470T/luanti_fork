// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#pragma once
#include "core.h"
#include "pipeline.h"

/*class DrawWield : public RenderStep
{
public:
	virtual void setRenderSource(RenderSource *) override {}
	virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }

	virtual void reset(PipelineContext &context) override {}
	virtual void run(PipelineContext &context) override;

private:
	RenderTarget *m_target {nullptr};
};*/

/*class MapPostFxStep : public TrivialRenderStep
{
public:
	virtual void setRenderTarget(RenderTarget *) override;
	virtual void run(PipelineContext &context) override;
private:
	RenderTarget *target;
};

/**
 * UpscaleStep step performs rescaling of the image
 * in the source texture 0 to the size of the target.
 */
class ScreenQuad;

class UpscaleStep : public RenderStep
{
public:

	virtual void setRenderSource(RenderSource *source) override { m_source = source; }
	virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }
	virtual void reset(PipelineContext &context) override {};
	virtual void run(PipelineContext &context) override;
private:
	RenderSource *m_source;
	RenderTarget *m_target;

    std::unique_ptr<ScreenQuad> lowres_image;
};

RenderStep* addUpscaling(RenderPipeline *pipeline, RenderStep *previousStep, v2f downscale_factor, Client *client);

void populatePlainPipeline(RenderPipeline *pipeline, Client *client);
