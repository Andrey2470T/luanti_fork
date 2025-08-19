#include "renderCAO.h"
#include "client/client.h"
#include "client/player/playercamera.h"
#include "client/ui/minimap.h"
#include "itemgroup.h"
#include "client/render/rendersystem.h"
#include "client/media/resource.h"
#include "client/ao/transformNode.h"

RenderCAO::RenderCAO(Client *client, ClientEnvironment *env)
    : GenericCAO(client, env), m_rndsys(client->getRenderSystem()), m_cache(client->getResourceCache())
{}

RenderCAO::~RenderCAO()
{
    /*
    if (auto shadow = RenderingEngine::get_shadow_renderer())
        if (auto node = getSceneNode())
            shadow->removeNodeFromShadowList(node);*/

    if (m_model)
        m_cache->clearResource<Model>(ResourceType::MODEL, m_model);

    if (m_nametag) {
        m_client->getCamera()->removeNametag(m_nametag);
        m_nametag = nullptr;
    }

    if (m_marker.has_value() && m_client->getMinimap())
        m_client->getMinimap()->removeMarker(m_marker.value());
}

bool RenderCAO::getCollisionBox(aabbf *toset) const
{
    if (m_prop.physical)
    {
        //update collision box
        toset->MinEdge = m_prop.collisionbox.MinEdge * BS;
        toset->MaxEdge = m_prop.collisionbox.MaxEdge * BS;

        toset->MinEdge += m_position;
        toset->MaxEdge += m_position;

        return true;
    }

    return false;
}

bool RenderCAO::collideWithObjects() const
{
    return m_prop.collideWithObjects;
}

bool RenderCAO::getSelectionBox(aabbf *toset) const
{
    if (!m_prop.is_visible || !m_is_visible || m_is_local_player) {
        return false;
    }
    *toset = m_selection_box;
    return true;
}

void RenderCAO::setChildrenVisible(bool toset)
{
    auto children = getAttachmentChildIds();

    for (u16 child_id : children) {
        auto cao = dynamic_cast<RenderCAO *>(m_env->getGenericCAO(child_id));

        if (!cao)
            continue;

        auto attach_node = cao->getAttachmentNode();
        // Check if the entity is forced to appear in first person.
        cao->setVisible(attach_node->ForceVisible ? true : toset);
    }
}

//void GenericCAO::addToScene()
//{
    //m_visuals_expired = false;

    //if (!m_prop.is_visible)
    //	return;

    //infostream << "GenericCAO::addToScene(): " << m_prop.visual << std::endl;

    /*m_material_type_param = 0.5f; // May cut off alpha < 128 depending on m_material_type

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
    }*/

    /* Set VBO hint */
    // wieldmesh sets its own hint, no need to handle it
    /*if (m_meshnode || m_animated_meshnode) {
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
    }*/

    /* don't update while punch texture modifier is active */
    //if (m_reset_textures_timer < 0)
    //	updateTextures(m_current_texture_modifier);

    /*if (scene::ISceneNode *node = getSceneNode()) {
        if (m_matrixnode)
            node->setParent(m_matrixnode);

        if (auto shadow = RenderingEngine::get_shadow_renderer())
            shadow->addNodeToShadowList(node);
    }*/

    //updateNametag();
    //updateMarker();
    //updateNodePos();
    //updateAnimation();
    //updateBones(.0f);
    //updateAttachments();
    //setNodeLight(m_last_light);
    //updateMeshCulling();

    /*if (m_animated_meshnode) {
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
    }*/
//}

/*static void setBillboardTextureMatrix(scene::IBillboardSceneNode *bill,
        float txs, float tys, int col, int row)
{
    video::SMaterial& material = bill->getMaterial(0);
    core::matrix4& matrix = material.getTextureMatrix(0);
    matrix.setTextureTranslate(txs*col, tys*row);
    matrix.setTextureScale(txs, tys);
}*/

/*static void setMeshBufferTextureCoords(scene::IMeshBuffer *buf, const v2f *uv, u32 count)
{
    assert(buf->getVertexType() == video::EVT_STANDARD);
    assert(buf->getVertexCount() == count);
    auto *vertices = static_cast<video::S3DVertex *>(buf->getVertices());
    for (u32 i = 0; i < count; i++)
        vertices[i].TCoords = uv[i];
    buf->setDirty(scene::EBT_VERTEX);
}*/

void RenderCAO::setAttachment(object_t parent_id, const std::string &bone,
        v3f position, v3f rotation, bool force_visible)
{
    GenericCAO::setAttachment(parent_id, bone, position, rotation, force_visible);

    auto node = getAttachmentNode();
    // Forcibly show attachments if required by set_attach
    if (node->ForceVisible) {
        m_is_visible = true;
    } else if (!m_is_local_player) {
        // Objects attached to the local player should be hidden in first person
        m_is_visible = !node->AttachedToLocal ||
            m_client->getCamera()->getCameraMode() != CAMERA_MODE_FIRST;
        node->ForceVisible = false;
    } else {
        // Local players need to have this set,
        // otherwise first person attachments fail.
        m_is_visible = true;
    }
}

void GenericCAO::step(float dtime, ClientEnvironment *env)
{
    // Handle model animations and update positions instantly to prevent lags

    // MOVE TO RENDER_CAO.H/CPP (OR PLAYER_CAO.H/CPP)

    /*if (m_is_local_player) {
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
    }*/

    /*if (m_visuals_expired && m_smgr) {
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
        node->setVisible(m_is_visible);*/

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
        //v3f lastpos = pos_translator.val_current;

        /*if(m_prop.physical)
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
        } else {*/
        m_position += m_velocity * dtime + m_acceleration * 0.5f * dtime * dtime;
        m_velocity += m_acceleration * dtime;
        pos_translator.update(m_position, pos_translator.aim_is_end,
            pos_translator.anim_time);
        //}
        pos_translator.translate(dtime);
        //updateNodePos();
        updateMatrices();

        /*float moved = lastpos.getDistanceFrom(pos_translator.val_current);
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
        }*/
    }

    /*m_anim_timer += dtime;
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
    }*/

    if (std::abs(m_prop.automatic_rotate) > 0.001f) {
        // This is the child node's rotation. It is only used for automatic_rotate.
        //v3f local_rot = node->getRotation();
        rot_translator.val_current.Y = modulo360f(rot_translator.val_current.Y - dtime * RADTODEG *
                m_prop.automatic_rotate);
        updateMatrices();
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
        //updateNodePos();
        updateMatrices();
    }

    /*if (m_animated_meshnode) {
        // Everything must be updated; the whole transform
        // chain as well as the animated mesh node.
        // Otherwise, bone attachments would be relative to
        // a position that's one frame old.
        if (m_matrixnode)
            updatePositionRecursive(m_matrixnode);
        m_animated_meshnode->updateAbsolutePosition();
        m_animated_meshnode->animateJoints();
        updateBones(dtime);
    }*/
}


void RenderCAO::updateLight(u32 day_night_ratio)
{
    if (m_prop.glow < 0)
        return;

    u16 light_at_pos = 0;
    u8 light_at_pos_intensity = 0;
    bool pos_ok = false;

    v3s16 pos[3];
    u16 npos = getLightPosition(pos);
    for (u16 i = 0; i < npos; i++) {
        bool this_ok;
        MapNode n = m_env->getMap().getNode(pos[i], &this_ok);
        if (this_ok) {
            // Get light level at the position plus the entity glow
            u16 this_light = getInteriorLight(n, m_prop.glow, m_client->ndef());
            u8 this_light_intensity = MYMAX(this_light & 0xFF, this_light >> 8);
            if (this_light_intensity > light_at_pos_intensity) {
                light_at_pos = this_light;
                light_at_pos_intensity = this_light_intensity;
            }
            pos_ok = true;
        }
    }
    if (!pos_ok)
        light_at_pos = LIGHT_SUN;

    // Initialize with full alpha, otherwise entity won't be visible
    video::SColor light{0xFFFFFFFF};

    // Encode light into color, adding a small boost
    // based on the entity glow.
    light = encode_light(light_at_pos, m_prop.glow);

    if (light != m_last_light) {
        m_last_light = light;
        setNodeLight(light);
    }
}

void RenderCAO::setNodeLight(const video::SColor &light_color)
{
    if (m_prop.visual == "wielditem" || m_prop.visual == "item") {
        if (m_wield_meshnode)
            m_wield_meshnode->setNodeLightColor(light_color);
        return;
    }

    {
        auto *node = getSceneNode();
        if (!node)
            return;
        setColorParam(node, light_color);
    }
}

u16 RenderCAO::getLightPosition(v3s16 *pos)
{
    const auto &box = m_prop.collisionbox;
    pos[0] = floatToInt(m_position + box.MinEdge * BS, BS);
    pos[1] = floatToInt(m_position + box.MaxEdge * BS, BS);

    // Skip center pos if it falls into the same node as Min or MaxEdge
    if ((box.MaxEdge - box.MinEdge).getLengthSQ() < 3.0f)
        return 2;
    pos[2] = floatToInt(m_position + box.getCenter() * BS, BS);
    return 3;
}

void RenderCAO::updateMarker()
{
    if (!m_client->getMinimap())
        return;

    if (!m_prop.show_on_minimap) {
        if (m_marker)
            m_client->getMinimap()->removeMarker(&m_marker);
        return;
    }

    if (m_marker)
        return;

    scene::ISceneNode *node = getSceneNode();
    if (!node)
        return;
    m_marker = m_client->getMinimap()->addMarker(node);
}

void RenderCAO::updateNametag()
{
    if (m_is_local_player) // No nametag for local player
        return;

    if (m_prop.nametag.empty() || m_prop.nametag_color.getAlpha() == 0) {
        // Delete nametag
        if (m_nametag) {
            m_client->getCamera()->removeNametag(m_nametag);
            m_nametag = nullptr;
        }
        return;
    }

    scene::ISceneNode *node = getSceneNode();
    if (!node)
        return;

    v3f pos;
    pos.Y = m_prop.selectionbox.MaxEdge.Y + 0.3f;
    if (!m_nametag) {
        // Add nametag
        m_nametag = m_client->getCamera()->addNametag(node,
            m_prop.nametag, m_prop.nametag_color,
            m_prop.nametag_bgcolor, pos);
    } else {
        // Update nametag
        m_nametag->text = m_prop.nametag;
        m_nametag->textcolor = m_prop.nametag_color;
        m_nametag->bgcolor = m_prop.nametag_bgcolor;
        m_nametag->pos = pos;
    }
}

void RenderCAO::updateTexturePos()
{
    if(m_spritenode)
    {
        scene::ICameraSceneNode* camera =
                m_spritenode->getSceneManager()->getActiveCamera();
        if(!camera)
            return;
        v3f cam_to_entity = m_spritenode->getAbsolutePosition()
                - camera->getAbsolutePosition();
        cam_to_entity.normalize();

        int row = m_tx_basepos.Y;
        int col = m_tx_basepos.X;

        // Yawpitch goes rightwards
        if (m_tx_select_horiz_by_yawpitch) {
            if (cam_to_entity.Y > 0.75)
                col += 5;
            else if (cam_to_entity.Y < -0.75)
                col += 4;
            else {
                float mob_dir =
                        atan2(cam_to_entity.Z, cam_to_entity.X) / M_PI * 180.;
                float dir = mob_dir - m_rotation.Y;
                dir = wrapDegrees_180(dir);
                if (std::fabs(wrapDegrees_180(dir - 0)) <= 45.1f)
                    col += 2;
                else if(std::fabs(wrapDegrees_180(dir - 90)) <= 45.1f)
                    col += 3;
                else if(std::fabs(wrapDegrees_180(dir - 180)) <= 45.1f)
                    col += 0;
                else if(std::fabs(wrapDegrees_180(dir + 90)) <= 45.1f)
                    col += 1;
                else
                    col += 4;
            }
        }

        // Animation goes downwards
        row += m_anim_frame;

        float txs = m_tx_size.X;
        float tys = m_tx_size.Y;
        setBillboardTextureMatrix(m_spritenode, txs, tys, col, row);
    }

    else if (m_meshnode) {
        if (m_prop.visual == "upright_sprite") {
            int row = m_tx_basepos.Y;
            int col = m_tx_basepos.X;

            // Animation goes downwards
            row += m_anim_frame;

            const auto &tx = m_tx_size;
            v2f t[4] = { // cf. vertices in GenericCAO::addToScene()
                tx * v2f(col+1, row+1),
                tx * v2f(col, row+1),
                tx * v2f(col, row),
                tx * v2f(col+1, row),
            };
            auto mesh = m_meshnode->getMesh();
            setMeshBufferTextureCoords(mesh->getMeshBuffer(0), t, 4);
            setMeshBufferTextureCoords(mesh->getMeshBuffer(1), t, 4);
        }
    }
}

// Do not pass by reference, see header.
void RenderCAO::updateTextures(std::string mod)
{
    ITextureSource *tsrc = m_client->tsrc();

    bool use_trilinear_filter = g_settings->getBool("trilinear_filter");
    bool use_bilinear_filter = g_settings->getBool("bilinear_filter");
    bool use_anisotropic_filter = g_settings->getBool("anisotropic_filter");

    m_previous_texture_modifier = m_current_texture_modifier;
    m_current_texture_modifier = mod;

    if (m_spritenode) {
        if (m_prop.visual == "sprite") {
            std::string texturestring = "no_texture.png";
            if (!m_prop.textures.empty())
                texturestring = m_prop.textures[0];
            texturestring += mod;

            video::SMaterial &material = m_spritenode->getMaterial(0);
            material.MaterialType = m_material_type;
            material.MaterialTypeParam = m_material_type_param;
            material.setTexture(0, tsrc->getTextureForMesh(texturestring));

            material.forEachTexture([=] (auto &tex) {
                setMaterialFilters(tex, use_bilinear_filter, use_trilinear_filter,
                        use_anisotropic_filter);
            });
        }
    }

    else if (m_animated_meshnode) {
        if (m_prop.visual == "mesh") {
            for (u32 i = 0; i < m_animated_meshnode->getMaterialCount(); ++i) {
                const auto texture_idx = m_animated_meshnode->getMesh()->getTextureSlot(i);
                if (texture_idx >= m_prop.textures.size())
                    continue;
                std::string texturestring = m_prop.textures[texture_idx];
                if (texturestring.empty())
                    continue; // Empty texture string means don't modify that material
                texturestring += mod;
                video::ITexture *texture = tsrc->getTextureForMesh(texturestring);
                if (!texture) {
                    errorstream<<"GenericCAO::updateTextures(): Could not load texture "<<texturestring<<std::endl;
                    continue;
                }

                // Set material flags and texture
                video::SMaterial &material = m_animated_meshnode->getMaterial(i);
                material.MaterialType = m_material_type;
                material.MaterialTypeParam = m_material_type_param;
                material.TextureLayers[0].Texture = texture;
                material.BackfaceCulling = m_prop.backface_culling;

                // don't filter low-res textures, makes them look blurry
                // player models have a res of 64
                const core::dimension2d<u32> &size = texture->getOriginalSize();
                const u32 res = std::min(size.Height, size.Width);
                use_trilinear_filter &= res > 64;
                use_bilinear_filter &= res > 64;

                material.forEachTexture([=] (auto &tex) {
                    setMaterialFilters(tex, use_bilinear_filter, use_trilinear_filter,
                            use_anisotropic_filter);
                });
            }
        }
    }

    else if (m_meshnode) {
        if(m_prop.visual == "cube")
        {
            for (u32 i = 0; i < 6; ++i)
            {
                std::string texturestring = "no_texture.png";
                if(m_prop.textures.size() > i)
                    texturestring = m_prop.textures[i];
                texturestring += mod;

                // Set material flags and texture
                video::SMaterial &material = m_meshnode->getMaterial(i);
                material.MaterialType = m_material_type;
                material.MaterialTypeParam = m_material_type_param;
                material.setTexture(0, tsrc->getTextureForMesh(texturestring));
                material.getTextureMatrix(0).makeIdentity();

                material.forEachTexture([=] (auto &tex) {
                    setMaterialFilters(tex, use_bilinear_filter, use_trilinear_filter,
                            use_anisotropic_filter);
                });
            }
        } else if (m_prop.visual == "upright_sprite") {
            scene::IMesh *mesh = m_meshnode->getMesh();
            {
                std::string tname = "no_texture.png";
                if (!m_prop.textures.empty())
                    tname = m_prop.textures[0];
                tname += mod;

                auto &material = m_meshnode->getMaterial(0);
                material.setTexture(0, tsrc->getTextureForMesh(tname));

                material.forEachTexture([=] (auto &tex) {
                    setMaterialFilters(tex, use_bilinear_filter, use_trilinear_filter,
                            use_anisotropic_filter);
                });
            }
            {
                std::string tname = "no_texture.png";
                if (m_prop.textures.size() >= 2)
                    tname = m_prop.textures[1];
                else if (!m_prop.textures.empty())
                    tname = m_prop.textures[0];
                tname += mod;

                auto &material = m_meshnode->getMaterial(1);
                material.setTexture(0, tsrc->getTextureForMesh(tname));

                material.forEachTexture([=] (auto &tex) {
                    setMaterialFilters(tex, use_bilinear_filter, use_trilinear_filter,
                            use_anisotropic_filter);
                });
            }
            // Set mesh color (only if lighting is disabled)
            if (!m_prop.colors.empty() && m_prop.glow < 0)
                setMeshColor(mesh, m_prop.colors[0]);
        }
    }
    // Prevent showing the player after changing texture
    if (m_is_local_player)
        updateMeshCulling();
}

void RenderCAO::updateAnimation()
{
    if (!m_animated_meshnode)
        return;

    // Note: This sets the current frame as well, (re)starting the animation.
    m_animated_meshnode->setFrameLoop(m_animation_range.X, m_animation_range.Y);
    if (m_animated_meshnode->getAnimationSpeed() != m_animation_speed)
        m_animated_meshnode->setAnimationSpeed(m_animation_speed);
    m_animated_meshnode->setTransitionTime(m_animation_blend);
    if (m_animated_meshnode->getLoopMode() != m_animation_loop)
        m_animated_meshnode->setLoopMode(m_animation_loop);
}

void RenderCAO::updateAnimationSpeed()
{
    if (!m_animated_meshnode)
        return;

    m_animated_meshnode->setAnimationSpeed(m_animation_speed);
}

void RenderCAO::updateBones(f32 dtime)
{
    if (!m_animated_meshnode)
        return;
    if (m_bone_override.empty()) {
        m_animated_meshnode->setJointMode(scene::EJUOR_NONE);
        return;
    }

    m_animated_meshnode->setJointMode(scene::EJUOR_CONTROL); // To write positions to the mesh on render
    for (auto &it : m_bone_override) {
        std::string bone_name = it.first;
        scene::IBoneSceneNode* bone = m_animated_meshnode->getJointNode(bone_name.c_str());
        if (!bone)
            continue;

        BoneOverride &props = it.second;
        props.dtime_passed += dtime;

        bone->setPosition(props.getPosition(bone->getPosition()));
        bone->setRotation(props.getRotationEulerDeg(bone->getRotation()));
        bone->setScale(props.getScale(bone->getScale()));
    }

    // search through bones to find mistakenly rotated bones due to bug in Irrlicht
    for (u32 i = 0; i < m_animated_meshnode->getJointCount(); ++i) {
        scene::IBoneSceneNode *bone = m_animated_meshnode->getJointNode(i);
        if (!bone)
            continue;

        //If bone is manually positioned there is no need to perform the bug check
        bool skip = false;
        for (auto &it : m_bone_override) {
            if (it.first == bone->getName()) {
                skip = true;
                break;
            }
        }
        if (skip)
            continue;

        // Workaround for Irrlicht bug
        // We check each bone to see if it has been rotated ~180deg from its expected position due to a bug in Irricht
        // when using EJUOR_CONTROL joint control. If the bug is detected we update the bone to the proper position
        // and update the bones transformation.
        v3f bone_rot = bone->getRelativeTransformation().getRotationDegrees();
        float offset = fabsf(bone_rot.X - bone->getRotation().X);
        if (offset > 179.9f && offset < 180.1f) {
            bone->setRotation(bone_rot);
            bone->updateAbsolutePosition();
        }
    }
    // The following is needed for set_bone_pos to propagate to
    // attached objects correctly.
    // Irrlicht ought to do this, but doesn't when using EJUOR_CONTROL.
    for (u32 i = 0; i < m_animated_meshnode->getJointCount(); ++i) {
        auto bone = m_animated_meshnode->getJointNode(i);
        // Look for the root bone.
        if (bone && bone->getParent() == m_animated_meshnode) {
            // Update entire skeleton.
            bone->updateAbsolutePositionOfAllChildren();
            break;
        }
    }
}

bool RenderCAO::visualExpiryRequired(const ObjectProperties &new_) const
{
    const ObjectProperties &old = m_prop;
    /* Visuals do not need to be expired for:
     * - nametag props: handled by updateNametag()
     * - textures:      handled by updateTextures()
     * - sprite props:  handled by updateTexturePos()
     * - glow:          handled by updateLight()
     * - any other properties that do not change appearance
     */

    bool uses_legacy_texture = new_.wield_item.empty() &&
        (new_.visual == "wielditem" || new_.visual == "item");
    // Ordered to compare primitive types before std::vectors
    return old.backface_culling != new_.backface_culling ||
        old.is_visible != new_.is_visible ||
        old.mesh != new_.mesh ||
        old.shaded != new_.shaded ||
        old.use_texture_alpha != new_.use_texture_alpha ||
        old.visual != new_.visual ||
        old.visual_size != new_.visual_size ||
        old.wield_item != new_.wield_item ||
        old.colors != new_.colors ||
        (uses_legacy_texture && old.textures != new_.textures);
}

void RenderCAO::updateMeshCulling()
{
    if (!m_is_local_player)
        return;

    const bool hidden = m_client->getCamera()->getCameraMode() == CAMERA_MODE_FIRST;

    scene::ISceneNode *node = getSceneNode();

    if (!node)
        return;

    if (m_prop.visual == "upright_sprite") {
        // upright sprite has no backface culling
        node->forEachMaterial([=] (auto &mat) {
            mat.FrontfaceCulling = hidden;
        });
        return;
    }

    if (hidden) {
        // Hide the mesh by culling both front and
        // back faces. Serious hackyness but it works for our
        // purposes. This also preserves the skeletal armature.
        node->forEachMaterial([] (auto &mat) {
            mat.BackfaceCulling = true;
            mat.FrontfaceCulling = true;
        });
    } else {
        // Restore mesh visibility.
        node->forEachMaterial([this] (auto &mat) {
            mat.BackfaceCulling = m_prop.backface_culling;
            mat.FrontfaceCulling = false;
        });
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
        //bool expire_visuals = visualExpiryRequired(newprops);
        //bool textures_changed = m_prop.textures != newprops.textures;

        // Apply changes
        m_prop = std::move(newprops);

        /*m_selection_box = m_prop.selectionbox;
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
        }*/
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
        //updateNodePos();
        updateMatrices();
    } /*else if (cmd == AO_CMD_SET_TEXTURE_MOD) {
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
    }*/ else if (cmd == AO_CMD_SET_PHYSICS_OVERRIDE) {
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
    } /*else if (cmd == AO_CMD_SET_ANIMATION) {
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
    } */else if (cmd == AO_CMD_ATTACH_TO) {
        u16 parent_id = readS16(is);
        std::string bone = deSerializeString16(is);
        v3f position = readV3F32(is);
        v3f rotation = readV3F32(is);
        bool force_visible = readU8(is); // Returns false for EOF

        setAttachment(parent_id, bone, position, rotation, force_visible);
    } /*else if (cmd == AO_CMD_PUNCHED) {
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
    } */else if (cmd == AO_CMD_SPAWN_INFANT) {
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
/*bool GenericCAO::directReportPunch(v3f dir, const ItemStack *punchitem,
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
}*/
