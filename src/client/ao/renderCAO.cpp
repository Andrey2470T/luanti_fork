#include "renderCAO.h"

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
    for (object_t cao_id : m_attachment_child_ids) {
        GenericCAO *obj = m_env->getGenericCAO(cao_id);
        if (obj) {
            // Check if the entity is forced to appear in first person.
            obj->setVisible(obj->m_force_visible ? true : toset);
        }
    }
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
