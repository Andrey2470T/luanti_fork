// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 Liso <anlismon@gmail.com>

#include "client/shadows/dynamicShadowsRender.h"
#include "gui/touchcontrols.h"
#include "settings.h"
#include "filesys.h"
#include "util/string.h"
#include "client/client.h"
#include "client/map/clientmap.h"
#include "profiler.h"
#include "client/render/rendersystem.h"
#include "client/render/renderer.h"
#include "client/media/resource.h"

// This binding point is reserved and can't be used for other UBOs
#define SHADOW_UBO_BINDING_POINT 2

ShadowRenderer::ShadowRenderer(Client *client) :
        m_client(client)
{
	m_shadow_strength_gamma = g_settings->getFloat("shadow_strength_gamma");
	if (std::isnan(m_shadow_strength_gamma))
		m_shadow_strength_gamma = 1.0f;
    m_shadow_strength_gamma = std::clamp(m_shadow_strength_gamma, 0.1f, 10.0f);

	m_shadow_map_max_distance = g_settings->getFloat("shadow_map_max_distance");

	m_shadow_map_texture_size = g_settings->getFloat("shadow_map_texture_size");

	m_shadow_map_texture_32bit = g_settings->getBool("shadow_map_texture_32bit");
	m_shadow_map_colored = g_settings->getBool("shadow_map_color");
	m_shadow_samples = g_settings->getS32("shadow_filters");
	m_map_shadow_update_frames = g_settings->getS16("shadow_update_frames");

	// add at least one light
	addDirectionalLight();
}

ShadowRenderer::~ShadowRenderer()
{
	// call to disable releases dynamically allocated resources
	disable();

	m_shadow_node_array.clear();
	m_light_list.clear();
}

void ShadowRenderer::disable()
{
	m_shadows_enabled = false;

    auto ctxt = m_client->getRenderSystem()->getRenderer()->getContext();
    ctxt->setFrameBuffer(nullptr);

	for (auto node : m_shadow_node_array)
		node.node->forEachMaterial([] (auto &mat) {
			mat.setTexture(TEXTURE_LAYER_SHADOW, nullptr);
		});
}

void ShadowRenderer::initialize()
{
	createShaders();

    m_texture_format = m_shadow_map_texture_32bit ? img::PF_R32F : img::PF_R16F;
    m_texture_format_color = m_shadow_map_texture_32bit ? img::PF_RG32F : img::PF_RG16F;

    shadowMapTextureDynamicObjects = std::make_unique<render::Texture2D>(
        std::string("shadow_dynamic_") + itos(m_shadow_map_texture_size),
            m_shadow_map_texture_size, m_shadow_map_texture_size, m_texture_format, 0);

    if (!shadowMapClientMapFuture && m_map_shadow_update_frames > 1) {
        shadowMapClientMapFuture = std::make_unique<render::Texture2D>(
            std::string("shadow_clientmap_bb_") + itos(m_shadow_map_texture_size),
            m_shadow_map_texture_size, m_shadow_map_texture_size, m_shadow_map_colored ? m_texture_format_color : m_texture_format, 0);
    }

    if (m_shadow_map_colored) {
        shadowMapTextureColors = std::make_unique<render::Texture2D>(
            std::string("shadow_colored_") + itos(m_shadow_map_texture_size),
            m_shadow_map_texture_size, m_shadow_map_texture_size, m_shadow_map_colored ? m_texture_format_color : m_texture_format, 0);
    }

    img::PixelFormat final_format;

    if (m_shadow_map_texture_32bit) {
        if (m_shadow_map_colored)
            final_format = img::PF_RGBA32F;
        else
            final_format = img::PF_R32F;
    } else {
        if (m_shadow_map_colored)
            final_format = img::PF_RGBA16F;
        else
            final_format = img::PF_R16F;
    }
    shadowMapTextureFinal = std::make_unique<render::Texture2D>(
        std::string("shadowmap_final_") + itos(m_shadow_map_texture_size),
        m_shadow_map_texture_size, m_shadow_map_texture_size, final_format, 0);

	m_shadows_enabled &= m_shadows_supported;
}


size_t ShadowRenderer::addDirectionalLight()
{
	m_light_list.emplace_back(m_shadow_map_texture_size,
			v3f(0.f, 0.f, 0.f),
            img::white, m_shadow_map_max_distance);
	return m_light_list.size() - 1;
}

DirectionalLight &ShadowRenderer::getDirectionalLight(u32 index)
{
	return m_light_list[index];
}

size_t ShadowRenderer::getDirectionalLightCount() const
{
	return m_light_list.size();
}

f32 ShadowRenderer::getMaxShadowFar() const
{
	float zMax = m_light_list[0].getFarValue();
	return zMax;
}

void ShadowRenderer::setShadowIntensity(float shadow_intensity)
{
	m_shadow_strength = std::pow(shadow_intensity, 1.0f / m_shadow_strength_gamma);
	if (m_shadow_strength > 1e-2f)
		enable();
	else
		disable();
}

void ShadowRenderer::addNodeToShadowList(
		scene::ISceneNode *node, E_SHADOW_MODE shadowMode)
{
	m_shadow_node_array.emplace_back(node, shadowMode);
	// node should never be ClientMap
	assert(!node->getName().has_value() || *node->getName() != "ClientMap");
	node->forEachMaterial([this] (auto &mat) {
		mat.setTexture(TEXTURE_LAYER_SHADOW, shadowMapTextureFinal);
	});
}

void ShadowRenderer::removeNodeFromShadowList(scene::ISceneNode *node)
{
	node->forEachMaterial([] (auto &mat) {
		mat.setTexture(TEXTURE_LAYER_SHADOW, nullptr);
	});
	for (auto it = m_shadow_node_array.begin(); it != m_shadow_node_array.end();) {
		if (it->node == node) {
			it = m_shadow_node_array.erase(it);
			break;
		} else {
			++it;
		}
	}
}

void ShadowRenderer::updateSMTextures()
{
	if (!m_shadows_enabled || m_smgr->getActiveCamera() == nullptr) {
		return;
	}

    shadowMapTextureDynamicObjects =
	if (!shadowMapTextureDynamicObjects) {

		shadowMapTextureDynamicObjects = getSMTexture(
			std::string("shadow_dynamic_") + itos(m_shadow_map_texture_size),
			m_texture_format, true);
		assert(shadowMapTextureDynamicObjects != nullptr);
	}

	if (!shadowMapClientMap) {

		shadowMapClientMap = getSMTexture(
			std::string("shadow_clientmap_") + itos(m_shadow_map_texture_size),
			m_shadow_map_colored ? m_texture_format_color : m_texture_format,
			true);
		assert(shadowMapClientMap != nullptr);
	}

	if (!shadowMapClientMapFuture && m_map_shadow_update_frames > 1) {
		shadowMapClientMapFuture = getSMTexture(
			std::string("shadow_clientmap_bb_") + itos(m_shadow_map_texture_size),
			m_shadow_map_colored ? m_texture_format_color : m_texture_format,
			true);
		assert(shadowMapClientMapFuture != nullptr);
	}

	if (m_shadow_map_colored && !shadowMapTextureColors) {
		shadowMapTextureColors = getSMTexture(
			std::string("shadow_colored_") + itos(m_shadow_map_texture_size),
			m_shadow_map_colored ? m_texture_format_color : m_texture_format,
			true);
		assert(shadowMapTextureColors != nullptr);
	}

	// The merge all shadowmaps texture
	if (!shadowMapTextureFinal) {
		video::ECOLOR_FORMAT frt;
		if (m_shadow_map_texture_32bit) {
			if (m_shadow_map_colored)
				frt = video::ECOLOR_FORMAT::ECF_A32B32G32R32F;
			else
				frt = video::ECOLOR_FORMAT::ECF_R32F;
		} else {
			if (m_shadow_map_colored)
				frt = video::ECOLOR_FORMAT::ECF_A16B16G16R16F;
			else
				frt = video::ECOLOR_FORMAT::ECF_R16F;
		}
		shadowMapTextureFinal = getSMTexture(
			std::string("shadowmap_final_") + itos(m_shadow_map_texture_size),
			frt, true);
		assert(shadowMapTextureFinal != nullptr);

		for (auto &node : m_shadow_node_array)
			node.node->forEachMaterial([this] (auto &mat) {
				mat.setTexture(TEXTURE_LAYER_SHADOW, shadowMapTextureFinal);
			});
	}

	if (!m_shadow_node_array.empty()) {
		bool reset_sm_texture = false;

		// detect if SM should be regenerated
		for (DirectionalLight &light : m_light_list) {
			if (light.should_update_map_shadow || m_force_update_shadow_map) {
				light.should_update_map_shadow = false;
				m_current_frame = 0;
				reset_sm_texture = true;
			}
		}

		video::ITexture* shadowMapTargetTexture = shadowMapClientMapFuture;
		if (shadowMapTargetTexture == nullptr)
			shadowMapTargetTexture = shadowMapClientMap;

		// Update SM incrementally:
		for (DirectionalLight &light : m_light_list) {
			// Static shader values.
			for (auto cb : {m_shadow_depth_cb, m_shadow_depth_entity_cb, m_shadow_depth_trans_cb})
				if (cb) {
					cb->MapRes = (f32)m_shadow_map_texture_size;
					cb->MaxFar = (f32)m_shadow_map_max_distance * BS;
					cb->PerspectiveBiasXY = getPerspectiveBiasXY();
					cb->PerspectiveBiasZ = getPerspectiveBiasZ();
					cb->CameraPos = light.getFuturePlayerPos();
				}

			// set the Render Target
			// right now we can only render in usual RTT, not
			// Depth texture is available in irrlicth maybe we
			// should put some gl* fn here


			if (m_current_frame < m_map_shadow_update_frames || m_force_update_shadow_map) {
				m_driver->setRenderTarget(shadowMapTargetTexture, reset_sm_texture, true,
                        img::color8(255, 255, 255, 255));
				renderShadowMap(shadowMapTargetTexture, light);

				// Render transparent part in one pass.
				// This is also handled in ClientMap.
				if (m_current_frame == m_map_shadow_update_frames - 1 || m_force_update_shadow_map) {
					if (m_shadow_map_colored) {
						m_driver->setRenderTarget(0, false, false);
						m_driver->setRenderTarget(shadowMapTextureColors,
                                true, false, img::color8(255, 255, 255, 255));
					}
					renderShadowMap(shadowMapTextureColors, light,
							scene::ESNRP_TRANSPARENT);
				}
				m_driver->setRenderTarget(0, false, false);
			}

			reset_sm_texture = false;
		} // end for lights

		// move to the next section
		if (m_current_frame <= m_map_shadow_update_frames)
			++m_current_frame;

		// pass finished, swap textures and commit light changes
		if (m_current_frame == m_map_shadow_update_frames || m_force_update_shadow_map) {
			if (shadowMapClientMapFuture != nullptr)
				std::swap(shadowMapClientMapFuture, shadowMapClientMap);

			// Let all lights know that maps are updated
			for (DirectionalLight &light : m_light_list)
				light.commitFrustum();
		}
		m_force_update_shadow_map = false;
	}
}

void ShadowRenderer::update(video::ITexture *outputTarget)
{
	if (!m_shadows_enabled || m_smgr->getActiveCamera() == nullptr) {
		return;
	}

	updateSMTextures();

	if (shadowMapTextureFinal == nullptr) {
		return;
	}


	if (!m_shadow_node_array.empty()) {

		for (DirectionalLight &light : m_light_list) {
			// Static shader values for entities are set in updateSMTextures
			// SM texture for entities is not updated incrementally and
			// must by updated using current player position.
			m_shadow_depth_entity_cb->CameraPos = light.getPlayerPos();

			// render shadows for the n0n-map objects.
			m_driver->setRenderTarget(shadowMapTextureDynamicObjects, true,
                    true, img::color8(255, 255, 255, 255));
			renderShadowObjects(shadowMapTextureDynamicObjects, light);
			// clear the Render Target
			m_driver->setRenderTarget(0, false, false);

			// in order to avoid too many map shadow renders,
			// we should make a second pass to mix clientmap shadows and
			// entities shadows :(
			m_screen_quad->getMaterial().setTexture(0, shadowMapClientMap);
			// dynamic objs shadow texture.
			if (m_shadow_map_colored)
				m_screen_quad->getMaterial().setTexture(1, shadowMapTextureColors);
			m_screen_quad->getMaterial().setTexture(2, shadowMapTextureDynamicObjects);

			m_driver->setRenderTarget(shadowMapTextureFinal, false, false,
                    img::color8(255, 255, 255, 255));
			m_screen_quad->render(m_driver);
			m_driver->setRenderTarget(0, false, false);

		} // end for lights
	}
}

//void ShadowRenderer::drawDebug()
//{
	/* this code just shows shadows textures in screen and in ONLY for debugging*/
    /*#if 0
	// this is debug, ignore for now.
	if (shadowMapTextureFinal)
		m_driver->draw2DImage(shadowMapTextureFinal,
				core::rect<s32>(0, 50, 128, 128 + 50),
				core::rect<s32>({0, 0}, shadowMapTextureFinal->getSize()));

	if (shadowMapClientMap)
		m_driver->draw2DImage(shadowMapClientMap,
				core::rect<s32>(0, 50 + 128, 128, 128 + 50 + 128),
				core::rect<s32>({0, 0}, shadowMapTextureFinal->getSize()));

	if (shadowMapTextureDynamicObjects)
		m_driver->draw2DImage(shadowMapTextureDynamicObjects,
				core::rect<s32>(0, 128 + 50 + 128, 128,
						128 + 50 + 128 + 128),
				core::rect<s32>({0, 0}, shadowMapTextureDynamicObjects->getSize()));

	if (m_shadow_map_colored && shadowMapTextureColors) {

		m_driver->draw2DImage(shadowMapTextureColors,
				core::rect<s32>(128,128 + 50 + 128 + 128,
						128 + 128, 128 + 50 + 128 + 128 + 128),
				core::rect<s32>({0, 0}, shadowMapTextureColors->getSize()));
	}
	#endif
}*/


video::ITexture *ShadowRenderer::getSMTexture(const std::string &shadow_map_name,
		video::ECOLOR_FORMAT texture_format, bool force_creation)
{
	if (force_creation) {
		return m_driver->addRenderTargetTexture(
				core::dimension2du(m_shadow_map_texture_size,
						m_shadow_map_texture_size),
				shadow_map_name.c_str(), texture_format);
	}

	return m_driver->getTexture(shadow_map_name.c_str());
}

void ShadowRenderer::renderShadowMap(video::ITexture *target,
		DirectionalLight &light, scene::E_SCENE_NODE_RENDER_PASS pass)
{
	m_driver->setTransform(video::ETS_VIEW, light.getFutureViewMatrix());
	m_driver->setTransform(video::ETS_PROJECTION, light.getFutureProjectionMatrix());

	ClientMap &map_node = static_cast<ClientMap &>(m_client->getEnv().getMap());

	video::SMaterial material;
	if (map_node.getMaterialCount() > 0) {
		// we only want the first material, which is the one with the albedo info
		material = map_node.getMaterial(0);
	}

	material.BackfaceCulling = false;
	material.FrontfaceCulling = true;

	if (m_shadow_map_colored && pass != scene::ESNRP_SOLID) {
		material.MaterialType = (video::E_MATERIAL_TYPE) depth_shader_trans;
	}
	else {
		material.MaterialType = (video::E_MATERIAL_TYPE) depth_shader;
		material.BlendOperation = video::EBO_MIN;
	}

	m_driver->setTransform(video::ETS_WORLD,
			map_node.getAbsoluteTransformation());

	int frame = m_force_update_shadow_map ? 0 : m_current_frame;
	int total_frames = m_force_update_shadow_map ? 1 : m_map_shadow_update_frames;

	map_node.renderMapShadows(m_driver, material, pass, frame, total_frames);
}

void ShadowRenderer::renderShadowObjects(
		video::ITexture *target, DirectionalLight &light)
{
	m_driver->setTransform(video::ETS_VIEW, light.getViewMatrix());
	m_driver->setTransform(video::ETS_PROJECTION, light.getProjectionMatrix());

	for (const auto &shadow_node : m_shadow_node_array) {
		// we only take care of the shadow casters and only visible nodes cast shadows
		if (shadow_node.shadowMode == ESM_RECEIVE || !shadow_node.node->isVisible())
			continue;

		// render other objects
		u32 n_node_materials = shadow_node.node->getMaterialCount();
		std::vector<s32> BufferMaterialList;
		std::vector<std::pair<bool, bool>> BufferMaterialCullingList;
		std::vector<video::E_BLEND_OPERATION> BufferBlendOperationList;
		BufferMaterialList.reserve(n_node_materials);
		BufferMaterialCullingList.reserve(n_node_materials);
		BufferBlendOperationList.reserve(n_node_materials);

		// backup materialtype for each material
		// (aka shader)
		// and replace it by our "depth" shader
		for (u32 m = 0; m < n_node_materials; m++) {
			auto &current_mat = shadow_node.node->getMaterial(m);

			BufferMaterialList.push_back(current_mat.MaterialType);
			current_mat.MaterialType =
					(video::E_MATERIAL_TYPE)depth_shader_entities;

			BufferMaterialCullingList.emplace_back(
				(bool)current_mat.BackfaceCulling, (bool)current_mat.FrontfaceCulling);
			BufferBlendOperationList.push_back(current_mat.BlendOperation);

			current_mat.BackfaceCulling = true;
			current_mat.FrontfaceCulling = false;
		}

		m_driver->setTransform(video::ETS_WORLD,
				shadow_node.node->getAbsoluteTransformation());
		shadow_node.node->render();

		// restore the material.

		for (u32 m = 0; m < n_node_materials; m++) {
			auto &current_mat = shadow_node.node->getMaterial(m);

			current_mat.MaterialType = (video::E_MATERIAL_TYPE) BufferMaterialList[m];

			current_mat.BackfaceCulling = BufferMaterialCullingList[m].first;
			current_mat.FrontfaceCulling = BufferMaterialCullingList[m].second;
			current_mat.BlendOperation = BufferBlendOperationList[m];
		}

	} // end for caster shadow nodes
}

/*
 * @Liso's disclaimer ;) This function loads the Shadow Mapping Shaders.
 * I used a custom loader because I couldn't figure out how to use the base
 * Shaders system with custom IShaderConstantSetCallBack without messing up the
 * code too much. If anyone knows how to integrate this with the standard MT
 * shaders, please feel free to change it.
 */

void ShadowRenderer::createShaders()
{
    auto cache = m_client->getResourceCache();

    // base material: ONETEXTURE_BLEND
    depth_shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "shadows_pass1");
    // base material: SOLID
    depth_shader_entities = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "shadows_pass1");
    // base material: SOLID
    mixcsm_shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "shadows_pass2");

    if (m_shadow_map_colored)
        // base material: SOLID
        depth_shader_trans = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "shadows_pass1_trans");
}

void ShadowRenderer::createShadowBuffer()
{
    /*
    vec3 lightDirection;
    float textureresolution;
    mat4 shadowViewProj;
    float shadowfar;
    float shadow_strength;
    float timeofday;
    vec4 cameraPos;
    float xyPerspectiveBias0;
    float xyPerspectiveBias1;
    float zPerspectiveBias;
    vec3 shadowTint;
    */
    ByteArray shadow_ba(
        3 + 1 + 16 + 1 + 1 + 1 + 4 + 1 + 1 + 1 + 3,
        (3 + 1 + 16 + 1 + 1 + 1 + 4 + 1 + 1 + 1 + 3) * sizeof(f32)
    );
    m_shadow_buffer = std::make_unique<render::UniformBuffer>(
        SHADOW_UBO_BINDING_POINT, shadow_ba
    );
}

ShadowRenderer *createShadowRenderer(Client *client)
{
	if (g_settings->getBool("enable_dynamic_shadows")) {
        ShadowRenderer *shadow_renderer = new ShadowRenderer(client);
		shadow_renderer->initialize();
		return shadow_renderer;
	}

	return nullptr;
}
