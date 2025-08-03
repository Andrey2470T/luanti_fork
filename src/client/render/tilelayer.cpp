// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "tilelayer.h"
#include "rendersystem.h"
#include "renderer.h"
#include "atlas.h"

/*!
 * Two layers are equal if they can be merged.
 */
bool TileLayer::operator==(const TileLayer &other) const
{
    return
        alpha_discard == other.alpha_discard &&
        tile_ref == other.tile_ref &&
        shader == other.shader &&
        material_type == other.material_type &&
        material_flags == other.material_flags &&
        color == other.color &&
        scale == other.scale &&
        rotation == other.rotation &&
        emissive_light == other.emissive_light;
}

void TileLayer::setupRenderState(RenderSystem *rndsys) const
{
    auto rnd = rndsys->getRenderer();
    rnd->setRenderState(true);

    auto ctxt = rnd->getContext();

    ctxt->enableCullFace(material_flags & MATERIAL_FLAG_BACKFACE_CULLING);

    rnd->setBlending(material_flags & MATERIAL_FLAG_TRANSPARENT);

    auto basicPool = rndsys->getPool(true);
    rnd->setTexture(basicPool->getAtlasByTile(tile_ref)->getTexture());

    ctxt->setShader(shader);
    shader->setUniformInt("mAlphaDiscard", alpha_discard);
}
