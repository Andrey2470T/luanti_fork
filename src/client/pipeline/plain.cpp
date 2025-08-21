// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "plain.h"
#include "postprocess.h"
#include "client/shadows/dynamicshadowsrender.h"
#include "base.h"
#include "settings.h"
#include "client/client.h"

/*void DrawWield::run(PipelineContext &context)
{
	if (m_target)
		m_target->activate(context);

	if (context.draw_wield_tool)
		context.client->getCamera()->drawWieldedTool();
}*/


/*void MapPostFxStep::setRenderTarget(RenderTarget * _target)
{
	target = _target;
}

void MapPostFxStep::run(PipelineContext &context)
{
	if (target)
		target->activate(context);

	context.client->getEnv().getClientMap().renderPostFx(context.client->getCamera()->getCameraMode());
}*/

// class UpscaleStep

void UpscaleStep::run(PipelineContext &context)
{
    auto rnd_sys = context.client->getRenderSystem();
    if (!lowres_image) {
        lowres_image = std::make_unique<ScreenQuad>(rnd_sys, m_source);
         lowres_image->setTextureMap({0});
    }
    auto lowres = m_source->getTexture(0);
    lowres_image->updateQuad(context.target_size, lowres->getSize());

    m_target->activate(context);
    lowres_image->render();
}

static v2f getDownscaleFactor()
{
	u16 undersampling = MYMAX(g_settings->getU16("undersampling"), 1);
	return v2f(1.0f / undersampling);
}

RenderStep* addUpscaling(RenderPipeline *pipeline, RenderStep *previousStep, v2f downscale_factor, Client *client)
{
	const int TEXTURE_LOWRES_COLOR = 0;
	const int TEXTURE_LOWRES_DEPTH = 1;

	if (downscale_factor.X == 1.0f && downscale_factor.Y == 1.0f)
		return previousStep;

	// post-processing pipeline takes care of rescaling
	if (g_settings->getBool("enable_post_processing"))
		return previousStep;

    auto color_format = selectColorFormat();
    auto depth_format = selectDepthFormat(32, false);

	// Initialize buffer
	TextureBuffer *buffer = pipeline->createOwned<TextureBuffer>();
	buffer->setTexture(TEXTURE_LOWRES_COLOR, downscale_factor, "lowres_color", color_format);
	buffer->setTexture(TEXTURE_LOWRES_DEPTH, downscale_factor, "lowres_depth", depth_format);

	// Attach previous step to the buffer
	TextureBufferOutput *buffer_output = pipeline->createOwned<TextureBufferOutput>(
			buffer, std::vector<u8> {TEXTURE_LOWRES_COLOR}, TEXTURE_LOWRES_DEPTH);
	previousStep->setRenderTarget(buffer_output);

	// Add upscaling step
	RenderStep *upscale = pipeline->createOwned<UpscaleStep>();
	upscale->setRenderSource(buffer);
	pipeline->addStep(upscale);

	return upscale;
}

void populatePlainPipeline(RenderPipeline *pipeline, Client *client)
{
	auto downscale_factor = getDownscaleFactor();
	auto step3D = pipeline->own(create3DStage(client, downscale_factor));
	pipeline->addStep(step3D);
    //pipeline->addStep<DrawWield>();

	step3D = addUpscaling(pipeline, step3D, downscale_factor, client);

	step3D->setRenderTarget(pipeline->createOwned<ScreenTarget>());

	pipeline->addStep<DrawHUD>();
}
