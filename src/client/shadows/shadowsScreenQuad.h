// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 Liso <anlismon@gmail.com>

#pragma once
#include "client/media/shader.h"

class ShadowScreenQuad
{
public:
	ShadowScreenQuad();

	void render(video::VideoDriver *driver);
	video::SMaterial &getMaterial() { return Material; }

private:
	scene::Vertex3D Vertices[6];
	video::SMaterial Material;
};

class ShadowScreenQuadUniformSetter : public IShaderUniformSetter
{
public:
	void onSetUniforms(video::MaterialRenderer *renderer) override;
private:
	CachedShaderSetting<s32> m_sm_client_map_setting{"ShadowMapClientMap"};
	CachedShaderSetting<s32>
		m_sm_client_map_trans_setting{"ShadowMapClientMapTraslucent"};
	CachedShaderSetting<s32>
		m_sm_dynamic_sampler_setting{"ShadowMapSamplerdynamic"};
};

class ShadowScreenQuadUniformSetterFactory : public IShaderUniformSetterFactory
{
public:
	virtual IShaderUniformSetter *create() {
		return new ShadowScreenQuadUniformSetter();
	}
};
