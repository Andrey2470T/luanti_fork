// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2020 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>

#include "guiScene.h"

#include "client/ao/animation.h"
#include "client/mesh/layeredmesh.h"
#include "client/render/datatexture.h"
#include "client/render/renderer.h"
#include "client/render/rendersystem.h"
#include "client/mesh/meshbuffer.h"
#include "client/mesh/model.h"
#include "gui/IGUIEnvironment.h"
#include "porting.h"
#include "client/mesh/meshoperations.h"
#include "client/render/tilelayer.h"
#include "client/ui/sprite.h"
#include "settings.h"

GUIScene::GUIScene(Client *client, gui::IGUIEnvironment *env,
           gui::IGUIElement *parent, recti rect, s32 id)
    : IGUIElement(EGUIET_ELEMENT, env, parent, id, rect), m_client(client),
      m_background(std::make_unique<UISprite>(nullptr, env->getRenderSystem()->getRenderer(),
        env->getResourceCache(), true))
{
    m_cam = new Camera(v2u(), v3f(0.0f, 0.0f, -100.0f), v3f(0.0f));
    m_cam->setFovY(degToRad(30.0f));

    m_layer = std::make_shared<TileLayer>();
    m_layer->thing = RenderThing::OBJECT;
    m_layer->alpha_discard = 1;
    m_layer->material_flags = MATERIAL_FLAG_TRANSPARENT;

    m_layer->material_type = TILE_MATERIAL_ALPHA;
    m_layer->use_default_shader = false;

    std::string shadername = m_model->getSkeleton() ? "object_skinned" : "object";
    m_layer->shader = env->getResourceCache()->getOrLoad<render::Shader>(ResourceType::SHADER, shadername);
    m_layer->textures.push_back(dynamic_cast<render::Texture *>(
        env->getRenderSystem()->getAnimationManager()->getBonesTexture()->getGLTexture()));
    //m_smgr->getParameters()->setAttribute(scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);
}

GUIScene::~GUIScene()
{
    if (m_model)
        Environment->getResourceCache()->clearResource<Model>(ResourceType::MODEL, m_model);
}

void GUIScene::setModel(Model *model)
{
    auto cache = Environment->getResourceCache();
    if (m_model) {
        cache->clearResource<Model>(ResourceType::MODEL, m_model);
    }

    m_model = model;
    cache->cacheResource<Model>(ResourceType::MODEL, m_model);
}

void GUIScene::setTexture(u32 idx, img::Image *texture)
{
    auto basic_pool = Environment->getRenderSystem()->getPool(true);
    auto buffer = m_model->getMesh()->getBuffer(0);
    auto layer = m_model->getMesh()->getBufferLayer(0, idx);

    basic_pool->updateMeshUVs(buffer, layer.second.offset, layer.second.count, texture, layer.first->tile_ref, true);

    layer.first->tile_ref = texture;
}

void GUIScene::draw()
{
    auto rnd = Environment->getRenderSystem()->getRenderer();
    auto ctxt = rnd->getContext();

    ctxt->clearBuffers(render::CBF_DEPTH);

	// Control rotation speed based on time
	u64 new_time = porting::getTimeMs();
	u64 dtime_ms = 0;
	if (m_last_time != 0)
		dtime_ms = porting::getDeltaMs(m_last_time, new_time);
	m_last_time = new_time;

    recti oldViewPort = ctxt->getViewportSize();
    ctxt->setViewportSize(getAbsoluteClippingRect());

    recti borderRect = Environment->getRootGUIElement()->getAbsoluteClippingRect();

    m_background->clear();
    Environment->getSkin()->add3DSunkenPane(
            m_background.get(), m_bgcolor, false, true, toRectf(borderRect));
    m_background->updateMesh(true);
    m_background->updateMesh(false);
    m_background->draw();

    v2i size = getAbsoluteClippingRect().getSize();
    m_cam->setAspectRatio((f32)size.X / (f32)size.Y);

	// Continuous rotation
	if (m_inf_rot)
		rotateCamera(v3f(0.f, -0.03f * (float)dtime_ms, 0.f));

    if (m_initial_rotation) {
		rotateCamera(v3f(m_custom_rot.X, m_custom_rot.Y, 0.f));
		calcOptimalDistance();

		m_initial_rotation = false;
	}

    updateCamera();

    if (m_model) {
        auto anim = m_model->getAnimation();

        if (anim)
            anim->animateBones(dtime_ms / 1000.0f);

        auto lmesh = m_model->getMesh();
        for (auto &layer : lmesh->getAllLayers()) {
            layer.first->setupRenderState(m_client);

            auto &ml = layer.second;

            rnd->draw(lmesh->getBuffer(0), render::PT_TRIANGLES, ml.offset, ml.count);
        }
    }

    ctxt->setViewportSize(oldViewPort);
}

bool GUIScene::OnEvent(const core::Event &event)
{
	if (m_mouse_ctrl && event.Type == EET_MOUSE_INPUT_EVENT) {
		if (event.MouseInput.Type == EMIE_LMOUSE_PRESSED_DOWN) {
			m_last_pos = v2f((f32)event.MouseInput.X, (f32)event.MouseInput.Y);
			return true;
		} else if (event.MouseInput.Type == EMIE_MOUSE_MOVED) {
			if (event.MouseInput.isLeftPressed()) {
				m_curr_pos = v2f((f32)event.MouseInput.X, (f32)event.MouseInput.Y);

				rotateCamera(v3f(
					m_last_pos.Y - m_curr_pos.Y,
					m_curr_pos.X - m_last_pos.X, 0.f));

				m_last_pos = m_curr_pos;
				return true;
			}
		}
	}

	return gui::IGUIElement::OnEvent(event);
}

void GUIScene::setStyles(const std::array<StyleSpec, StyleSpec::NUM_STATES> &styles)
{
	StyleSpec::State state = StyleSpec::STATE_DEFAULT;
	StyleSpec style = StyleSpec::getStyleFromStatePropagation(styles, state);

	setNotClipped(style.getBool(StyleSpec::NOCLIP, false));
	setBackgroundColor(style.getColor(StyleSpec::BGCOLOR, m_bgcolor));
}

/**
 * Sets the frame loop range for the mesh
 */
void GUIScene::setFrameLoop(s32 begin, s32 end)
{
    auto anim = m_model->getAnimation();

    if (!anim)
        return;

    v2i range = anim->getRange();
    if (range.X != begin || range.Y != end) {
        anim->setRange({begin, end});
        anim->setLooped(true);
    }
}

/**
 * Sets the animation speed (FPS) for the mesh
 */
void GUIScene::setAnimationSpeed(f32 speed)
{
    auto anim = m_model->getAnimation();

    if (!anim)
        return;

    anim->setFPS(speed);
}

/* Camera control functions */

inline void GUIScene::calcOptimalDistance()
{
    aabbf box = m_model->getMesh()->getBuffer(0)->getBoundingBox();
	f32 width  = box.MaxEdge.X - box.MinEdge.X;
	f32 height = box.MaxEdge.Y - box.MinEdge.Y;
	f32 depth  = box.MaxEdge.Z - box.MinEdge.Z;
	f32 max_width = width > depth ? width : depth;

    f32 cam_far = m_cam->getFarValue();
    f32 frustum_length = m_cam->getFarValue();
    f32 far_width = tanf(m_cam->getFovX()) * frustum_length * 2.0f;
    f32 far_height = tanf(m_cam->getFovY()) * frustum_length * 2.0f;

	recti rect = getAbsolutePosition();
	f32 zoomX = rect.getWidth() / max_width;
	f32 zoomY = rect.getHeight() / height;
	f32 dist;

	if (zoomX < zoomY)
		dist = (max_width / (far_width / cam_far)) + (0.5f * max_width);
	else
		dist = (height / (far_height / cam_far)) + (0.5f * max_width);

	m_cam_distance = dist;
	m_update_cam = true;
}

void GUIScene::updateTargetPos(v3f target)
{
    m_last_target_pos = m_target_pos;
    m_target_pos = target;

	m_update_cam = true;
}

void GUIScene::setCameraRotation(v3f rot)
{
    correctBounds(rot);

    matrix4 mat;
    mat.setRotationDegrees(rot);

    m_cam_pos = mat.rotateAndScaleVect(v3f(0.f, 0.f, m_cam_distance));
    m_cam_pos += m_target_pos;

    m_update_cam = true;
}

void GUIScene::updateCamera()
{
    if (!m_update_cam) {
        return;
    }
	m_cam->setPosition(m_cam_pos);
    m_cam->setDirection(m_target_pos-m_cam_pos);
	m_update_cam = false;
}

bool GUIScene::correctBounds(v3f &rot)
{
	const float ROTATION_MAX_1 = 60.0f;
	const float ROTATION_MAX_2 = 300.0f;

	// Limit and correct the rotation when needed
	if (rot.X < 90.f) {
		if (rot.X > ROTATION_MAX_1) {
			rot.X = ROTATION_MAX_1;
			return true;
		}
	} else if (rot.X < ROTATION_MAX_2) {
		rot.X = ROTATION_MAX_2;
		return true;
	}

	// Not modified
	return false;
}

/*void GUIScene::cameraLoop()
{
    //updateCameraPos();
    //updateTargetPos();

    //if (m_target_pos != m_last_target_pos)
    //	m_update_cam = true;

    if (m_update_cam) {
        m_cam_pos = m_target_pos + (m_cam_pos - m_target_pos).normalize() * m_cam_distance;

		v3f rot = getCameraRotation();
		if (correctBounds(rot))
			setCameraRotation(rot);

		m_cam->setPosition(m_cam_pos);
		m_cam->setTarget(m_target_pos);

        m_update_cam = false;
	}
}*/
