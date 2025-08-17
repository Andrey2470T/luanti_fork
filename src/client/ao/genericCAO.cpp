// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "genericCAO.h"
#include "client/client.h"
#include "client/render/rendersystem.h"
#include "client/sound/sound.h"
#include "client/media/resource.h"
#include "client/map/mapblockmesh.h"
#include "util/basic_macros.h"
#include "util/numeric.h"
#include "util/serialize.h"
#include "client/player/playercamera.h" // CameraModes
#include "collision.h"
#include "environment.h"
#include "itemdef.h"
#include "client/player/localplayer.h"
#include "map.h"
#include "client/mesh/meshoperations.h"
#include "nodedef.h"
#include "settings.h"
#include "tool.h"
#include "client/render/wieldmesh.h"
#include <algorithm>
#include <cmath>
#include "client/ui/minimap.h"
#include <Utils/Quaternion.h>

class Settings;
struct ToolCapabilities;

std::unordered_map<u16, ClientActiveObject::Factory> ClientActiveObject::m_types;

template<typename T>
void SmoothTranslator<T>::init(T current)
{
    val_start = current;
	val_current = current;
    val_end = current;
	anim_time = 0;
	anim_time_counter = 0;
	aim_is_end = true;
}

template<typename T>
void SmoothTranslator<T>::update(T new_target, bool is_end_position, float update_interval)
{
	aim_is_end = is_end_position;
    val_start = val_current;
    val_end = new_target;
	if (update_interval > 0) {
		anim_time = update_interval;
	} else {
		if (anim_time < 0.001 || anim_time > 1.0)
			anim_time = anim_time_counter;
		else
			anim_time = anim_time * 0.9 + anim_time_counter * 0.1;
	}
	anim_time_counter = 0;
}

template<typename T>
void SmoothTranslator<T>::translate(f32 dtime)
{
	anim_time_counter = anim_time_counter + dtime;
    T val_diff = val_end - val_start;
	f32 moveratio = 1.0;
	if (anim_time > 0.001)
		moveratio = anim_time_counter / anim_time;
	f32 move_end = aim_is_end ? 1.0 : 1.5;

	// Move a bit less than should, to avoid oscillation
	moveratio = std::min(moveratio * 0.8f, move_end);
    val_current = val_start + val_diff * moveratio;
}

void SmoothTranslatorWrapped::translate(f32 dtime)
{
	anim_time_counter = anim_time_counter + dtime;
    f32 val_diff = std::abs(val_end - val_start);
	if (val_diff > 180.f)
		val_diff = 360.f - val_diff;

	f32 moveratio = 1.0;
	if (anim_time > 0.001)
		moveratio = anim_time_counter / anim_time;
	f32 move_end = aim_is_end ? 1.0 : 1.5;

	// Move a bit less than should, to avoid oscillation
	moveratio = std::min(moveratio * 0.8f, move_end);
    wrappedApproachShortest(val_current, val_end,
		val_diff * moveratio, 360.f);
}

void SmoothTranslatorWrappedv3f::translate(f32 dtime)
{
	anim_time_counter = anim_time_counter + dtime;

    v3f val_diff_v3f;
    val_diff_v3f.X = std::abs(val_end.X - val_start.X);
    val_diff_v3f.Y = std::abs(val_end.Y - val_start.Y);
    val_diff_v3f.Z = std::abs(val_end.Z - val_start.Z);

	if (val_diff_v3f.X > 180.f)
		val_diff_v3f.X = 360.f - val_diff_v3f.X;

	if (val_diff_v3f.Y > 180.f)
		val_diff_v3f.Y = 360.f - val_diff_v3f.Y;

	if (val_diff_v3f.Z > 180.f)
		val_diff_v3f.Z = 360.f - val_diff_v3f.Z;

	f32 moveratio = 1.0;
	if (anim_time > 0.001)
		moveratio = anim_time_counter / anim_time;
	f32 move_end = aim_is_end ? 1.0 : 1.5;

	// Move a bit less than should, to avoid oscillation
	moveratio = std::min(moveratio * 0.8f, move_end);
    wrappedApproachShortest(val_current.X, val_end.X,
		val_diff_v3f.X * moveratio, 360.f);

    wrappedApproachShortest(val_current.Y, val_end.Y,
		val_diff_v3f.Y * moveratio, 360.f);

    wrappedApproachShortest(val_current.Z, val_end.Z,
		val_diff_v3f.Z * moveratio, 360.f);
}

/*
	Other stuff
*/

/*static void setBillboardTextureMatrix(scene::IBillboardSceneNode *bill,
		float txs, float tys, int col, int row)
{
	video::SMaterial& material = bill->getMaterial(0);
	core::matrix4& matrix = material.getTextureMatrix(0);
	matrix.setTextureTranslate(txs*col, tys*row);
	matrix.setTextureScale(txs, tys);
}*/

// Evaluate transform chain recursively; irrlicht does not do this for us
/*static void updatePositionRecursive(scene::ISceneNode *node)
{
	scene::ISceneNode *parent = node->getParent();
	if (parent)
		updatePositionRecursive(parent);
	node->updateAbsolutePosition();
}*/

static bool logOnce(const std::ostringstream &from, std::ostream &log_to)
{
	thread_local std::vector<u64> logged;

	std::string message = from.str();
	u64 hash = murmur_hash_64_ua(message.data(), message.length(), 0xBADBABE);

	if (std::find(logged.begin(), logged.end(), hash) != logged.end())
		return false;
	logged.push_back(hash);
	log_to << message << std::endl;
	return true;
}

/*static void setColorParam(scene::ISceneNode *node, video::SColor color)
{
	for (u32 i = 0; i < node->getMaterialCount(); ++i)
		node->getMaterial(i).ColorParam = color;
}*/


/*
	GenericCAO
*/

GenericCAO::GenericCAO(Client *client, ClientEnvironment *env):
    ClientActiveObject(0, client, env), m_node_mgr(env->getTransformNodeManager())
{
	if (!client) {
        ClientActiveObject::registerType(getType(), create);
	} else {
		m_client = client;
	}
}

void GenericCAO::initialize(const std::string &data)
{
	processInitData(data);
}

void GenericCAO::processInitData(const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	const u8 version = readU8(is);

	if (version < 1) {
		errorstream << "GenericCAO: Unsupported init data version"
				<< std::endl;
		return;
	}

	// PROTOCOL_VERSION >= 37
	m_name = deSerializeString16(is);
	m_is_player = readU8(is);
	m_id = readU16(is);
	m_position = readV3F32(is);
	m_rotation = readV3F32(is);
	m_hp = readU16(is);

	if (m_is_player) {
		// Check if it's the current player
		LocalPlayer *player = m_env->getLocalPlayer();
		if (player && player->getName() == m_name) {
			m_is_local_player = true;
            //m_is_visible = false;
			player->setCAO(this);
		}
	}

	const u8 num_messages = readU8(is);
	for (u8 i = 0; i < num_messages; i++) {
		std::string message = deSerializeString32(is);
		processMessage(message);
	}

	m_rotation = wrapDegrees_0_360_v3f(m_rotation);
	pos_translator.init(m_position);
	rot_translator.init(m_rotation);
	updateNodePos();
}

GenericCAO::~GenericCAO()
{
	removeFromScene(true);
}

const v3f GenericCAO::getPosition() const
{
	if (!getParent())
		return pos_translator.val_current;
    else {
        v3s16 camera_offset = m_env->getCameraOffset();
        return getAttachmentNode()->getAbsolutePosition() + intToFloat(camera_offset, BS);
    }
	// Calculate real position in world based on MatrixNode
    /*if (m_matrixnode) {
		v3s16 camera_offset = m_env->getCameraOffset();
		return m_matrixnode->getAbsolutePosition() +
				intToFloat(camera_offset, BS);
    }*/

	return m_position;
}

const v3f GenericCAO::getRotation() const
{
    if (!getParent())
        return rot_translator.val_current;
    else {
        auto rot = getAttachmentNode()->getAbsoluteRotation();

        v3f euler_rot;
        rot.toEuler(euler_rot);

        return euler_rot;
    }
}

bool GenericCAO::isImmortal() const
{
	return itemgroup_get(getGroups(), "immortal");
}

matrix4 &GenericCAO::getRelativeMatrix()
{
    if (!getParent())
        return m_rel_transform;
    else
        return getAttachmentNode()->RelativeTransform;
}

const matrix4 &GenericCAO::getAbsoluteMatrix() const
{
    if (!getParent())
        return m_abs_transform;
    else
        return getAttachmentNode()->AbsoluteTransform;
}

Attachment *GenericCAO::getAttachmentNode() const
{
    if (!m_attachment_tree_id.has_value() || !m_attachment_node_id.has_value())
        return nullptr;

    return dynamic_cast<Attachment *>(m_node_mgr->getNode(
        m_attachment_tree_id.value(), m_attachment_node_id.value()));
}

void GenericCAO::setAttachment(object_t parent_id, const std::string &bone,
		v3f position, v3f rotation, bool force_visible)
{
    TransformNodeTree *tree;

    auto new_parent = m_env->getGenericCAO(parent_id);
    auto new_parent_node = new_parent->getAttachmentNode();

    if (new_parent_node) {
        tree = new_parent_node->Tree;

        auto cur_parent = getParent();
        if (parent_id != cur_parent->getId())
            cur_parent->removeAttachmentChild(m_id);
    }
    else {
        tree = new TransformNodeTree();
        new_parent_node = new Attachment();
        new_parent_node->ObjectId = parent_id;
        new_parent_node->Position = new_parent->getPosition();
        new_parent_node->Rotation = new_parent->getRotation();
        tree->addNode(new_parent_node);
        m_node_mgr->addNodeTree(tree);
    }

    new_parent->addAttachmentChild(m_id);
    /*const auto old_parent = m_attachment_parent_id;
    m_attachment_parent_id = parent_id;
	m_attachment_bone = bone;
	m_attachment_position = position;
	m_attachment_rotation = rotation;
	m_force_visible = force_visible;

	ClientActiveObject *parent = m_env->getActiveObject(parent_id);

	if (parent_id != old_parent) {
		if (auto *o = m_env->getActiveObject(old_parent))
			o->removeAttachmentChild(m_id);
		if (parent)
			parent->addAttachmentChild(m_id);
	}
	updateAttachments();

	// Forcibly show attachments if required by set_attach
	if (m_force_visible) {
		m_is_visible = true;
	} else if (!m_is_local_player) {
		// Objects attached to the local player should be hidden in first person
		m_is_visible = !m_attached_to_local ||
			m_client->getCamera()->getCameraMode() != CAMERA_MODE_FIRST;
		m_force_visible = false;
	} else {
		// Local players need to have this set,
		// otherwise first person attachments fail.
		m_is_visible = true;
    }*/
}

void GenericCAO::getAttachment(object_t *parent_id, std::string *bone, v3f *position,
	v3f *rotation, bool *force_visible) const
{
	*parent_id = m_attachment_parent_id;
	*bone = m_attachment_bone;
	*position = m_attachment_position;
	*rotation = m_attachment_rotation;
	*force_visible = m_force_visible;
}

void GenericCAO::clearChildAttachments()
{
	// Cannot use for-loop here: setAttachment() modifies 'm_attachment_child_ids'!
	while (!m_attachment_child_ids.empty()) {
		const auto child_id = *m_attachment_child_ids.begin();

		if (auto *child = m_env->getActiveObject(child_id))
			child->clearParentAttachment();
		else
			removeAttachmentChild(child_id);
	}
}

void GenericCAO::addAttachmentChild(object_t child_id)
{
	m_attachment_child_ids.insert(child_id);
}

void GenericCAO::removeAttachmentChild(object_t child_id)
{
	m_attachment_child_ids.erase(child_id);
}

GenericCAO* GenericCAO::getParent() const
{
    auto node = getAttachmentNode();
    if (!node)
        return nullptr;

    auto parent = node->getParent();
    if (!parent || parent->Type != TransformNodeType::OBJECT)
        return nullptr;

    return m_env->getGenericCAO(dynamic_cast<Attachment *>(parent)->ObjectId);
}

/*void GenericCAO::removeFromScene(bool permanent)
{
	// Should be true when removing the object permanently
	// and false when refreshing (eg: updating visuals)
	if (m_env && permanent) {
		// The client does not know whether this object does re-appear to
		// a later time, thus do not clear child attachments.

		clearParentAttachment();
	}

	if (auto shadow = RenderingEngine::get_shadow_renderer())
		if (auto node = getSceneNode())
			shadow->removeNodeFromShadowList(node);

	if (m_meshnode) {
		m_meshnode->remove();
		m_meshnode->drop();
		m_meshnode = nullptr;
	} else if (m_animated_meshnode)	{
		m_animated_meshnode->remove();
		m_animated_meshnode->drop();
		m_animated_meshnode = nullptr;
	} else if (m_wield_meshnode) {
		m_wield_meshnode->remove();
		m_wield_meshnode->drop();
		m_wield_meshnode = nullptr;
	} else if (m_spritenode) {
		m_spritenode->remove();
		m_spritenode->drop();
		m_spritenode = nullptr;
	}

	if (m_matrixnode) {
		m_matrixnode->remove();
		m_matrixnode->drop();
		m_matrixnode = nullptr;
	}

	if (m_nametag) {
		m_client->getCamera()->removeNametag(m_nametag);
		m_nametag = nullptr;
	}

	if (m_marker && m_client->getMinimap())
		m_client->getMinimap()->removeMarker(&m_marker);
}*/

void GenericCAO::addToScene()
{
	m_smgr = smgr;

	if (getSceneNode() != NULL) {
		return;
	}

	m_visuals_expired = false;

	if (!m_prop.is_visible)
		return;

	infostream << "GenericCAO::addToScene(): " << m_prop.visual << std::endl;

	m_material_type_param = 0.5f; // May cut off alpha < 128 depending on m_material_type

	{
		IShaderSource *shader_source = m_client->getShaderSource();
		MaterialType material_type;

		if (m_prop.shaded && m_prop.glow == 0)
			material_type = (m_prop.use_texture_alpha) ?
				TILE_MATERIAL_ALPHA : TILE_MATERIAL_BASIC;
		else
			material_type = (m_prop.use_texture_alpha) ?
				TILE_MATERIAL_PLAIN_ALPHA : TILE_MATERIAL_PLAIN;

		u32 shader_id = shader_source->getShader("object_shader", material_type, NDT_NORMAL);
		m_material_type = shader_source->getShaderInfo(shader_id).material;
	}

	auto grabMatrixNode = [this] {
		m_matrixnode = m_smgr->addDummyTransformationSceneNode();
		m_matrixnode->grab();
	};

	auto setMaterial = [this] (video::SMaterial &mat) {
		mat.MaterialType = m_material_type;
		mat.FogEnable = true;
		mat.forEachTexture([] (auto &tex) {
			tex.MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;
			tex.MagFilter = video::ETMAGF_NEAREST;
		});
	};

	auto setSceneNodeMaterials = [setMaterial] (scene::ISceneNode *node) {
		node->forEachMaterial(setMaterial);
	};

	if (m_prop.visual == "sprite") {
		grabMatrixNode();
		m_spritenode = m_smgr->addBillboardSceneNode(
				m_matrixnode, v2f(1, 1), v3f(0,0,0), -1);
		m_spritenode->grab();
		video::ITexture *tex = tsrc->getTextureForMesh("no_texture.png");
		m_spritenode->forEachMaterial([tex] (auto &mat) {
			mat.setTexture(0, tex);
		});

		setSceneNodeMaterials(m_spritenode);

		m_spritenode->setSize(v2f(m_prop.visual_size.X,
				m_prop.visual_size.Y) * BS);
		{
			const float txs = 1.0 / 1;
			const float tys = 1.0 / 1;
			setBillboardTextureMatrix(m_spritenode,
					txs, tys, 0, 0);
		}
	} else if (m_prop.visual == "upright_sprite") {
		grabMatrixNode();
		auto mesh = make_irr<scene::SMesh>();
		f32 dx = BS * m_prop.visual_size.X / 2;
		f32 dy = BS * m_prop.visual_size.Y / 2;
		video::SColor c(0xFFFFFFFF);

		video::S3DVertex vertices[4] = {
			video::S3DVertex(-dx, -dy, 0, 0,0,1, c, 1,1),
			video::S3DVertex( dx, -dy, 0, 0,0,1, c, 0,1),
			video::S3DVertex( dx,  dy, 0, 0,0,1, c, 0,0),
			video::S3DVertex(-dx,  dy, 0, 0,0,1, c, 1,0),
		};
		if (m_is_player) {
			// Move minimal Y position to 0 (feet position)
			for (auto &vertex : vertices)
				vertex.Pos.Y += dy;
		}
		const u16 indices[] = {0,1,2,2,3,0};

		for (int face : {0, 1}) {
			auto buf = make_irr<scene::SMeshBuffer>();
			// Front (0) or Back (1)
			if (face == 1) {
				for (auto &v : vertices)
					v.Normal *= -1;
				for (int i : {0, 2})
					std::swap(vertices[i].Pos, vertices[i+1].Pos);
			}
			buf->append(vertices, 4, indices, 6);

			// Set material
			setMaterial(buf->getMaterial());
			buf->getMaterial().ColorParam = c;

			// Add to mesh
			mesh->addMeshBuffer(buf.get());
		}

		mesh->recalculateBoundingBox();
		m_meshnode = m_smgr->addMeshSceneNode(mesh.get(), m_matrixnode);
		m_meshnode->grab();
	} else if (m_prop.visual == "cube") {
		grabMatrixNode();
		scene::IMesh *mesh = createCubeMesh(v3f(BS,BS,BS));
		m_meshnode = m_smgr->addMeshSceneNode(mesh, m_matrixnode);
		m_meshnode->grab();
		mesh->drop();

		m_meshnode->setScale(m_prop.visual_size);

		setSceneNodeMaterials(m_meshnode);

		m_meshnode->forEachMaterial([this] (auto &mat) {
			mat.BackfaceCulling = m_prop.backface_culling;
		});
	} else if (m_prop.visual == "mesh") {
		grabMatrixNode();
		scene::IAnimatedMesh *mesh = m_client->getMesh(m_prop.mesh, true);
		if (mesh) {
			if (!checkMeshNormals(mesh)) {
				infostream << "GenericCAO: recalculating normals for mesh "
					<< m_prop.mesh << std::endl;
				m_smgr->getMeshManipulator()->
						recalculateNormals(mesh, true, false);
			}

			m_animated_meshnode = m_smgr->addAnimatedMeshSceneNode(mesh, m_matrixnode);
			m_animated_meshnode->grab();
			mesh->drop(); // The scene node took hold of it
			m_animated_meshnode->animateJoints(); // Needed for some animations
			m_animated_meshnode->setScale(m_prop.visual_size);

			// set vertex colors to ensure alpha is set
			setMeshColor(m_animated_meshnode->getMesh(), video::SColor(0xFFFFFFFF));

			setSceneNodeMaterials(m_animated_meshnode);

			m_animated_meshnode->forEachMaterial([this] (auto &mat) {
				mat.BackfaceCulling = m_prop.backface_culling;
			});
		} else
			errorstream<<"GenericCAO::addToScene(): Could not load mesh "<<m_prop.mesh<<std::endl;
	} else if (m_prop.visual == "wielditem" || m_prop.visual == "item") {
		grabMatrixNode();
		ItemStack item;
		if (m_prop.wield_item.empty()) {
			// Old format, only textures are specified.
			infostream << "textures: " << m_prop.textures.size() << std::endl;
			if (!m_prop.textures.empty()) {
				infostream << "textures[0]: " << m_prop.textures[0]
					<< std::endl;
				IItemDefManager *idef = m_client->idef();
				item = ItemStack(m_prop.textures[0], 1, 0, idef);
			}
		} else {
			infostream << "serialized form: " << m_prop.wield_item << std::endl;
			item.deSerialize(m_prop.wield_item, m_client->idef());
		}
		m_wield_meshnode = new WieldMeshSceneNode(m_smgr, -1);
		m_wield_meshnode->setItem(item, m_client,
			(m_prop.visual == "wielditem"));

		m_wield_meshnode->setScale(m_prop.visual_size / 2.0f);
	} else {
		infostream<<"GenericCAO::addToScene(): \""<<m_prop.visual
				<<"\" not supported"<<std::endl;
	}

	/* Set VBO hint */
	// wieldmesh sets its own hint, no need to handle it
	if (m_meshnode || m_animated_meshnode) {
		// sprite uses vertex animation
		if (m_meshnode && m_prop.visual != "upright_sprite")
			m_meshnode->getMesh()->setHardwareMappingHint(scene::EHM_STATIC);

		if (m_animated_meshnode) {
			auto *mesh = m_animated_meshnode->getMesh();
			// skinning happens on the CPU
			if (m_animated_meshnode->getJointCount() > 0)
				mesh->setHardwareMappingHint(scene::EHM_STREAM, scene::EBT_VERTEX);
			else
				mesh->setHardwareMappingHint(scene::EHM_STATIC, scene::EBT_VERTEX);
			mesh->setHardwareMappingHint(scene::EHM_STATIC, scene::EBT_INDEX);
		}
	}

	/* don't update while punch texture modifier is active */
	if (m_reset_textures_timer < 0)
		updateTextures(m_current_texture_modifier);

	if (scene::ISceneNode *node = getSceneNode()) {
		if (m_matrixnode)
			node->setParent(m_matrixnode);

		if (auto shadow = RenderingEngine::get_shadow_renderer())
			shadow->addNodeToShadowList(node);
	}

	updateNametag();
	updateMarker();
	updateNodePos();
	updateAnimation();
	updateBones(.0f);
	updateAttachments();
	setNodeLight(m_last_light);
	updateMeshCulling();

	if (m_animated_meshnode) {
		u32 mat_count = m_animated_meshnode->getMaterialCount();
		assert(mat_count == m_animated_meshnode->getMesh()->getMeshBufferCount());
		u32 max_tex_idx = 0;
		for (u32 i = 0; i < mat_count; ++i) {
			max_tex_idx = std::max(max_tex_idx,
					m_animated_meshnode->getMesh()->getTextureSlot(i));
		}
		if (mat_count == 0 || m_prop.textures.empty()) {
			// nothing
		} else if (max_tex_idx >= m_prop.textures.size()) {
			std::ostringstream oss;
			oss << "GenericCAO::addToScene(): Model "
				<< m_prop.mesh << " is missing " << (max_tex_idx + 1 - m_prop.textures.size())
				<< " more texture(s), this is deprecated.";
			logOnce(oss, warningstream);

			video::ITexture *last = m_animated_meshnode->getMaterial(0).TextureLayers[0].Texture;
			for (u32 i = 1; i < mat_count; i++) {
				auto &layer = m_animated_meshnode->getMaterial(i).TextureLayers[0];
				if (!layer.Texture)
					layer.Texture = last;
				last = layer.Texture;
			}
		}
	}
}

void GenericCAO::updateNodePos()
{
	if (getParent() != NULL)
		return;

	scene::ISceneNode *node = getSceneNode();

	if (node) {
		v3s16 camera_offset = m_env->getCameraOffset();
		v3f pos = pos_translator.val_current -
				intToFloat(camera_offset, BS);
		getPosRotMatrix().setTranslation(pos);
		if (node != m_spritenode) { // rotate if not a sprite
			v3f rot = m_is_local_player ? -m_rotation : -rot_translator.val_current;
			setPitchYawRoll(getPosRotMatrix(), rot);
		}
	}
}

void GenericCAO::step(float dtime, ClientEnvironment *env)
{
	// Handle model animations and update positions instantly to prevent lags
	if (m_is_local_player) {
		LocalPlayer *player = m_env->getLocalPlayer();
		m_position = player->getPosition();
		pos_translator.val_current = m_position;
		m_rotation.Y = wrapDegrees_0_360(player->getYaw());
		rot_translator.val_current = m_rotation;

		if (m_is_visible) {
			LocalPlayerAnimation old_anim = player->last_animation;
			float old_anim_speed = player->last_animation_speed;
			m_velocity = v3f(0,0,0);
			m_acceleration = v3f(0,0,0);
			const PlayerControl &controls = player->getPlayerControl();
			f32 new_speed = player->local_animation_speed;

			bool walking = false;
			if (controls.movement_speed > 0.001f) {
				new_speed *= controls.movement_speed;
				walking = true;
			}

			v2f new_anim(0,0);
			bool allow_update = false;

			// increase speed if using fast or flying fast
			if((g_settings->getBool("fast_move") &&
					m_client->checkLocalPrivilege("fast")) &&
					(controls.aux1 ||
					(!player->touching_ground &&
					g_settings->getBool("free_move") &&
					m_client->checkLocalPrivilege("fly"))))
					new_speed *= 1.5;
			// slowdown speed if sneaking
			if (controls.sneak && walking)
				new_speed /= 2;

			if (walking && (controls.dig || controls.place)) {
				new_anim = player->local_animations[3];
				player->last_animation = LocalPlayerAnimation::WD_ANIM;
			} else if (walking) {
				new_anim = player->local_animations[1];
				player->last_animation = LocalPlayerAnimation::WALK_ANIM;
			} else if (controls.dig || controls.place) {
				new_anim = player->local_animations[2];
				player->last_animation = LocalPlayerAnimation::DIG_ANIM;
			}

			// Apply animations if input detected and not attached
			// or set idle animation
			if ((new_anim.X + new_anim.Y) > 0 && !getParent()) {
				allow_update = true;
				m_animation_range = new_anim;
				m_animation_speed = new_speed;
				player->last_animation_speed = m_animation_speed;
			} else {
				player->last_animation = LocalPlayerAnimation::NO_ANIM;

				if (old_anim != LocalPlayerAnimation::NO_ANIM) {
					m_animation_range = player->local_animations[0];
					updateAnimation();
				}
			}

			// Update local player animations
			if ((player->last_animation != old_anim ||
					m_animation_speed != old_anim_speed) &&
					player->last_animation != LocalPlayerAnimation::NO_ANIM &&
					allow_update)
				updateAnimation();

		}
	}

	if (m_visuals_expired && m_smgr) {
		m_visuals_expired = false;

		// Attachments, part 1: All attached objects must be unparented first,
		// or Irrlicht causes a segmentation fault
		for (u16 cao_id : m_attachment_child_ids) {
			ClientActiveObject *obj = m_env->getActiveObject(cao_id);
			if (obj) {
				scene::ISceneNode *child_node = obj->getSceneNode();
				// The node's parent is always an IDummyTraformationSceneNode,
				// so we need to reparent that one instead.
				if (child_node)
					child_node->getParent()->setParent(m_smgr->getRootSceneNode());
			}
		}

		removeFromScene(false);
		addToScene(m_client->tsrc(), m_smgr);

		// Attachments, part 2: Now that the parent has been refreshed, put its attachments back
		for (u16 cao_id : m_attachment_child_ids) {
			ClientActiveObject *obj = m_env->getActiveObject(cao_id);
			if (obj)
				obj->updateAttachments();
		}
	}

	// Make sure m_is_visible is always applied
	scene::ISceneNode *node = getSceneNode();
	if (node)
		node->setVisible(m_is_visible);

	if(getParent() != NULL) // Attachments should be glued to their parent by Irrlicht
	{
		// Set these for later
		m_position = getPosition();
		m_velocity = v3f(0,0,0);
		m_acceleration = v3f(0,0,0);
		pos_translator.val_current = m_position;
        pos_translator.val_end = m_position;
	} else {
		rot_translator.translate(dtime);
		v3f lastpos = pos_translator.val_current;

		if(m_prop.physical)
		{
			aabb3f box = m_prop.collisionbox;
			box.MinEdge *= BS;
			box.MaxEdge *= BS;
			collisionMoveResult moveresult;
			v3f p_pos = m_position;
			v3f p_velocity = m_velocity;
			moveresult = collisionMoveSimple(env,env->getGameDef(),
					box, m_prop.stepheight, dtime,
					&p_pos, &p_velocity, m_acceleration,
					this, m_prop.collideWithObjects);
			// Apply results
			m_position = p_pos;
			m_velocity = p_velocity;

			bool is_end_position = moveresult.collides;
			pos_translator.update(m_position, is_end_position, dtime);
		} else {
			m_position += dtime * m_velocity + 0.5 * dtime * dtime * m_acceleration;
			m_velocity += dtime * m_acceleration;
			pos_translator.update(m_position, pos_translator.aim_is_end,
					pos_translator.anim_time);
		}
		pos_translator.translate(dtime);
		updateNodePos();

		float moved = lastpos.getDistanceFrom(pos_translator.val_current);
		m_step_distance_counter += moved;
		if (m_step_distance_counter > 1.5f * BS) {
			m_step_distance_counter = 0.0f;
			if (!m_is_local_player && m_prop.makes_footstep_sound) {
				const NodeDefManager *ndef = m_client->ndef();
				v3f foot_pos = getPosition() * (1.0f/BS)
						+ v3f(0.0f, m_prop.collisionbox.MinEdge.Y, 0.0f);
				v3s16 node_below_pos = floatToInt(foot_pos + v3f(0.0f, -0.5f, 0.0f),
						1.0f);
				MapNode n = m_env->getMap().getNode(node_below_pos);
				SoundSpec spec = ndef->get(n).sound_footstep;
				// Reduce footstep gain, as non-local-player footsteps are
				// somehow louder.
				spec.gain *= 0.6f;
				// The footstep-sound doesn't travel with the object. => vel=0
				m_client->sound()->playSoundAt(0, spec, foot_pos, v3f(0.0f));
			}
		}
	}

	m_anim_timer += dtime;
	if(m_anim_timer >= m_anim_framelength)
	{
		m_anim_timer -= m_anim_framelength;
		m_anim_frame++;
		if(m_anim_frame >= m_anim_num_frames)
			m_anim_frame = 0;
	}

	updateTexturePos();

	if(m_reset_textures_timer >= 0)
	{
		m_reset_textures_timer -= dtime;
		if(m_reset_textures_timer <= 0) {
			m_reset_textures_timer = -1;
			updateTextures(m_previous_texture_modifier);
		}
	}

	if (node && std::abs(m_prop.automatic_rotate) > 0.001f) {
		// This is the child node's rotation. It is only used for automatic_rotate.
		v3f local_rot = node->getRotation();
		local_rot.Y = modulo360f(local_rot.Y - dtime * core::RADTODEG *
				m_prop.automatic_rotate);
		node->setRotation(local_rot);
	}

	if (!getParent() && m_prop.automatic_face_movement_dir &&
			(fabs(m_velocity.Z) > 0.001f || fabs(m_velocity.X) > 0.001f)) {
		float target_yaw = atan2(m_velocity.Z, m_velocity.X) * 180 / M_PI
				+ m_prop.automatic_face_movement_dir_offset;
		float max_rotation_per_sec =
				m_prop.automatic_face_movement_max_rotation_per_sec;

		if (max_rotation_per_sec > 0) {
			wrappedApproachShortest(m_rotation.Y, target_yaw,
				dtime * max_rotation_per_sec, 360.f);
		} else {
			// Negative values of max_rotation_per_sec mean disabled.
			m_rotation.Y = target_yaw;
		}

		rot_translator.val_current = m_rotation;
		updateNodePos();
	}

	if (m_animated_meshnode) {
		// Everything must be updated; the whole transform
		// chain as well as the animated mesh node.
		// Otherwise, bone attachments would be relative to
		// a position that's one frame old.
		if (m_matrixnode)
			updatePositionRecursive(m_matrixnode);
		m_animated_meshnode->updateAbsolutePosition();
		m_animated_meshnode->animateJoints();
		updateBones(dtime);
	}
}

/*static void setMeshBufferTextureCoords(scene::IMeshBuffer *buf, const v2f *uv, u32 count)
{
	assert(buf->getVertexType() == video::EVT_STANDARD);
	assert(buf->getVertexCount() == count);
	auto *vertices = static_cast<video::S3DVertex *>(buf->getVertices());
	for (u32 i = 0; i < count; i++)
		vertices[i].TCoords = uv[i];
	buf->setDirty(scene::EBT_VERTEX);
}*/

void GenericCAO::updateAttachments()
{
	ClientActiveObject *parent = getParent();

	m_attached_to_local = parent && parent->isLocalPlayer();

	/*
	Following cases exist:
		m_attachment_parent_id == 0 && !parent
			This object is not attached
		m_attachment_parent_id != 0 && parent
			This object is attached
		m_attachment_parent_id != 0 && !parent
			This object will be attached as soon the parent is known
		m_attachment_parent_id == 0 && parent
			Impossible case
	*/

	if (!parent) { // Detach or don't attach
		if (m_matrixnode) {
			v3s16 camera_offset = m_env->getCameraOffset();
			v3f old_pos = getPosition();

			m_matrixnode->setParent(m_smgr->getRootSceneNode());
			getPosRotMatrix().setTranslation(old_pos - intToFloat(camera_offset, BS));
			m_matrixnode->updateAbsolutePosition();
		}
	}
	else // Attach
	{
		parent->updateAttachments();
		scene::ISceneNode *parent_node = parent->getSceneNode();
		scene::IAnimatedMeshSceneNode *parent_animated_mesh_node =
				parent->getAnimatedMeshSceneNode();
		if (parent_animated_mesh_node && !m_attachment_bone.empty()) {
			parent_node = parent_animated_mesh_node->getJointNode(m_attachment_bone.c_str());
		}

		if (m_matrixnode && parent_node) {
			m_matrixnode->setParent(parent_node);
			parent_node->updateAbsolutePosition();
			getPosRotMatrix().setTranslation(m_attachment_position);
			//setPitchYawRoll(getPosRotMatrix(), m_attachment_rotation);
			// use Irrlicht eulers instead
			getPosRotMatrix().setRotationDegrees(m_attachment_rotation);
			m_matrixnode->updateAbsolutePosition();
		}
	}
}

void GenericCAO::processMessage(const std::string &data)
{
	//infostream<<"GenericCAO: Got message"<<std::endl;
	std::istringstream is(data, std::ios::binary);
	// command
	u8 cmd = readU8(is);
	if (cmd == AO_CMD_SET_PROPERTIES) {
		ObjectProperties newprops;
		newprops.show_on_minimap = m_is_player; // default

		newprops.deSerialize(is);

		// Check what exactly changed
		bool expire_visuals = visualExpiryRequired(newprops);
		bool textures_changed = m_prop.textures != newprops.textures;

		// Apply changes
		m_prop = std::move(newprops);

		m_selection_box = m_prop.selectionbox;
		m_selection_box.MinEdge *= BS;
		m_selection_box.MaxEdge *= BS;

		m_tx_size.X = 1.0f / m_prop.spritediv.X;
		m_tx_size.Y = 1.0f / m_prop.spritediv.Y;

		if(!m_initial_tx_basepos_set){
			m_initial_tx_basepos_set = true;
			m_tx_basepos = m_prop.initial_sprite_basepos;
		}
		if (m_is_local_player) {
			LocalPlayer *player = m_env->getLocalPlayer();
			player->makes_footstep_sound = m_prop.makes_footstep_sound;
			aabb3f collision_box = m_prop.collisionbox;
			collision_box.MinEdge *= BS;
			collision_box.MaxEdge *= BS;
			player->setCollisionbox(collision_box);
			player->setEyeHeight(m_prop.eye_height);
			player->setZoomFOV(m_prop.zoom_fov);
		}

		if ((m_is_player && !m_is_local_player) && m_prop.nametag.empty())
			m_prop.nametag = m_name;
		if (m_is_local_player)
			m_prop.show_on_minimap = false;

		if (expire_visuals) {
			expireVisuals();
		} else {
			if (textures_changed) {
				// don't update while punch texture modifier is active
				if (m_reset_textures_timer < 0)
					updateTextures(m_current_texture_modifier);
			}
			updateNametag();
			updateMarker();
		}
	} else if (cmd == AO_CMD_UPDATE_POSITION) {
		// Not sent by the server if this object is an attachment.
		// We might however get here if the server notices the object being detached before the client.
		m_position = readV3F32(is);
		m_velocity = readV3F32(is);
		m_acceleration = readV3F32(is);
		m_rotation = readV3F32(is);

		m_rotation = wrapDegrees_0_360_v3f(m_rotation);
		bool do_interpolate = readU8(is);
		bool is_end_position = readU8(is);
		float update_interval = readF32(is);

		if(getParent() != NULL) // Just in case
			return;

		if(do_interpolate)
		{
			if(!m_prop.physical)
				pos_translator.update(m_position, is_end_position, update_interval);
		} else {
			pos_translator.init(m_position);
		}
		rot_translator.update(m_rotation, false, update_interval);
		updateNodePos();
	} else if (cmd == AO_CMD_SET_TEXTURE_MOD) {
		std::string mod = deSerializeString16(is);

		// immediately reset an engine issued texture modifier if a mod sends a different one
		if (m_reset_textures_timer > 0) {
			m_reset_textures_timer = -1;
			updateTextures(m_previous_texture_modifier);
		}
		updateTextures(mod);
	} else if (cmd == AO_CMD_SET_SPRITE) {
		v2s16 p = readV2S16(is);
		int num_frames = readU16(is);
		float framelength = readF32(is);
		bool select_horiz_by_yawpitch = readU8(is);

		m_tx_basepos = p;
		m_anim_num_frames = num_frames;
		m_anim_frame = 0;
		m_anim_framelength = framelength;
		m_tx_select_horiz_by_yawpitch = select_horiz_by_yawpitch;

		updateTexturePos();
	} else if (cmd == AO_CMD_SET_PHYSICS_OVERRIDE) {
		float override_speed = readF32(is);
		float override_jump = readF32(is);
		float override_gravity = readF32(is);
		// MT 0.4.10 legacy: send inverted for detault `true` if the server sends nothing
		bool sneak = !readU8(is);
		bool sneak_glitch = !readU8(is);
		bool new_move = !readU8(is);

		// new overrides since 5.8.0
		float override_speed_climb = readF32(is);
		float override_speed_crouch = readF32(is);
		float override_liquid_fluidity = readF32(is);
		float override_liquid_fluidity_smooth = readF32(is);
		float override_liquid_sink = readF32(is);
		float override_acceleration_default = readF32(is);
		float override_acceleration_air = readF32(is);
		if (is.eof()) {
			override_speed_climb = 1.0f;
			override_speed_crouch = 1.0f;
			override_liquid_fluidity = 1.0f;
			override_liquid_fluidity_smooth = 1.0f;
			override_liquid_sink = 1.0f;
			override_acceleration_default = 1.0f;
			override_acceleration_air = 1.0f;
		}

		// new overrides since 5.9.0
		float override_speed_fast = readF32(is);
		float override_acceleration_fast = readF32(is);
		float override_speed_walk = readF32(is);
		if (is.eof()) {
			override_speed_fast = 1.0f;
			override_acceleration_fast = 1.0f;
			override_speed_walk = 1.0f;
		}

		if (m_is_local_player) {
			auto &phys = m_env->getLocalPlayer()->physics_override;
			phys.speed = override_speed;
			phys.jump = override_jump;
			phys.gravity = override_gravity;
			phys.sneak = sneak;
			phys.sneak_glitch = sneak_glitch;
			phys.new_move = new_move;
			phys.speed_climb = override_speed_climb;
			phys.speed_crouch = override_speed_crouch;
			phys.liquid_fluidity = override_liquid_fluidity;
			phys.liquid_fluidity_smooth = override_liquid_fluidity_smooth;
			phys.liquid_sink = override_liquid_sink;
			phys.acceleration_default = override_acceleration_default;
			phys.acceleration_air = override_acceleration_air;
			phys.speed_fast = override_speed_fast;
			phys.acceleration_fast = override_acceleration_fast;
			phys.speed_walk = override_speed_walk;
		}
	} else if (cmd == AO_CMD_SET_ANIMATION) {
		v2f range = readV2F32(is);
		if (!m_is_local_player) {
			m_animation_range = range;
			m_animation_speed = readF32(is);
			m_animation_blend = readF32(is);
			// these are sent inverted so we get true when the server sends nothing
			m_animation_loop = !readU8(is);
			updateAnimation();
		} else {
			LocalPlayer *player = m_env->getLocalPlayer();
			if(player->last_animation == LocalPlayerAnimation::NO_ANIM)
			{
				m_animation_range = range;
				m_animation_speed = readF32(is);
				m_animation_blend = readF32(is);
				// these are sent inverted so we get true when the server sends nothing
				m_animation_loop = !readU8(is);
			}
			// update animation only if local animations present
			// and received animation is unknown (except idle animation)
			bool is_known = false;
			for (int i = 1;i<4;i++)
			{
				if(m_animation_range.Y == player->local_animations[i].Y)
					is_known = true;
			}
			if(!is_known ||
					(player->local_animations[1].Y + player->local_animations[2].Y < 1))
			{
					updateAnimation();
			}
			// FIXME: ^ This code is trash. It's also broken.
		}
	} else if (cmd == AO_CMD_SET_ANIMATION_SPEED) {
		m_animation_speed = readF32(is);
		updateAnimationSpeed();
	} else if (cmd == AO_CMD_SET_BONE_POSITION) {
		std::string bone = deSerializeString16(is);
		auto it = m_bone_override.find(bone);
		BoneOverride props;
		if (it != m_bone_override.end()) {
			props = it->second;
			// Reset timer
			props.dtime_passed = 0;
			// Save previous values for interpolation
			props.position.previous = props.position.vector;
			props.rotation.previous = props.rotation.next;
			props.scale.previous = props.scale.vector;
		} else {
			// Disable interpolation
			props.position.interp_timer = 0.0f;
			props.rotation.interp_timer = 0.0f;
			props.scale.interp_timer = 0.0f;
		}
		// Read new values
		props.position.vector = readV3F32(is);
		props.rotation.next = core::quaternion(readV3F32(is) * core::DEGTORAD);
		props.scale.vector = readV3F32(is); // reads past end of string on older cmds
		if (is.eof()) {
			// Backwards compatibility
			props.scale.vector = v3f(1, 1, 1); // restore the scale which was not sent
			props.position.absolute = true;
			props.rotation.absolute = true;
		} else {
			props.position.interp_timer = readF32(is);
			props.rotation.interp_timer = readF32(is);
			props.scale.interp_timer = readF32(is);
			u8 absoluteFlag = readU8(is);
			props.position.absolute = (absoluteFlag & 1) > 0;
			props.rotation.absolute = (absoluteFlag & 2) > 0;
			props.scale.absolute = (absoluteFlag & 4) > 0;
		}
		if (props.isIdentity()) {
			m_bone_override.erase(bone);
		} else {
			m_bone_override[bone] = props;
		}
		// updateBones(); now called every step
	} else if (cmd == AO_CMD_ATTACH_TO) {
		u16 parent_id = readS16(is);
		std::string bone = deSerializeString16(is);
		v3f position = readV3F32(is);
		v3f rotation = readV3F32(is);
		bool force_visible = readU8(is); // Returns false for EOF

		setAttachment(parent_id, bone, position, rotation, force_visible);
	} else if (cmd == AO_CMD_PUNCHED) {
		u16 result_hp = readU16(is);

		// Use this instead of the send damage to not interfere with prediction
		s32 damage = (s32)m_hp - (s32)result_hp;

		m_hp = result_hp;

		if (m_is_local_player)
			m_env->getLocalPlayer()->hp = m_hp;

		if (damage > 0)
		{
			if (m_hp == 0)
			{
				// TODO: Execute defined fast response
				// As there is no definition, make a smoke puff
				ClientSimpleObject *simple = createSmokePuff(
						m_smgr, m_env, m_position,
						v2f(m_prop.visual_size.X, m_prop.visual_size.Y) * BS);
				m_env->addSimpleObject(simple);
			} else if (m_reset_textures_timer < 0 && !m_prop.damage_texture_modifier.empty()) {
				m_reset_textures_timer = 0.05;
				if(damage >= 2)
					m_reset_textures_timer += 0.05 * damage;
				// Cap damage overlay to 1 second
				m_reset_textures_timer = std::min(m_reset_textures_timer, 1.0f);
				updateTextures(m_current_texture_modifier + m_prop.damage_texture_modifier);
			}
		}

		if (m_hp == 0) {
			// Same as 'Server::DiePlayer'
			clearParentAttachment();
			// Same as 'ObjectRef::l_remove'
			if (!m_is_player)
				clearChildAttachments();
		}
	} else if (cmd == AO_CMD_UPDATE_ARMOR_GROUPS) {
		m_armor_groups.clear();
		int armor_groups_size = readU16(is);
		for(int i=0; i<armor_groups_size; i++)
		{
			std::string name = deSerializeString16(is);
			int rating = readS16(is);
			m_armor_groups[name] = rating;
		}
	} else if (cmd == AO_CMD_SPAWN_INFANT) {
		u16 child_id = readU16(is);
		u8 type = readU8(is); // maybe this will be useful later
		(void)type;

		addAttachmentChild(child_id);
	} else if (cmd == AO_CMD_OBSOLETE1) {
		// Don't do anything and also don't log a warning
	} else {
		warningstream << FUNCTION_NAME
			<< ": unknown command or outdated client \""
			<< +cmd << "\"" << std::endl;
	}
}

/* \pre punchitem != NULL
 */
bool GenericCAO::directReportPunch(v3f dir, const ItemStack *punchitem,
		float time_from_last_punch)
{
	assert(punchitem);	// pre-condition
	const ToolCapabilities *toolcap =
			&punchitem->getToolCapabilities(m_client->idef());
	PunchDamageResult result = getPunchDamage(
			m_armor_groups,
			toolcap,
			punchitem,
			time_from_last_punch,
			punchitem->wear);

	if(result.did_punch && result.damage != 0)
	{
		if(result.damage < m_hp)
		{
			m_hp -= result.damage;
		} else {
			m_hp = 0;
			// TODO: Execute defined fast response
			// As there is no definition, make a smoke puff
			ClientSimpleObject *simple = createSmokePuff(
					m_smgr, m_env, m_position,
					v2f(m_prop.visual_size.X, m_prop.visual_size.Y) * BS);
			m_env->addSimpleObject(simple);
		}
		if (m_reset_textures_timer < 0 && !m_prop.damage_texture_modifier.empty()) {
			m_reset_textures_timer = 0.05;
			if (result.damage >= 2)
				m_reset_textures_timer += 0.05 * result.damage;
			// Cap damage overlay to 1 second
			m_reset_textures_timer = std::min(m_reset_textures_timer, 1.0f);
			updateTextures(m_current_texture_modifier + m_prop.damage_texture_modifier);
		}
	}

	return false;
}

std::string GenericCAO::debugInfoText()
{
	std::ostringstream os(std::ios::binary);
	os<<"GenericCAO hp="<<m_hp<<"\n";
	os<<"armor={";
	for(ItemGroupList::const_iterator i = m_armor_groups.begin();
			i != m_armor_groups.end(); ++i)
	{
		os<<i->first<<"="<<i->second<<", ";
	}
	os<<"}";
	return os.str();
}

// Prototype
static GenericCAO proto_GenericCAO(nullptr, nullptr);
