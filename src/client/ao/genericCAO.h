// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "clientActiveObject.h"
#include "object_properties.h"
#include "transformNode.h"
#include "constants.h"
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
    T val_start;
	T val_current;
    T val_end;
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

struct Attachment : public TransformNode
{
    u16 ObjectId;
    std::string AttachedToBone = "";
    bool AttachedToLocal = false;
    bool ForceVisible = false;

    Attachment()
    {
        Type = TransformNodeType::OBJECT;
    }
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

    matrix4 m_abs_transform;
    matrix4 m_rel_transform;
public:
    TransformNodeManager *m_node_mgr = nullptr;

    // Attachment transform node of this GenericCAO
    std::optional<u8> m_attachment_tree_id;
    std::optional<u8> m_attachment_node_id;

    GenericCAO(Client *client, ClientEnvironment *env);

    ~GenericCAO();

    ActiveObjectType getType() const override
    {
        return ACTIVEOBJECT_TYPE_GENERIC;
    }

    void initialize(const std::string &data) override;

    virtual void processInitData(const std::string &data);

    const v3f getPosition() const override final;

    const v3f getVelocity() const override final { return m_velocity; }

    const v3f getRotation() const { return m_rotation; };

    bool isImmortal() const;

    const ObjectProperties &getProperties() const { return m_prop; }

    const std::string &getName() const { return m_name; }

    matrix4 &getRelativeMatrix();

    const matrix4 &getAbsoluteMatrix() const;

    virtual void updateMatrices();

    bool isLocalPlayer() const override
    {
        return m_is_local_player;
    }

    bool isPlayer() const
    {
        return m_is_player;
    }

    Attachment *getAttachmentNode() const;
    void removeAttachmentNode();

    void setAttachment(object_t parent_id, const std::string &bone, v3f position,
            v3f rotation, bool force_visible) override;
    void getAttachment(object_t *parent_id, std::string *bone, v3f *position,
            v3f *rotation, bool *force_visible) const override;
    void clearChildAttachments() override;
    GenericCAO *getParent() const;
    const std::unordered_set<u16> getAttachmentChildIds() const override;

    void step(float dtime, ClientEnvironment *env) override;

    void processMessage(const std::string &data) override;
};
