// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 Liso <anlismon@gmail.com>

#pragma once
#include <Video/IShaderConstantSetCallBack.h>
#include "client/media/shader.h"

// Used by main game rendering

class ShadowUniformSetter : public IShaderUniformSetter
{
	CachedShaderSetting<f32, 16> m_shadow_view_proj{"m_ShadowViewProj"};
	CachedShaderSetting<f32, 3> m_light_direction{"v_LightDirection"};
	CachedShaderSetting<f32> m_texture_res{"f_textureresolution"};
	CachedShaderSetting<f32> m_shadow_strength{"f_shadow_strength"};
	CachedShaderSetting<f32, 3> m_shadow_tint{ "shadow_tint" };
	CachedShaderSetting<f32> m_time_of_day{"f_timeofday"};
	CachedShaderSetting<f32> m_shadowfar{"f_shadowfar"};
	CachedShaderSetting<f32, 4> m_camera_pos{"CameraPos"};
	CachedShaderSetting<s32> m_shadow_texture{"ShadowMapSampler"};
	CachedShaderSetting<f32>
		m_perspective_bias0_vertex{"xyPerspectiveBias0"};
	CachedShaderSetting<f32>
		m_perspective_bias0_pixel{"xyPerspectiveBias0"};
	CachedShaderSetting<f32>
		m_perspective_bias1_vertex{"xyPerspectiveBias1"};
	CachedShaderSetting<f32>
		m_perspective_bias1_pixel{"xyPerspectiveBias1"};
	CachedShaderSetting<f32>
		m_perspective_zbias_vertex{"zPerspectiveBias"};
	CachedShaderSetting<f32> m_perspective_zbias_pixel{"zPerspectiveBias"};

public:
	ShadowUniformSetter() = default;
	~ShadowUniformSetter() = default;

	virtual void onSetUniforms(video::MaterialRenderer *renderer) override;
};

class ShadowUniformSetterFactory : public IShaderUniformSetterFactory
{
public:
	virtual IShaderUniformSetter *create() {
		return new ShadowUniformSetter();
	}
};

// Used by depth shader

class ShadowDepthShaderCB : public video::IShaderConstantSetCallBack
{
public:
	void OnSetMaterial(const video::SMaterial &material) override {}

	void OnSetUniforms(video::MaterialRenderer *renderer) override;

	f32 MaxFar{2048.0f}, MapRes{1024.0f};
	f32 PerspectiveBiasXY {0.9f}, PerspectiveBiasZ {0.5f};
	v3f CameraPos;

private:
	CachedShaderSetting<f32, 16> m_light_mvp_setting{"LightMVP"};
	CachedShaderSetting<f32> m_map_resolution_setting{"MapResolution"};
	CachedShaderSetting<f32> m_max_far_setting{"MaxFar"};
	CachedShaderSetting<s32>
		m_color_map_sampler_setting{"ColorMapSampler"};
	CachedShaderSetting<f32> m_perspective_bias0{"xyPerspectiveBias0"};
	CachedShaderSetting<f32> m_perspective_bias1{"xyPerspectiveBias1"};
	CachedShaderSetting<f32> m_perspective_zbias{"zPerspectiveBias"};
	CachedShaderSetting<f32, 4> m_cam_pos_setting{"CameraPos"};
};
