// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "clientActiveObject.h"
#include "object_properties.h"
#include "transformNode.h"
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

enum class GenericCAOType
{
    DUMMY = 0,
    RENDER_SPRITE,
    RENDER_UPRIGHT_SPRITE,
    RENDER_CUBE,
    RENDER_MESH,
    RENDER_WIELDITEM,
    RENDER_TEST
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

    GenericCAOType m_gcao_type;

    v3f m_position = v3f(0.0f, 10.0f * BS, 0);
    v3f m_velocity;
    v3f m_acceleration;
    v3f m_rotation;
    u16 m_hp = 1;

    SmoothTranslator<v3f> pos_translator;
    SmoothTranslatorWrappedv3f rot_translator;

    TransformNodeManager *m_node_mgr = nullptr;

    // Attachment transform node of this GenericCAO
    std::optional<u8> m_attachment_tree_id;
    std::optional<u8> m_attachment_node_id;

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

    ActiveObjectType getType() const override
    {
        return ACTIVEOBJECT_TYPE_GENERIC;
    }
    GenericCAOType getGenericCAOType() const
    {
        return m_gcao_type;
    }
    const ItemGroupList &getGroups() const
    {
        return m_armor_groups;
    }
    void initialize(const std::string &data) override;

    void processInitData(const std::string &data);

    const v3f getPosition() const override final;

    const v3f getVelocity() const override final { return m_velocity; }

    //const v3f &getRotation() const { return m_rotation; }
    const v3f getRotation() const;

    bool isImmortal() const;

    const ObjectProperties &getProperties() const { return m_prop; }

    const std::string &getName() const { return m_name; }

    matrix4 &getRelativeMatrix();

    const matrix4 &getAbsoluteMatrix() const;

    bool isLocalPlayer() const override
    {
        return m_is_local_player;
    }

    bool isPlayer() const
    {
        return m_is_player;
    }

    Attachment *getAttachmentNode() const;

    void setAttachment(object_t parent_id, const std::string &bone, v3f position,
            v3f rotation, bool force_visible) override;
    void getAttachment(object_t *parent_id, std::string *bone, v3f *position,
            v3f *rotation, bool *force_visible) const override;
    void clearChildAttachments() override;
    void addAttachmentChild(object_t child_id) override;
    void removeAttachmentChild(object_t child_id) override;
    GenericCAO *getParent() const override;
    const std::unordered_set<object_t> &getAttachmentChildIds() const override;
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
