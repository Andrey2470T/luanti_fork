// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "genericCAO.h"
#include "renderCAO.h"
#include "client/client.h"
#include "util/numeric.h"
#include "util/serialize.h"
#include "client/player/localplayer.h"
#include <algorithm>
#include <cmath>
#include <Utils/Quaternion.h>
#include "client/mesh/model.h"
#include "client/mesh/layeredmesh.h"
#include "client/ao/skeleton.h"

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
    updateMatrices();
}

GenericCAO::~GenericCAO()
{
    removeAttachmentNode();
}

const v3f GenericCAO::getPosition() const
{
	if (!getParent())
		return pos_translator.val_current;
    else {
        v3s16 camera_offset = m_env->getCameraOffset();
        return getAttachmentNode()->getAbsolutePosition() + intToFloat(camera_offset, BS);
    }

	return m_position;
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

void GenericCAO::updateMatrices()
{
    v3f pos = pos_translator.val_current - intToFloat(m_env->getCameraOffset(), BS);
    v3f rot = m_is_local_player ? -m_rotation : -rot_translator.val_current;

    auto node = getAttachmentNode();
    if (!node) {
        m_rel_transform.setTranslation(pos);
        setPitchYawRoll(m_rel_transform, rot);

        m_abs_transform = m_rel_transform;
    }
    else {
        node->Position = pos;

        Quaternion rot_q;
        rot_q.fromEuler(rot);
        node->Rotation = rot_q;

        node->updateNodeAndChildren();
    }
}

Attachment *GenericCAO::getAttachmentNode() const
{
    if (!m_attachment_tree_id.has_value() || !m_attachment_node_id.has_value())
        return nullptr;

    return dynamic_cast<Attachment *>(m_node_mgr->getNode(
        m_attachment_tree_id.value(), m_attachment_node_id.value()));
}

void GenericCAO::removeAttachmentNode()
{
    auto node = getAttachmentNode();

    if (!node)
        return;

    auto tree = node->Tree;
    u8 tree_id = m_attachment_tree_id.value();

    for (u8 child_id : node->Children) {
        auto child_node = dynamic_cast<Attachment *>(tree->getNode(child_id));

        if (!child_node)
            continue;

        auto child = m_env->getGenericCAO(child_node->ObjectId);
        child->m_attachment_tree_id = std::nullopt;
        child->m_attachment_node_id = std::nullopt;
    }
    tree->removeNode(m_attachment_node_id.value());

    m_attachment_tree_id = std::nullopt;
    m_attachment_node_id = std::nullopt;

    if (tree->getNodesCount() == 0)
        m_node_mgr->removeNodeTree(tree_id);
}

void GenericCAO::setAttachment(object_t parent_id, const std::string &bone,
		v3f position, v3f rotation, bool force_visible)
{
    auto cur_parent = getParent();

    Attachment *cur_node;

    // As this class already doesn't manage the bone animations,
    // defer the bone attachment to the RenderCAO's setAttachment
    bool attachment_changed = cur_parent && (cur_parent->getId() != parent_id
        || getAttachmentNode()->AttachedToBone != bone);

    if (!cur_parent || attachment_changed) {
        if (attachment_changed)
            removeAttachmentNode();

        cur_node = new Attachment();
        cur_node->ObjectId = m_id;
    }
    else
        cur_node = getAttachmentNode();

    auto new_parent = m_env->getGenericCAO(parent_id);
    if (attachment_changed) {
        TransformNodeTree *cur_tree;
        TransformNode *parent_node;
        if (bone.empty()) {
            TransformNodeTree *cur_tree;
            auto attach_node = new_parent->getAttachmentNode();
            parent_node = attach_node;

            if (!attach_node) {
                cur_tree = new TransformNodeTree();

                attach_node = new Attachment();
                attach_node->ObjectId = parent_id;
                attach_node->Position = new_parent->getPosition();

                v3f euler_rot = new_parent->getRotation();
                Quaternion rot;
                rot.fromEuler(euler_rot);
                attach_node->Rotation = rot;
                attach_node->Tree = cur_tree;

                parent_node = attach_node;

                cur_tree->addNode(attach_node);
                m_node_mgr->addNodeTree(cur_tree);

                new_parent->m_attachment_tree_id = m_node_mgr->getNodeTreeCount()-1;
                new_parent->m_attachment_node_id = 0;
            }
            else
                cur_tree = attach_node->Tree;
        }
        else {
            auto rendercao = dynamic_cast<RenderCAO *>(new_parent);

            if (rendercao) {
                auto skeleton = rendercao->getModel()->getSkeleton();
                auto bones = skeleton->getAllBones();

                auto found_bone = std::find(bones.begin(), bones.end(),
                [bone] (const Bone *cur_bone)
                {
                    return bone == cur_bone->Name;
                });

                if (found_bone != bones.end()) {
                    cur_tree = found_bone->Tree;
                    parent_node = found_bone;
                }
            }
        }

        if (cur_tree && parent_node) {
            cur_tree->addNode(cur_node, new_parent->m_attachment_node_id);
            parent_node->updateNodeAndChildren();

            m_attachment_tree_id = new_parent->m_attachment_tree_id;
            m_attachment_node_id = cur_tree->getNodesCount()-1;
        }
    }

    cur_node->Position = position;

    Quaternion rot;
    rot.fromEuler(rotation);
    cur_node->Rotation = rot;
    cur_node->AttachedToBone = bone;
    cur_node->ForceVisible = force_visible;
    cur_node->AttachedToLocal = new_parent->isLocalPlayer();
}

void GenericCAO::getAttachment(object_t *parent_id, std::string *bone, v3f *position,
	v3f *rotation, bool *force_visible) const
{
    auto cur_node = getAttachmentNode();

    if (cur_node) {
        *parent_id = getParent()->getId();
        *bone = cur_node->AttachedToBone;
        *position = cur_node->Position;
        cur_node->Rotation.toEuler(*rotation);
        *force_visible = cur_node->ForceVisible;
    }
}

void GenericCAO::clearChildAttachments()
{
    auto node = getAttachmentNode();

    if (!node)
        return;

    for (u8 id : node->Children) {
        auto child_node = dynamic_cast<Attachment *>(node->Tree->getNode(id));

        if (!child_node)
            continue;
        auto child = m_env->getGenericCAO(child_node->ObjectId);

        child->removeAttachmentNode();
    }
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

const std::unordered_set<u16> GenericCAO::getAttachmentChildIds() const
{
    std::unordered_set<u16> childs_ids;

    auto node = getAttachmentNode();
    if (!node)
        return childs_ids;

    for (u8 id : node->Children) {
        auto child_node = node->Tree->getNode(id);

        if (child_node->Type == TransformNodeType::OBJECT)
            childs_ids.emplace(dynamic_cast<Attachment *>(child_node)->ObjectId);
    }
}

void GenericCAO::step(float dtime, ClientEnvironment *env)
{
    if(getParent()) // Attachments should be glued to their parent by Irrlicht
	{
		// Set these for later
		m_position = getPosition();
		m_velocity = v3f(0,0,0);
		m_acceleration = v3f(0,0,0);
		pos_translator.val_current = m_position;
        pos_translator.val_end = m_position;
	} else {
		rot_translator.translate(dtime);

        m_position += m_velocity * dtime + m_acceleration * 0.5f * dtime * dtime;
        m_velocity += m_acceleration * dtime;
        pos_translator.update(m_position, pos_translator.aim_is_end,
            pos_translator.anim_time);
		pos_translator.translate(dtime);
	}

    if (std::abs(m_prop.automatic_rotate) > 0.001f) {
		// This is the child node's rotation. It is only used for automatic_rotate.
        //v3f local_rot = node->getRotation();
        rot_translator.val_current.Y = modulo360f(rot_translator.val_current.Y - dtime * RADTODEG *
				m_prop.automatic_rotate);
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
	}

    updateMatrices();
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

		// Apply changes
		m_prop = std::move(newprops);
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
        updateMatrices();
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
    } else if (cmd == AO_CMD_ATTACH_TO) {
		u16 parent_id = readS16(is);
		std::string bone = deSerializeString16(is);
		v3f position = readV3F32(is);
		v3f rotation = readV3F32(is);
		bool force_visible = readU8(is); // Returns false for EOF

		setAttachment(parent_id, bone, position, rotation, force_visible);
    } // ???
    /*else if (cmd == AO_CMD_SPAWN_INFANT) {
		u16 child_id = readU16(is);
		u8 type = readU8(is); // maybe this will be useful later
		(void)type;

		addAttachmentChild(child_id);
    }*/ else if (cmd == AO_CMD_OBSOLETE1) {
		// Don't do anything and also don't log a warning
	} else {
		warningstream << FUNCTION_NAME
			<< ": unknown command or outdated client \""
			<< +cmd << "\"" << std::endl;
	}
}

// Prototype
static GenericCAO proto_GenericCAO(nullptr, nullptr);
