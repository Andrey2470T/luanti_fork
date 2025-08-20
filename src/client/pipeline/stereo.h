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
