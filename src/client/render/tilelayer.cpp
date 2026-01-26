// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "tilelayer.h"
#include "client/core/client.h"
#include "rendersystem.h"
#include "renderer.h"
#include "client/mesh/lighting.h"
#include "client/player/localplayer.h"
#include "client/player/playercamera.h"
#include "client/ao/animation.h"

/*!
 * Two layers are equal if they can be merged.
 */
bool TileLayer::operator==(const TileLayer &other) const
{
    return (
        alpha_discard == other.alpha_discard &&
        tile_ref == other.tile_ref &&
        shader == other.shader &&
        material_type == other.material_type &&
        material_flags == other.material_flags &&
        color == other.color &&
        scale == other.scale &&
        rotation == other.rotation &&
        emissive_light == other.emissive_light);
}

void TileLayer::setupRenderState(Client *client) const
{
    auto rndsys = client->getRenderSystem();
    auto rnd = rndsys->getRenderer();
    rnd->setRenderState(true);

    rnd->setClipRect(recti());

    auto ctxt = rnd->getContext();

    ctxt->enableCullFace(material_flags & MATERIAL_FLAG_BACKFACE_CULLING);
    ctxt->setCullMode(render::CM_BACK);
    ctxt->setLineWidth(line_thickness);

    if (!use_default_shader) {
        rnd->setBlending(material_flags & MATERIAL_FLAG_TRANSPARENT);
        rnd->setShader(shader);
        shader->setUniformInt("mAlphaDiscard", alpha_discard);
    }
    else {
        rnd->setDefaultShader(material_flags & MATERIAL_FLAG_TRANSPARENT, true);
        rnd->setDefaultUniforms(1.0f, alpha_discard, 0.5f, img::BM_COUNT);
    }

    if (tile_ref)
        rndsys->activateAtlas(tile_ref);
    for (u8 i = 0; i < textures.size(); i++)
        ctxt->setActiveUnit(i+1, textures.at(i));

    // Workaround
    if (shader && (thing == RenderThing::NODE || thing == RenderThing::OBJECT)) {
        u32 daynight_ratio = (f32)client->getEnv().getDayNightRatio();
        shader->setUniformFloat("mDayNightRatio", (f32)daynight_ratio);

        u32 animation_timer = client->getEnv().getFrameTime() % 1000000;
        f32 animation_timer_f = (f32)animation_timer / 100000.f;
        shader->setUniformFloat("mAnimationTimer", animation_timer_f);

        auto camera = client->getEnv().getLocalPlayer()->getCamera();
        v3f offset = intToFloat(camera->getOffset(), BS);
        shader->setUniform3Float("mCameraOffset", offset);

        if (thing == RenderThing::NODE) {
            v3f camera_position = camera->getPosition();
            shader->setUniform3Float("mCameraPosition", camera_position);
        }
        else {
            shader->setUniformInt("mBonesOffset", bone_offset);
            shader->setUniformInt("mAnimateNormals", (s32)animate_normals);

            auto bonetex = rndsys->getAnimationManager()->getBonesTexture();
            rnd->setDataTexParams(bonetex);
        }
    }
}
