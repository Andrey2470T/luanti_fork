// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#pragma once
#include "pipeline.h"

/**
 * Offset camera for a specific eye in stereo rendering mode
 */
class OffsetCameraStep : public TrivialRenderStep
{
public:
	OffsetCameraStep(f32 eye_offset);
	OffsetCameraStep(bool right_eye);

	void run(PipelineContext &context) override;
	void reset(PipelineContext &context) override;
private:
    matrix4 base_transform;
    matrix4 move;
};

/**
 * Implements a pipeline step that renders the 3D scene
 */
class Draw3D : public RenderStep
{
public:
    virtual void setRenderSource(RenderSource *) override {}
    virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }

    virtual void reset(PipelineContext &context) override {}
    virtual void run(PipelineContext &context) override;

private:
    RenderTarget *m_target {nullptr};
};

/**
 * Implements a pipeline step that renders the game HUD
 */
class DrawHUD : public RenderStep
{
public:
    virtual void setRenderSource(RenderSource *) override {}
    virtual void setRenderTarget(RenderTarget *) override {}

    virtual void reset(PipelineContext &context) override {}
    virtual void run(PipelineContext &context) override;
};

/*class RenderShadowMapStep : public TrivialRenderStep
{
public:
    virtual void run(PipelineContext &context) override;
};*/
std::unique_ptr<RenderStep> create3DStage(Client *client, v2f scale);

img::PixelFormat selectColorFormat();
img::PixelFormat selectDepthFormat(u32 bits, bool stencil);
