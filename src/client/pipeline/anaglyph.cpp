// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "anaglyph.h"
#include <Render/DrawContext.h>
#include "client/core/client.h"
#include "client/render/rendersystem.h"
#include "client/render/renderer.h"


/// SetColorMaskStep step

SetColorMaskStep::SetColorMaskStep(int _color_mask)
	: color_mask(_color_mask)
{}

void SetColorMaskStep::run(PipelineContext &context)
{
    auto ctxt = context.client->getRenderSystem()->getRenderer()->getContext();
    ctxt->setColorMask(color_mask);
}

/// ClearDepthBufferTarget

ClearDepthBufferTarget::ClearDepthBufferTarget(RenderTarget *_target) :
	target(_target)
{}

void ClearDepthBufferTarget::activate(PipelineContext &context)
{
	target->activate(context);

    auto ctxt = context.client->getRenderSystem()->getRenderer()->getContext();
    ctxt->clearBuffers(render::CBF_DEPTH);
}

ConfigureOverrideMaterialTarget::ConfigureOverrideMaterialTarget(RenderTarget *_upstream, bool _enable) :
	upstream(_upstream), enable(_enable)
{
}

void ConfigureOverrideMaterialTarget::activate(PipelineContext &context)
{
	upstream->activate(context);
    //context.device->getVideoDriver()->getOverrideMaterial().Enabled = enable;
}


void populateAnaglyphPipeline(RenderPipeline *pipeline, Client *client)
{
	// clear depth buffer every time 3D is rendered
	auto step3D = pipeline->own(create3DStage(client, v2f(1.0)));
	auto screen = pipeline->createOwned<ScreenTarget>();
	auto clear_depth = pipeline->createOwned<ClearDepthBufferTarget>(screen);
	auto enable_override_material = pipeline->createOwned<ConfigureOverrideMaterialTarget>(clear_depth, true);
	step3D->setRenderTarget(enable_override_material);

	// left eye
	pipeline->addStep<OffsetCameraStep>(false);
    pipeline->addStep<SetColorMaskStep>(render::CP_RED);
	pipeline->addStep(step3D);

	// right eye
	pipeline->addStep<OffsetCameraStep>(true);
    pipeline->addStep<SetColorMaskStep>(render::CP_GREEN | render::CP_BLUE);
	pipeline->addStep(step3D);

	// reset
	pipeline->addStep<OffsetCameraStep>(0.0f);
    pipeline->addStep<SetColorMaskStep>(render::CP_ALL);

    //pipeline->addStep<DrawWield>();
	pipeline->addStep<DrawHUD>();
}
