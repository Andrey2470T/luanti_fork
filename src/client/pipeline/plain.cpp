// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "plain.h"
#include "secondstage.h"
#include "client/player/camera.h"
#include "client/core/client.h"
#include "client/render/clientmap.h"
#include "client/render/renderingengine.h"
#include "client/ui/hud.h"
#include "client/ui/minimap.h"
#include "client/shadows/dynamicshadowsrender.h"
#include <GUI/IGUIEnvironment.h>

/// Draw3D pipeline step
void Draw3D::run(PipelineContext &context)
{
	if (m_target)
		m_target->activate(context);

	context.device->getSceneManager()->drawAll();
	context.device->getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (!context.show_hud)
		return;
	context.hud->drawBlockBounds();
	context.hud->drawSelectionMesh();
}

void Draw3DCubeMap::run(PipelineContext &context)
{
	auto tb_output = dynamic_cast<TextureBufferOutput *>(getRenderTarget());

	if (!tb_output)
		return;
	
	auto camera = context.client->getCamera()->getCameraNode();
	v3f up = camera->getUpVector();
	v3f pos = camera->getAbsolutePosition();
	v3f target = camera->getTarget();
	v3f fwd = (target - pos).normalize();

	switch (curRenderedFace) {
	case video::ECMF_POS_X:
		fwd = fwd.crossProduct(up);
		break;
	case video::ECMF_NEG_X:
		fwd = up.crossProduct(fwd);
		break;
	case video::ECMF_POS_Y: {
		v3f up2 = up;
		up = -fwd;
		fwd = up2;
		break;
	}
	case video::ECMF_NEG_Y: {
		v3f up2 = up;
		up = fwd;
		fwd = -up2;
		break;
	}
	case video::ECMF_NEG_Z:
		fwd = -fwd;
		break;
	default:
		break;
	}

	f32 prevFarValue = camera->getFarValue();
	v3f prevTarget = camera->getTarget();
	v3f prevUpVector = camera->getUpVector();
	camera->setFarValue(2500.0f);
	camera->setTarget(pos+fwd);
	camera->setUpVector(up);
	camera->updateMatrices();

	tb_output->overrideTextureMap({{TEXTURE_COLOR_CUBE, curRenderedFace}});
	tb_output->overrideDepthMap({TEXTURE_DEPTH_CUBE, curRenderedFace});

	tb_output->activate(context);

	context.device->getSceneManager()->drawAll();
	context.device->getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (!context.show_hud)
		return;
	context.hud->drawBlockBounds();
	context.hud->drawSelectionMesh();

	camera->setFarValue(prevFarValue);
	camera->setTarget(prevTarget);
	camera->setUpVector(prevUpVector);
	camera->updateMatrices();

	auto driver = context.client->getRenderingEngine()->get_video_driver();
	driver->setTransform(video::ETS_PROJECTION, camera->getProjectionMatrix());
	driver->setTransform(video::ETS_VIEW, camera->getViewMatrix());

	u8 nextRenderedFace = curRenderedFace;

	/*if (nextRenderedFace == 3)
		nextRenderedFace = 5;
	else */if (nextRenderedFace == 5)
		nextRenderedFace = 0;
	else
		nextRenderedFace++;
	curRenderedFace = (video::E_CUBEMAP_FACE)nextRenderedFace;
}

void DrawWield::run(PipelineContext &context)
{
	if (m_target)
		m_target->activate(context);

	if (context.draw_wield_tool)
		context.client->getCamera()->drawWieldedTool();
}

void DrawHUD::run(PipelineContext &context)
{
	if (context.show_hud) {
		if (context.shadow_renderer)
			context.shadow_renderer->drawDebug();

		context.hud->resizeHotbar();

		if (context.draw_crosshair)
			context.hud->drawCrosshair();

		context.hud->drawLuaElements(context.client->getCamera()->getOffset());
		context.client->getCamera()->drawNametags();
	}
	context.device->getGUIEnvironment()->drawAll();
}


void MapPostFxStep::setRenderTarget(RenderTarget * _target)
{
	target = _target;
}

void MapPostFxStep::run(PipelineContext &context)
{
	if (target)
		target->activate(context);

	context.client->getEnv().getClientMap().renderPostFx(context.client->getCamera()->getCameraMode());
}

void RenderShadowMapStep::run(PipelineContext &context)
{
	// This is necessary to render shadows for animations correctly
	context.device->getSceneManager()->getRootSceneNode()->OnAnimate(os::Timer::getTime());
	context.shadow_renderer->update(context.client->getSky());
}

// class UpscaleStep

void UpscaleStep::run(PipelineContext &context)
{
	video::GLTexture *lowres = m_source->getTexture(0);
	m_target->activate(context);
	context.device->getVideoDriver()->draw2DImage(lowres,
			core::rect<s32>(0, 0, context.target_size.X, context.target_size.Y),
			core::rect<s32>(0, 0, lowres->getSize().Width, lowres->getSize().Height));
}

std::unique_ptr<RenderStep> create3DStage(Client *client, v2f scale)
{
	RenderStep *step = new Draw3D();
	if (g_settings->getBool("enable_post_processing")) {
		auto pipeline = new PostProcessingPipeline();
		pipeline->addDraw3DStep((Draw3D *)step);

		auto effect = addPostProcessing(pipeline, step, scale, client);
		effect->setRenderTarget(pipeline->getRenderTarget());
		step = pipeline;
	}
	return std::unique_ptr<RenderStep>(step);
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

	auto driver = client->getSceneManager()->getVideoDriver();
	video::ECOLOR_FORMAT color_format = TextureBuffer::selectColorFormat(driver);
	video::ECOLOR_FORMAT depth_format = TextureBuffer::selectDepthFormat(driver);

	// Initialize buffer
	TextureBuffer *buffer = pipeline->createTextureBuffer("Plain");
	buffer->setTexture(TEXTURE_LOWRES_COLOR, downscale_factor, "lowres_color", color_format);
	buffer->setTexture(TEXTURE_LOWRES_DEPTH, downscale_factor, "lowres_depth", depth_format);

	// Attach previous step to the buffer
	TextureBufferOutput *buffer_output = pipeline->createOwned<TextureBufferOutput>(
			buffer, std::vector<u8> {TEXTURE_LOWRES_COLOR}, TEXTURE_LOWRES_DEPTH);
	previousStep->setRenderTarget(buffer_output);

	// Add upscaling step
	RenderStep *upscale = pipeline->createOwned<UpscaleStep>();
	upscale->setRenderSource(buffer);
	pipeline->addStep("Upscaling", upscale);

	return upscale;
}

void populatePlainPipeline(RenderPipeline *pipeline, Client *client)
{
	auto downscale_factor = getDownscaleFactor();
	auto step3D = pipeline->own(create3DStage(client, downscale_factor));
	pipeline->addStep("Main", step3D);
	pipeline->addStep<DrawWield>("DrawWield");
	pipeline->addStep<MapPostFxStep>("MapPostFx");

	step3D = addUpscaling(pipeline, step3D, downscale_factor, client);

	step3D->setRenderTarget(pipeline->createOwned<ScreenTarget>());

	pipeline->addStep<DrawHUD>("DrawHUD");
}
