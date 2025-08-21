// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "interlaced.h"
#include "postprocess.h"
#include "client/client.h"
#include "client/shader.h"
#include "client/camera.h"
#include "client/render/renderer.h"
#include "client/media/resource.h"

InitInterlacedMaskStep::InitInterlacedMaskStep(TextureBuffer *_buffer, u8 _index)	:
	buffer(_buffer), index(_index)
{
}

void InitInterlacedMaskStep::run(PipelineContext &context)
{
    auto mask = dynamic_cast<render::Texture2D *>(buffer->getTexture(index));
	if (!mask)
		return;
	if (mask == last_mask)
		return;
	last_mask = mask;

	auto size = mask->getSize();
    u8 *data = mask->downloadData().at(0)->getData();
    for (u32 j = 0; j < size.Y; j++) {
        u8 val = j % 2 ? 0xff : 0x00;
        memset(data, val, 4 * size.X);
        data += 4 * size.X;
	}
}

void populateInterlacedPipeline(RenderPipeline *pipeline, Client *client)
{
	// FIXME: "3d_mode = interlaced" is currently broken. Two options:
	// 1. Remove it
	// 2. Fix it
	// If you fix it, make sure to test it with "enable_post_processing = false".
	// You'll probably have to add a depth texture to make that combination work.
	// Also, this code should probably use selectColorFormat/selectDepthFormat.

	static const u8 TEXTURE_LEFT = 0;
	static const u8 TEXTURE_RIGHT = 1;
	static const u8 TEXTURE_MASK = 2;

	TextureBuffer *buffer = pipeline->createOwned<TextureBuffer>();
    buffer->setTexture(TEXTURE_LEFT, v2f(1.0f, 0.5f), "3d_render_left", img::PF_RGBA8);
    buffer->setTexture(TEXTURE_RIGHT, v2f(1.0f, 0.5f), "3d_render_right", img::PF_RGBA8);
    buffer->setTexture(TEXTURE_MASK, v2f(1.0f, 1.0f), "3d_render_mask", img::PF_RGBA8);

	pipeline->addStep<InitInterlacedMaskStep>(buffer, TEXTURE_MASK);

	auto step3D = pipeline->own(create3DStage(client, v2f(1.0f, 0.5f)));

	// eyes
	for (bool right : { false, true }) {
		pipeline->addStep<OffsetCameraStep>(right);
		auto output = pipeline->createOwned<TextureBufferOutput>(buffer, right ? TEXTURE_RIGHT : TEXTURE_LEFT);
		pipeline->addStep<SetRenderTargetStep>(step3D, output);
		pipeline->addStep(step3D);
        //pipeline->addStep<DrawWield>();
        //pipeline->addStep<MapPostFxStep>();
	}

	pipeline->addStep<OffsetCameraStep>(0.0f);

    auto cache = client->getResourceCache();
    auto shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "3d_interlaced_merge");
    shader->mapSamplers({"baseTexture", "normalTexture", "textureFlags"});
	auto texture_map = { TEXTURE_LEFT, TEXTURE_RIGHT, TEXTURE_MASK };

    auto merge = pipeline->addStep<PostProcessingStep>(shader, texture_map);
	merge->setRenderSource(buffer);
	merge->setRenderTarget(pipeline->createOwned<ScreenTarget>());
    pipeline->addStep<DrawHUD>();
}
