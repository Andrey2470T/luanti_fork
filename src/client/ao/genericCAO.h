// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "clientActiveObject.h"
#include "object_properties.h"
#include "clientSimpleObject.h"
#include "constants.h"
#include "itemgroup.h"
#include <cassert>
#include <map>
#include <memory>

class Client;

/*
	SmoothTranslator
*/

template<typename T>
struct SmoothTranslator
{
	T val_old;
	T val_current;
	T val_target;
	f32 anim_time = 0;
	f32 anim_time_counter = 0;
	bool aim_is_end = true;

	SmoothTranslator() = default;

	void init(T current);

	void update(T new_target, bool is_end_position = false,
		float update_interval = -1);

	void translate(f32 dtime);
};

struct SmoothTranslatorWrapped : SmoothTranslator<f32>
{
	void translate(f32 dtime);
};

struct SmoothTranslatorWrappedv3f : SmoothTranslator<v3f>
{
	void translate(f32 dtime);
};

enum class TransformNodeType
{
    BONE,
    OBJECT
};

class TransformNodeTree;

struct TransformNode
{
    TransformNodeType Type;

    // Relative transforms
    v3f Position;
    Quaternion Rotation;
    v3f Scale {1.0f};

    matrix4 AbsoluteTransform;
    matrix4 RelativeTransform;

    std::optional<u8> Parent;

    std::vector<u8> Children;

    TransformNodeTree *Tree;

    TransformNode() = default;
    ~TransformNode();

    bool isRoot() const
    {
        return !Parent.has_value();
    }
    TransformNode *getParent() const;

    virtual void updateNode();

    void updateNodeAndChildren();
};

class TransformNodeTree
{
protected:
    std::vector<std::unique_ptr<TransformNode>> Nodes;

    std::vector<u8> RootNodes;

    u8 maxAcceptedNodeCount = 255;
public:
    TransformNodeTree() = default;

    u8 getNodesCount() const
    {
        return Nodes.size();
    }

    TransformNode *getNode(u8 id) const;

    void addNode(TransformNode *newNode);
    void removeNode(u8 id);

    void updateNodes();
};

struct Attachment : public TransformNode
{
    u16 ObjectId;
};

// CAO able moving in the space and being attached to another one
class GenericCAO : public ClientActiveObject
{
protected:
    // Only set at initialization
    std::string m_name = "";
    bool m_is_player = false;
    bool m_is_local_player = false;
    // Property-ish things
    ObjectProperties m_prop;

    Client *m_client = nullptr;

    v3f m_position = v3f(0.0f, 10.0f * BS, 0);
    v3f m_velocity;
    v3f m_acceleration;
    v3f m_rotation;
    u16 m_hp = 1;

    SmoothTranslator<v3f> pos_translator;
    SmoothTranslatorWrappedv3f rot_translator;

    object_t m_attachment_parent_id = 0;
    std::unordered_set<object_t> m_attachment_child_ids;
    std::string m_attachment_bone = "";
    v3f m_attachment_position;
    v3f m_attachment_rotation;
    bool m_attached_to_local = false;
    bool m_force_visible = false;

    ItemGroupList m_armor_groups;

    matrix4 m_abs_transform;
    matrix4 m_rel_transform;
public:
    GenericCAO(Client *client, ClientEnvironment *env);

    ~GenericCAO();

    static std::unique_ptr<ClientActiveObject> create(Client *client, ClientEnvironment *env)
    {
        return std::make_unique<GenericCAO>(client, env);
    }

    inline ActiveObjectType getType() const override
    {
        return ACTIVEOBJECT_TYPE_GENERIC;
    }
    inline const ItemGroupList &getGroups() const
    {
        return m_armor_groups;
    }
    void initialize(const std::string &data) override;

    void processInitData(const std::string &data);

    const v3f getPosition() const override final;

    const v3f getVelocity() const override final { return m_velocity; }

    inline const v3f &getRotation() const { return m_rotation; }

    bool isImmortal() const;

    inline const ObjectProperties &getProperties() const { return m_prop; }

    inline const std::string &getName() const { return m_name; }

    // m_matrixnode controls the position and rotation of the child node
    // for all scene nodes, as a workaround for an Irrlicht problem with
    // rotations. The child node's position can't be used because it's
    // rotated, and must remain as 0.
    // Note that m_matrixnode.setPosition() shouldn't be called. Use
    // m_matrixnode->getRelativeTransformationMatrix().setTranslation()
    // instead (aka getPosRotMatrix().setTranslation()).
    inline matrix4 &getPosRotMatrix()
    {
        return m_rel_transform;
    }

    inline const matrix4 &getAbsolutePosRotMatrix() const
    {
        return m_abs_transform;
    }

    inline bool isLocalPlayer() const override
    {
        return m_is_local_player;
    }

    inline bool isPlayer() const
    {
        return m_is_player;
    }

    void setAttachment(object_t parent_id, const std::string &bone, v3f position,
            v3f rotation, bool force_visible) override;
    void getAttachment(object_t *parent_id, std::string *bone, v3f *position,
            v3f *rotation, bool *force_visible) const override;
    void clearChildAttachments() override;
    void addAttachmentChild(object_t child_id) override;
    void removeAttachmentChild(object_t child_id) override;
    ClientActiveObject *getParent() const override;
    const std::unordered_set<object_t> &getAttachmentChildIds() const override
    { return m_attachment_child_ids; }
    void updateAttachments() override;

    void removeFromScene(bool permanent) override;

    void addToScene() override;

    void step(float dtime, ClientEnvironment *env) override;

    void processMessage(const std::string &data) override;

    bool directReportPunch(v3f dir, const ItemStack *punchitem=NULL,
            float time_from_last_punch=1000000) override;

    std::string debugInfoText() override;

    std::string infoText() override
    {
        return m_prop.infotext;
    }
};
