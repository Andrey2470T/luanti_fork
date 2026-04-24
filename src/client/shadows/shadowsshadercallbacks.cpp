// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 Liso <anlismon@gmail.com>

#include "client/shadows/shadowsshadercallbacks.h"
#include "client/render/renderingengine.h"

void ShadowUniformSetter::onSetUniforms(video::MaterialRenderer *renderer)
{
	auto *shadow = RenderingEngine::get_shadow_renderer();
	if (!shadow)
		return;

	const auto &light = shadow->getDirectionalLight();

	core::matrix4 shadowViewProj = light.getProjectionMatrix();
	shadowViewProj *= light.getViewMatrix();
	m_shadow_view_proj.set(shadowViewProj, renderer);

	m_light_direction.set(light.getDirection(), renderer);

	f32 TextureResolution = light.getMapResolution();
	m_texture_res.set(TextureResolution, renderer);

	f32 ShadowStrength = shadow->getShadowStrength();
	m_shadow_strength.set(ShadowStrength, renderer);

	video::SColor ShadowTint = shadow->getShadowTint();
	m_shadow_tint.set(ShadowTint, renderer);

	f32 timeOfDay = shadow->getTimeOfDay();
	m_time_of_day.set(timeOfDay, renderer);

	f32 shadowFar = shadow->getMaxShadowFar();
	m_shadowfar.set(shadowFar, renderer);

	f32 cam_pos[4];
	shadowViewProj.transformVect(cam_pos, light.getPlayerPos());
	m_camera_pos.set(cam_pos, renderer);

	s32 TextureLayerID = ShadowRenderer::TEXTURE_LAYER_SHADOW;
	m_shadow_texture.set(TextureLayerID, renderer);

	f32 bias0 = shadow->getPerspectiveBiasXY();
	m_perspective_bias0_vertex.set(bias0, renderer);
	m_perspective_bias0_pixel.set(bias0, renderer);
	f32 bias1 = 1.0f - bias0 + 1e-5f;
	m_perspective_bias1_vertex.set(bias1, renderer);
	m_perspective_bias1_pixel.set(bias1, renderer);
	f32 zbias = shadow->getPerspectiveBiasZ();
	m_perspective_zbias_vertex.set(zbias, renderer);
	m_perspective_zbias_pixel.set(zbias, renderer);
}

void ShadowDepthShaderCB::OnSetUniforms(
		video::MaterialRenderer *renderer, s32 userData)
{
	video::VideoDriver *driver = renderer->getVideoDriver();

	core::matrix4 lightMVP = driver->getTransform(video::ETS_PROJECTION);
	lightMVP *= driver->getTransform(video::ETS_VIEW);

	f32 cam_pos[4];
	lightMVP.transformVect(cam_pos, CameraPos);

	lightMVP *= driver->getTransform(video::ETS_WORLD);

	m_light_mvp_setting.set(lightMVP, renderer);
	m_map_resolution_setting.set(MapRes, renderer);
	m_max_far_setting.set(MaxFar, renderer);
	s32 TextureId = 0;
	m_color_map_sampler_setting.set(TextureId, renderer);
	f32 bias0 = PerspectiveBiasXY;
	m_perspective_bias0.set(bias0, renderer);
	f32 bias1 = 1.0f - bias0 + 1e-5f;
	m_perspective_bias1.set(bias1, renderer);
	f32 zbias = PerspectiveBiasZ;
	m_perspective_zbias.set(zbias, renderer);

	m_cam_pos_setting.set(cam_pos, renderer);
}
