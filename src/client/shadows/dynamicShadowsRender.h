// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 Liso <anlismon@gmail.com>

#pragma once

#include "client/shadows/dynamicshadows.h"
#include <Render/Texture2D.h>
#include <Render/Shader.h>
#include <Render/UniformBuffer.h>

enum E_SHADOW_MODE : u8
{
	ESM_RECEIVE = 0,
	ESM_BOTH,
};

/*struct NodeToApply
{
	NodeToApply(scene::ISceneNode *n,
			E_SHADOW_MODE m = E_SHADOW_MODE::ESM_BOTH) :
			node(n),
			shadowMode(m){};
	bool operator<(const NodeToApply &other) const { return node < other.node; };

	scene::ISceneNode *node;

	E_SHADOW_MODE shadowMode{E_SHADOW_MODE::ESM_BOTH};
	bool dirty{false};
};*/

class ShadowRenderer
{
public:
	static const int TEXTURE_LAYER_SHADOW = 3;

    ShadowRenderer(Client *client);
	~ShadowRenderer();

	void initialize();

	/// Adds a directional light shadow map (Usually just one (the sun) except in
	/// Tattoine ).
	size_t addDirectionalLight();
	DirectionalLight &getDirectionalLight(u32 index = 0);
	size_t getDirectionalLightCount() const;
	f32 getMaxShadowFar() const;

	/// Adds a shadow to the scene node.
	/// The shadow mode can be ESM_BOTH, or ESM_RECEIVE.
	/// ESM_BOTH casts and receives shadows
	/// ESM_RECEIVE only receives but does not cast shadows.
	///
    /*void addNodeToShadowList(scene::ISceneNode *node,
			E_SHADOW_MODE shadowMode = ESM_BOTH);
    void removeNodeFromShadowList(scene::ISceneNode *node);*/

    void update(render::Texture2D *outputTarget = nullptr);
	void setForceUpdateShadowMap() { m_force_update_shadow_map = true; }
    //void drawDebug();

    render::Texture2D *get_texture()
	{
        return shadowMapTextureFinal.get();
	}


	bool is_active() const { return m_shadows_enabled && shadowMapTextureFinal != nullptr; }
    void setTimeOfDay(f32 isDay) { m_time_day = isDay; };
    void setShadowIntensity(f32 shadow_intensity);
    void setShadowTint(img::color8 shadow_tint) { m_shadow_tint = shadow_tint; }

	s32 getShadowSamples() const { return m_shadow_samples; }
    f32 getShadowStrength() const { return m_shadows_enabled ? m_shadow_strength : 0.0f; }
    img::color8 getShadowTint() const { return m_shadow_tint; }
    f32 getTimeOfDay() const { return m_time_day; }

	f32 getPerspectiveBiasXY() { return m_perspective_bias_xy; }
	f32 getPerspectiveBiasZ() { return m_perspective_bias_z; }

private:
    /*render::Texture2D *getSMTexture(const std::string &shadow_map_name,
			video::ECOLOR_FORMAT texture_format,
			bool force_creation = false);

    void renderShadowMap(render::Texture2D *target, DirectionalLight &light,
			scene::E_SCENE_NODE_RENDER_PASS pass =
                    scene::ESNRP_SOLID);*/
    void renderShadowObjects(render::Texture2D *target, DirectionalLight &light);
	void updateSMTextures();

	void disable();
	void enable() { m_shadows_enabled = m_shadows_supported; }

	// a bunch of variables
	Client *m_client{nullptr};
    std::unique_ptr<render::Texture2D> shadowMapClientMap;
    std::unique_ptr<render::Texture2D> shadowMapClientMapFuture;
    std::unique_ptr<render::Texture2D> shadowMapTextureFinal;
    std::unique_ptr<render::Texture2D> shadowMapTextureDynamicObjects;
    std::unique_ptr<render::Texture2D> shadowMapTextureColors;

	std::vector<DirectionalLight> m_light_list;
    //std::vector<NodeToApply> m_shadow_node_array;

    f32 m_shadow_strength;
    img::color8 m_shadow_tint{img::black};
    f32 m_shadow_strength_gamma;
    f32 m_shadow_map_max_distance;
    f32 m_shadow_map_texture_size;
    f32 m_time_day{0.0f};
	int m_shadow_samples;
	bool m_shadow_map_texture_32bit;
    bool m_shadows_enabled{true};
    bool m_shadows_supported{true}; // assume shadows supported. We will check actual support in initialize
	bool m_shadow_map_colored;
    bool m_force_update_shadow_map{false};
	u8 m_map_shadow_update_frames; /* Use this number of frames to update map shaodw */
    u8 m_current_frame{0}; /* Current frame */
    f32 m_perspective_bias_xy{0.8f};
    f32 m_perspective_bias_z{0.5f};

    img::PixelFormat m_texture_format{img::PF_R16F};
    img::PixelFormat m_texture_format_color{img::PF_RG16};

	// Shadow Shader stuff

	void createShaders();
    void createShadowBuffer();

    render::Shader *depth_shader{nullptr};
    render::Shader *depth_shader_entities{nullptr};
    render::Shader *depth_shader_trans{nullptr};
    render::Shader *mixcsm_shader{nullptr};

    std::unique_ptr<render::UniformBuffer> m_shadow_buffer;

    //shadowScreenQuad *m_screen_quad{nullptr};
    //shadowScreenQuadCB *m_shadow_mix_cb{nullptr};
};

/**
 * @brief Create a shadow renderer if settings allow this.
 *
 * @param device Device to be used to render shadows.
 * @param client Reference to the client context.
 * @return A new ShadowRenderer instance or nullptr if shadows are disabled or not supported.
 */
ShadowRenderer *createShadowRenderer(Client *client);

/*
class ShadowConstantSetter : public IShaderConstantSetter
{
    CachedPixelShaderSetting<f32, 16> m_shadow_view_proj{"m_ShadowViewProj"};
    CachedPixelShaderSetting<f32, 3> m_light_direction{"v_LightDirection"};
    CachedPixelShaderSetting<f32> m_texture_res{"f_textureresolution"};
    CachedPixelShaderSetting<f32> m_shadow_strength{"f_shadow_strength"};
    CachedPixelShaderSetting<f32, 3> m_shadow_tint{ "shadow_tint" };
    CachedPixelShaderSetting<f32> m_time_of_day{"f_timeofday"};
    CachedPixelShaderSetting<f32> m_shadowfar{"f_shadowfar"};
    CachedPixelShaderSetting<f32, 4> m_camera_pos{"CameraPos"};
    CachedPixelShaderSetting<s32> m_shadow_texture{"ShadowMapSampler"};
    CachedVertexShaderSetting<f32>
        m_perspective_bias0_vertex{"xyPerspectiveBias0"};
    CachedPixelShaderSetting<f32>
        m_perspective_bias0_pixel{"xyPerspectiveBias0"};
    CachedVertexShaderSetting<f32>
        m_perspective_bias1_vertex{"xyPerspectiveBias1"};
    CachedPixelShaderSetting<f32>
        m_perspective_bias1_pixel{"xyPerspectiveBias1"};
    CachedVertexShaderSetting<f32>
        m_perspective_zbias_vertex{"zPerspectiveBias"};
    CachedPixelShaderSetting<f32> m_perspective_zbias_pixel{"zPerspectiveBias"};

public:
    ShadowConstantSetter() = default;
    ~ShadowConstantSetter() = default;

    virtual void onSetConstants(video::IMaterialRendererServices *services) override;
};

class ShadowConstantSetterFactory : public IShaderConstantSetterFactory
{
public:
    virtual IShaderConstantSetter *create() {
        return new ShadowConstantSetter();
    }
};

// Used by depth shader

class ShadowDepthShaderCB : public video::IShaderConstantSetCallBack
{
public:
    void OnSetMaterial(const video::SMaterial &material) override {}

    void OnSetConstants(video::IMaterialRendererServices *services,
            s32 userData) override;

    f32 MaxFar{2048.0f}, MapRes{1024.0f};
    f32 PerspectiveBiasXY {0.9f}, PerspectiveBiasZ {0.5f};
    v3f CameraPos;

private:
    CachedVertexShaderSetting<f32, 16> m_light_mvp_setting{"LightMVP"};
    CachedVertexShaderSetting<f32> m_map_resolution_setting{"MapResolution"};
    CachedVertexShaderSetting<f32> m_max_far_setting{"MaxFar"};
    CachedPixelShaderSetting<s32>
        m_color_map_sampler_setting{"ColorMapSampler"};
    CachedVertexShaderSetting<f32> m_perspective_bias0{"xyPerspectiveBias0"};
    CachedVertexShaderSetting<f32> m_perspective_bias1{"xyPerspectiveBias1"};
    CachedVertexShaderSetting<f32> m_perspective_zbias{"zPerspectiveBias"};
    CachedVertexShaderSetting<f32, 4> m_cam_pos_setting{"CameraPos"};
};

class shadowScreenQuadCB : public video::IShaderConstantSetCallBack
{
public:
    virtual void OnSetConstants(video::IMaterialRendererServices *services,
            s32 userData);
private:
    CachedPixelShaderSetting<s32> m_sm_client_map_setting{"ShadowMapClientMap"};
    CachedPixelShaderSetting<s32>
        m_sm_client_map_trans_setting{"ShadowMapClientMapTraslucent"};
    CachedPixelShaderSetting<s32>
        m_sm_dynamic_sampler_setting{"ShadowMapSamplerdynamic"};
};
*/
