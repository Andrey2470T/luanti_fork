// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 Liso <anlismon@gmail.com>

#pragma once
#include <IShaderConstantSetCallBack.h>
#include "client/shader.h"

class shadowScreenQuad
{
public:
	shadowScreenQuad();

	void render(video::VideoDriver *driver);
	video::SMaterial &getMaterial() { return Material; }

private:
	video::S3DVertex Vertices[6];
	video::SMaterial Material;
};

class shadowScreenQuadCB : public video::IShaderConstantSetCallBack
{
public:
	virtual void OnSetConstants(video::IMaterialRenderer *renderer,
			s32 userData);
private:
	CachedPixelShaderSetting<s32> m_sm_client_map_setting{"ShadowMapClientMap"};
	CachedPixelShaderSetting<s32>
		m_sm_client_map_trans_setting{"ShadowMapClientMapTraslucent"};
	CachedPixelShaderSetting<s32>
		m_sm_dynamic_sampler_setting{"ShadowMapSamplerdynamic"};
};
