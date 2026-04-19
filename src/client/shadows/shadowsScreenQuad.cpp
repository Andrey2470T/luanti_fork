// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 Liso <anlismon@gmail.com>

#include "shadowsScreenQuad.h"
#include <Video/VideoDriver.h>

shadowScreenQuad::shadowScreenQuad()
{
	Material.Wireframe = false;

	video::SColor color(0x0);
	Vertices[0] = {{-1.0f, -1.0f, 0.0f}, {0, 0, 1}, color, {0.0f, 1.0f}};
	Vertices[1] = {{-1.0f, 1.0f, 0.0f}, {0, 0, 1}, color, {0.0f, 0.0f}};
	Vertices[2] = {{1.0f, 1.0f, 0.0f}, {0, 0, 1}, color, {1.0f, 0.0f}};
	Vertices[3] = {{1.0f, -1.0f, 0.0f}, {0, 0, 1}, color, {1.0f, 1.0f}};
	Vertices[4] = {{-1.0f, -1.0f, 0.0f}, {0, 0, 1}, color, {0.0f, 1.0f}};
	Vertices[5] = {{1.0f, 1.0f, 0.0f}, {0, 0, 1}, color, {1.0f, 0.0f}};
}

void shadowScreenQuad::render(video::VideoDriver *driver)
{
	u16 indices[6] = {0, 1, 2, 3, 4, 5};
	driver->setMaterial(Material);
	driver->setTransform(video::ETS_WORLD, core::matrix4());
	driver->drawIndexedTriangleList(&Vertices[0], 6, &indices[0], 2);
}

void shadowScreenQuadCB::OnSetConstants(video::MaterialRenderer *renderer, s32 userData)
{
	s32 TextureId = 0;
	m_sm_client_map_setting.set(TextureId, renderer);

	TextureId = 1;
	m_sm_client_map_trans_setting.set(TextureId, renderer);

	TextureId = 2;
	m_sm_dynamic_sampler_setting.set(TextureId, renderer);
}
