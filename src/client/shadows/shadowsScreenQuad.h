// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 Liso <anlismon@gmail.com>

#pragma once
#include <Video/IShaderConstantSetCallBack.h>
#include "client/media/shader.h"

class shadowScreenQuad
{
public:
	shadowScreenQuad();

	void render(video::VideoDriver *driver);
	video::SMaterial &getMaterial() { return Material; }

private:
	scene::Vertex3D Vertices[6];
	video::SMaterial Material;
};

class shadowScreenQuadCB : public video::IShaderConstantSetCallBack
{
public:
	virtual void OnSetUniforms(video::MaterialRenderer *renderer,
			s32 userData);
private:
	CachedShaderSetting<s32> m_sm_client_map_setting{"ShadowMapClientMap"};
	CachedShaderSetting<s32>
		m_sm_client_map_trans_setting{"ShadowMapClientMapTraslucent"};
	CachedShaderSetting<s32>
		m_sm_dynamic_sampler_setting{"ShadowMapSamplerdynamic"};
};
