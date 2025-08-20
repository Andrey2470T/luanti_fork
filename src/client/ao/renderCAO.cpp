#include "renderCAO.h"
#include "client/client.h"
#include "client/player/playercamera.h"
#include "client/ui/minimap.h"
#include "itemgroup.h"
#include "client/render/rendersystem.h"
#include "client/media/resource.h"
#include "client/ao/transformNode.h"
#include "map.h"
#include "client/mesh/lighting.h"
#include "client/mesh/model.h"
#include "client/mesh/meshoperations.h"
#include "client/mesh/layeredmesh.h"
#include "client/ao/skeleton.h"
#include "client/ao/animation.h"
#include "collision.h"
#include "nodedef.h"
#include "client/sound/sound.h"
#include "client/render/batcher3d.h"
#include "client/render/atlas.h"
#include "client/mesh/defaultVertexTypes.h"

RenderCAO::RenderCAO(Client *client, ClientEnvironment *env)
    : GenericCAO(client, env), m_rndsys(client->getRenderSystem()), m_cache(client->getResourceCache()),
      m_anim_mgr(env->getAnimationManager())
{
    initTileLayer();

    /* don't update while punch texture modifier is active */
    if (m_reset_textures_timer < 0)
        updateAppearance(m_current_texture_modifier);

    /*if (auto shadow = RenderingEngine::get_shadow_renderer())
            shadow->addNodeToShadowList(node);
    }*/
}

RenderCAO::~RenderCAO()
{
    /*
    if (auto shadow = RenderingEngine::get_shadow_renderer())
        if (auto node = getSceneNode())
            shadow->removeNodeFromShadowList(node);*/

    removeMesh();

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

void RenderCAO::removeMesh()
{
    if (m_model)
        m_cache->clearResource<Model>(ResourceType::MODEL, m_model);

    m_model = nullptr;
}

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

void RenderCAO::step(float dtime, ClientEnvironment *env)
{
    // Handle model animations and update positions instantly to prevent lags

    // MOVE TO RENDER_CAO.H/CPP (OR PLAYER_CAO.H/CPP)

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

            auto anim = m_model->getAnimation();

            if (anim) {
                v2i set_range = anim->getRange();
                f32 set_speed = anim->getFPS();

                bool update = true;
                if ((new_anim.X + new_anim.Y) > 0 && !getParent()) {
                    allow_update = true;
                    set_range = v2i(new_anim.X, new_anim.Y);
                    set_speed = new_speed;
                    player->last_animation_speed = set_speed;
                } else {
                    player->last_animation = LocalPlayerAnimation::NO_ANIM;

                    if (old_anim != LocalPlayerAnimation::NO_ANIM) {
                        auto range = player->local_animations[0];
                        set_range = v2i(range.X, range.Y);
                    }
                }

                // Update local player animations
                if (!((player->last_animation != old_anim ||
                        set_speed != old_anim_speed) &&
                        player->last_animation != LocalPlayerAnimation::NO_ANIM &&
                        allow_update))
                    update = false;

                if (update)
                    setAnimation(set_range, set_speed, anim->getLooped());
            }

        }
    }

    if (m_visuals_expired) {
        m_visuals_expired = false;

        // Attachments, part 1: All attached objects must be unparented first,
        // or Irrlicht causes a segmentation fault
        /*for (u16 cao_id : m_attachment_child_ids) {
            ClientActiveObject *obj = m_env->getActiveObject(cao_id);
            if (obj) {
                scene::ISceneNode *child_node = obj->getSceneNode();
                // The node's parent is always an IDummyTraformationSceneNode,
                // so we need to reparent that one instead.
                if (child_node)
                    child_node->getParent()->setParent(m_smgr->getRootSceneNode());
            }
        }*/

        removeMesh();
        addMesh();

        // Attachments, part 2: Now that the parent has been refreshed, put its attachments back
        /*for (u16 cao_id : m_attachment_child_ids) {
            ClientActiveObject *obj = m_env->getActiveObject(cao_id);
            if (obj)
                obj->updateAttachments();
        }*/
    }

    // Make sure m_is_visible is always applied

    setVisible(m_is_visible);

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
        v3f lastpos = pos_translator.val_current;

        if(m_prop.physical)
        {
            aabbf box = m_prop.collisionbox;
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
        m_position += m_velocity * dtime + m_acceleration * 0.5f * dtime * dtime;
        m_velocity += m_acceleration * dtime;
        pos_translator.update(m_position, pos_translator.aim_is_end,
            pos_translator.anim_time);
        }
        pos_translator.translate(dtime);
        updateMatrices();

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

    /*m_anim_timer += dtime;
    if(m_anim_timer >= m_anim_framelength)
    {
        m_anim_timer -= m_anim_framelength;
        m_anim_frame++;
        if(m_anim_frame >= m_anim_num_frames)
            m_anim_frame = 0;
    }*/

    updateTexturePos();

    if(m_reset_textures_timer >= 0)
    {
        m_reset_textures_timer -= dtime;
        if(m_reset_textures_timer <= 0) {
            m_reset_textures_timer = -1;
            updateAppearance(m_previous_texture_modifier);
        }
    }

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
        updateMatrices();
    }

    updateMeshCulling();

    updateBones(dtime);
}


void RenderCAO::updateVertexColor(bool update_light)
{
    if (m_prop.glow >= 0 && update_light) {
        v3s16 pos[3];
        u16 npos = getLightPosition(pos);

        std::vector<v3s16> positions;

        for (u16 i = 0; i < npos; i++)
            positions[i] = pos[i];

        auto light = getLightColor( m_env->getMap(), m_client->ndef(), positions, m_prop.glow);

        if (light != m_last_light)
            m_last_light = light;

    }
    MeshOperations::colorizeMesh(m_model->getMesh()->getBuffer(0), m_base_color*m_last_light);
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
        if (m_marker.has_value())
            m_client->getMinimap()->removeMarker(m_marker.value());
        return;
    }

    if (m_marker)
        return;

    m_marker = getPosition();
    m_client->getMinimap()->addMarker(m_marker.value());
}

void RenderCAO::updateNametag()
{
    if (m_is_local_player) // No nametag for local player
        return;

    if (m_prop.nametag.empty() || m_prop.nametag_color.A() == 0) {
        // Delete nametag
        if (m_nametag) {
            m_client->getCamera()->removeNametag(m_nametag);
            m_nametag = nullptr;
        }
        return;
    }

    v3f pos;
    pos.Y = m_prop.selectionbox.MaxEdge.Y + 0.3f;
    if (!m_nametag) {
        // Add nametag
        m_nametag = m_client->getCamera()->addNametag(
            m_prop.nametag, m_prop.nametag_color,
            m_prop.nametag_bgcolor, pos);
    } else {
        // Update nametag
        m_nametag->text = m_prop.nametag;
        m_nametag->textcolor = m_prop.nametag_color;
        m_nametag->bgcolor = m_prop.nametag_bgcolor;
        m_nametag->updateBank(pos);
    }
}

/*void RenderCAO::updateTexturePos()
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
}*/

void RenderCAO::setAnimation(v2i range, f32 speed, bool loop)
{
    auto anim = m_model->getAnimation();

    if (!anim)
        return;

    // Note: This sets the current frame as well, (re)starting the animation.
    anim->setLooped(loop);
    anim->setFPS(speed);
    anim->setRange(range);
    //m_animated_meshnode->setTransitionTime(m_animation_blend);

    m_restart_anim = true;
}

void RenderCAO::setAnimationSpeed(f32 speed)
{
    auto anim = m_model->getAnimation();

    if (!anim)
        return;

    anim->setFPS(speed);

    m_restart_anim = true;
}

void RenderCAO::updateBones(f32 dtime)
{
    auto anim = m_model->getAnimation();

    if (!anim)
        return;

    if (!anim->isStarted() || m_restart_anim) {
        m_restart_anim = false;
        anim->start();
    }
    bool animated = anim->animateBones(dtime);

    if (animated)
        m_model->getSkeleton()->updateDataTexture();
}

bool RenderCAO::visualExpiryRequired(const ObjectProperties &newprops)
{
    const ObjectProperties &old = m_prop;
    /* Visuals do not need to be expired for:
     * - nametag props: handled by updateNametag()
     * - textures:      handled by updateTextures()
     * - sprite props:  handled by updateTexturePos()
     * - glow:          handled by updateLight()
     * - any other properties that do not change appearance
     */

    bool uses_legacy_texture = newprops.wield_item.empty() &&
        (newprops.visual == "wielditem" || newprops.visual == "item");
    // Ordered to compare primitive types before std::vectors
    return old.backface_culling != newprops.backface_culling ||
        old.is_visible != newprops.is_visible ||
        old.mesh != newprops.mesh ||
        old.shaded != newprops.shaded ||
        old.use_texture_alpha != newprops.use_texture_alpha ||
        old.visual != newprops.visual ||
        old.visual_size != newprops.visual_size ||
        old.wield_item != newprops.wield_item ||
        old.colors != newprops.colors ||
        (uses_legacy_texture && old.textures != newprops.textures);
}

void RenderCAO::initTileLayer()
{
    m_tile_layer.alpha_discard = 1;
    m_tile_layer.material_flags = MATERIAL_FLAG_TRANSPARENT;

    if (m_prop.shaded && m_prop.glow == 0)
        m_tile_layer.material_type = (m_prop.use_texture_alpha) ?
            TILE_MATERIAL_ALPHA : TILE_MATERIAL_BASIC;
    else
        m_tile_layer.material_type = (m_prop.use_texture_alpha) ?
            TILE_MATERIAL_PLAIN_ALPHA : TILE_MATERIAL_PLAIN;
    m_tile_layer.use_default_shader = false;

    std::string shadername = m_model->getSkeleton() ? "object_skinned" : "object";
    m_tile_layer.shader = m_cache->getOrLoad<render::Shader>(ResourceType::SHADER, shadername);
}

void RenderCAO::updateMeshCulling()
{
    if (!m_is_local_player)
        return;

    bool hidden = m_client->getCamera()->getCameraMode() == CAMERA_MODE_FIRST;

    setVisible(hidden);
}

void RenderCAO::processMessage(const std::string &data)
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
            aabbf collision_box = m_prop.collisionbox;
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
                    updateAppearance(m_current_texture_modifier);
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

        if(getParent()) // Just in case
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
    } else if (cmd == AO_CMD_SET_TEXTURE_MOD) {
        std::string mod = deSerializeString16(is);

        // immediately reset an engine issued texture modifier if a mod sends a different one
        if (m_reset_textures_timer > 0) {
            m_reset_textures_timer = -1;
            updateAppearance(m_previous_texture_modifier);
        }
        updateAppearance(mod);
    } else if (cmd == AO_CMD_SET_SPRITE) {
        /*v2s16 p = readV2S16(is);
        int num_frames = readU16(is);
        float framelength = readF32(is);
        bool select_horiz_by_yawpitch = readU8(is);

        m_tx_basepos = p;
        //m_anim_num_frames = num_frames;
        //m_anim_frame = 0;
        //m_anim_framelength = framelength;
        m_tx_select_horiz_by_yawpitch = select_horiz_by_yawpitch;

        updateTexturePos();*/
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
        f32 speed = readF32(is);
        f32 blend = readF32(is); // unused for now
        bool loop = !readU8(is);

        bool update = true;
        if (m_is_local_player) {
            LocalPlayer *player = m_env->getLocalPlayer();
            // update animation only if local animations present
            // and received animation is unknown (except idle animation)
            bool is_known = false;

            auto anim = m_model->getAnimation();

            if (anim) {
                for (int i = 1;i<4;i++)
                {
                    if(anim->getRange().Y == player->local_animations[i].Y)
                        is_known = true;
                }
            }
            if(!(!is_known ||
                    (player->local_animations[1].Y + player->local_animations[2].Y < 1)))
            {
                update = false;
            }
            // FIXME: ^ This code is trash. It's also broken.
        }

        if (update)
            setAnimation(v2i(range.X, range.Y), speed, loop);
    } else if (cmd == AO_CMD_SET_ANIMATION_SPEED) {
        f32 speed = readF32(is);
        setAnimationSpeed(speed);
    } else if (cmd == AO_CMD_SET_BONE_POSITION) {
        /*std::string bone = deSerializeString16(is);
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
        }*/
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
                /*ClientSimpleObject *simple = createSmokePuff(
                        m_smgr, m_env, m_position,
                        v2f(m_prop.visual_size.X, m_prop.visual_size.Y) * BS);
                m_env->addSimpleObject(simple);*/
            } else if (m_reset_textures_timer < 0 && !m_prop.damage_texture_modifier.empty()) {
                m_reset_textures_timer = 0.05;
                if(damage >= 2)
                    m_reset_textures_timer += 0.05 * damage;
                // Cap damage overlay to 1 second
                m_reset_textures_timer = std::min(m_reset_textures_timer, 1.0f);
                updateAppearance(m_current_texture_modifier + m_prop.damage_texture_modifier);
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
bool RenderCAO::directReportPunch(v3f dir, const ItemStack *punchitem,
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
            /*ClientSimpleObject *simple = createSmokePuff(
                    m_smgr, m_env, m_position,
                    v2f(m_prop.visual_size.X, m_prop.visual_size.Y) * BS);
            m_env->addSimpleObject(simple);*/
        }
        if (m_reset_textures_timer < 0 && !m_prop.damage_texture_modifier.empty()) {
            m_reset_textures_timer = 0.05;
            if (result.damage >= 2)
                m_reset_textures_timer += 0.05 * result.damage;
            // Cap damage overlay to 1 second
            m_reset_textures_timer = std::min(m_reset_textures_timer, 1.0f);
            updateAppearance(m_current_texture_modifier + m_prop.damage_texture_modifier);
        }
    }

    return false;
}

std::string RenderCAO::debugInfoText()
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

void SpriteRenderCAO::addMesh()
{
    MeshBuffer *sprite = new MeshBuffer(4, 6);
    Batcher3D::appendUnitFace(sprite, {});

    MeshOperations::scaleMesh(sprite, m_prop.visual_size * BS);

    LayeredMeshPart mesh_p;
    mesh_p.count = 6;

    MeshLayer mesh_l;
    mesh_l.first = std::make_shared<TileLayer>(m_tile_layer);
    mesh_l.second = mesh_p;
    m_model = new Model(m_position, {mesh_l}, sprite);
    m_cache->cacheResource<Model>(ResourceType::MODEL, m_model);
}

void RenderCAO::updateLayerUVs(std::string new_texture, u8 layer_id)
{
    auto img = m_cache->getOrLoad<img::Image>(ResourceType::IMAGE, new_texture);

    if (!img)
        return;

    auto basic_pool = m_rndsys->getPool(true);
    auto buffer = m_model->getMesh()->getBuffer(0);
    auto layer = m_model->getMesh()->getBufferLayer(0, layer_id);

    if (layer.first->tile_ref) {
        auto prev_atlas = basic_pool->getAtlasByTile(layer.first->tile_ref, true);
        u32 prev_atlas_res = prev_atlas->getTextureSize();
        auto prev_tile = basic_pool->getTileRect(layer.first->tile_ref);

        for (u32 k = layer.second.offset; k < layer.second.count; k++) {
            v2f cur_uv = svtGetUV(buffer, k);

            v2u pixel_coords;
            pixel_coords.X = cur_uv.X * prev_atlas_res;
            pixel_coords.Y = cur_uv.Y * prev_atlas_res;

            pixel_coords -= v2u(prev_tile.ULC.X, prev_tile.ULC.Y);

            v2f texture_uv;
            texture_uv.X = pixel_coords.X / prev_tile.getWidth();
            texture_uv.Y = pixel_coords.Y / prev_tile.getHeight();

            svtSetUV(buffer, texture_uv, k);
        }
    }

    auto atlas = basic_pool->getAtlasByTile(img, true);
    layer.first->atlas = atlas;
    layer.first->tile_ref = img;

    u32 atlas_res = atlas->getTextureSize();
    rectf r = basic_pool->getTileRect(img, false, true);

    for (u32 k = layer.second.offset; k < layer.second.count; k++) {
        v2f cur_uv = svtGetUV(buffer, k);

        u32 rel_x = round32(cur_uv.X * r.getWidth());
        u32 rel_y = round32(cur_uv.Y * r.getHeight());

        v2f atlas_uv(
            (r.ULC.X + rel_x) / atlas_res,
            (r.ULC.Y + rel_y) / atlas_res
        );

        svtSetUV(buffer, atlas_uv, k);
    }
}

void SpriteRenderCAO::updateAppearance(std::string mod)
{
    m_previous_texture_modifier = m_current_texture_modifier;
    m_current_texture_modifier = mod;

    std::string texturestring = "no_texture.png";
    if (!m_prop.textures.empty())
        texturestring = m_prop.textures[0];
    texturestring += mod;

    updateLayerUVs(texturestring, 0);
}

void UprightSpriteRenderCAO::addMesh()
{
    MeshBuffer *sprite = new MeshBuffer(4, 6);
    Batcher3D::appendUnitFace(sprite, {});

    MeshOperations::scaleMesh(sprite, m_prop.visual_size * BS);

    if (m_is_player) {
        f32 dy = BS * m_prop.visual_size.Y / 2;
        // Move minimal Y position to 0 (feet position)
        MeshOperations::translateMesh(sprite, v3f(0.0f, dy, 0.0f));
    }

    LayeredMeshPart mesh_p;
    mesh_p.count = 6;

    MeshLayer mesh_l;
    mesh_l.first = std::make_shared<TileLayer>(m_tile_layer);
    mesh_l.second = mesh_p;
    m_model = new Model(m_position, {mesh_l}, sprite);
    m_cache->cacheResource<Model>(ResourceType::MODEL, m_model);
}

void UprightSpriteRenderCAO::updateAppearance(std::string mod)
{
    m_previous_texture_modifier = m_current_texture_modifier;
    m_current_texture_modifier = mod;

    std::string tname1 = "no_texture.png";
    std::string tname2 = "no_texture.png";

    if (!m_prop.textures.empty()) {
        tname1 = m_prop.textures[0];
        tname2 = m_prop.textures[1];
    }
    tname1 += mod;
    tname2 += mod;

    updateLayerUVs(tname1, 0);
    updateLayerUVs(tname2, 1);

    // Set mesh color (only if lighting is disabled)
    if (!m_prop.colors.empty() && m_prop.glow < 0) {
        m_base_color = m_prop.colors[0];
        updateVertexColor(false);
    }
}

void CubeRenderCAO::addMesh()
{
    MeshBuffer *cube = new MeshBuffer(4 * 6, 6 * 6);
    Batcher3D::appendUnitBox(cube, {});

    MeshOperations::scaleMesh(cube, m_prop.visual_size * BS);

    LayeredMeshPart mesh_p;
    mesh_p.count = 6 * 6;

    MeshLayer mesh_l;
    mesh_l.first = std::make_shared<TileLayer>(m_tile_layer);
    mesh_l.second = mesh_p;
    m_model = new Model(m_position, {mesh_l}, cube);
    m_cache->cacheResource<Model>(ResourceType::MODEL, m_model);
}
void CubeRenderCAO::updateAppearance(std::string mod)
{
    for (u32 i = 0; i < 6; ++i)
    {
        std::string texturestring = "no_texture.png";
        if(m_prop.textures.size() > i)
            texturestring = m_prop.textures[i];
        texturestring += mod;

        updateLayerUVs(texturestring, i);
    }
}

void MeshRenderCAO::addMesh()
{
    std::vector<std::shared_ptr<TileLayer>> layers;

    for (u8 i = 0; i < 6; i++)
        layers.push_back(std::make_shared<TileLayer>(m_tile_layer));
    m_model = Model::load(m_anim_mgr, m_position, layers, m_prop.mesh, m_cache);

    if (m_model) {
        auto mesh = m_model->getMesh();

        for (u8 i = 0; i < mesh->getBuffersCount(); i++) {
            auto buffer = mesh->getBuffer(i);

            if (!MeshOperations::checkMeshNormals(buffer)) {
                infostream << "MeshRenderCAO: recalculating normals for mesh "
                    << m_prop.mesh << std::endl;
                MeshOperations::recalculateNormals(buffer, true, false);
            }

            MeshOperations::scaleMesh(buffer, m_prop.visual_size);
        }
    }
    m_base_color = img::white;
    updateVertexColor(false);
    m_cache->cacheResource<Model>(ResourceType::MODEL, m_model);
}

void MeshRenderCAO::updateAppearance(std::string mod)
{
    for (u32 i = 0; i < m_model->getMesh()->getBufferLayersCount(0); ++i) {
        std::string texturestring = m_prop.textures[i];
        if (texturestring.empty())
            continue; // Empty texture string means don't modify that material
        texturestring += mod;

        updateLayerUVs(texturestring, i);

        auto layer = m_model->getMesh()->getBufferLayer(0, i);

        if (m_prop.backface_culling)
            layer.first->material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
        else
            layer.first->material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
    }
}

void WieldItemRenderCAO::addMesh()
{
    /*if (m_prop.visual == "wielditem" || m_prop.visual == "item") {
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
}
