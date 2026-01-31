// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include <Render/Texture.h>
#include <Render/Shader.h>
#include <Image/Image.h>

enum MaterialType{
	TILE_MATERIAL_BASIC,
	TILE_MATERIAL_ALPHA,
	TILE_MATERIAL_LIQUID_TRANSPARENT,
	TILE_MATERIAL_LIQUID_OPAQUE,
	TILE_MATERIAL_WAVING_LEAVES,
	TILE_MATERIAL_WAVING_PLANTS,
	TILE_MATERIAL_OPAQUE,
	TILE_MATERIAL_WAVING_LIQUID_BASIC,
	TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT,
	TILE_MATERIAL_WAVING_LIQUID_OPAQUE,
	// Note: PLAIN isn't a material actually used by tiles, rather just entities.
	TILE_MATERIAL_PLAIN,
	TILE_MATERIAL_PLAIN_ALPHA
};

// Material flags
#define MATERIAL_FLAG_BACKFACE_CULLING 0x01
#define MATERIAL_FLAG_TRANSPARENT 0x02
#define MATERIAL_FLAG_HARDWARE_COLORIZED 0x04
#define MATERIAL_FLAG_ANIMATION 0x08
#define MATERIAL_FLAG_OVERLAY_LAYER 0x10
#define MATERIAL_FLAG_WORLD_ALIGNED 0x20

enum class TileRotation: u8 {
    RNone,
	R90,
	R180,
	R270,
};

enum class RenderThing : u8 {
    NODE,
    OBJECT,
    BOX
};

class Client;
class Atlas;

//! Defines a layer of a tile.
struct TileLayer
{
	TileLayer() = default;

	/*!
	 * Two layers are equal if they can be merged.
	 */
    bool operator==(const TileLayer &other) const;

	/*!
	 * Two tiles are not equal if they must have different vertices.
	 */
	bool operator!=(const TileLayer &other) const
	{
		return !(*this == other);
	}

    void setupRenderState(Client *client) const;

    // '0' = alpha discard, '1' = alpha discard ref, '2' = no discard (solid)
    s32 alpha_discard = 2;

    img::Image *tile_ref = nullptr;

    render::Shader *shader = nullptr;

    bool use_default_shader = true;

    // Additional textures after the first atlas one
    std::vector<render::Texture *> textures;

	u8 material_type = TILE_MATERIAL_BASIC;
	u8 material_flags =
		//0 // <- DEBUG, Use the one below
        MATERIAL_FLAG_BACKFACE_CULLING;
        //MATERIAL_FLAG_TILEABLE_HORIZONTAL
        //MATERIAL_FLAG_TILEABLE_VERTICAL;

	/*!
	 * The color of the tile, or if the tile does not own
	 * a color then the color of the node owning this tile.
	 */
    img::color8 color = img::black;

    u32 animation_frame_length_ms = 0;
    u32 animation_frame_count = 1;

    /*!
     * Used if tile.world_aligned = true
     */
	u8 scale = 1;

    f32 line_thickness = 1.0f;

	//! Tile rotation.
    TileRotation rotation = TileRotation::RNone;
	//! This much light does the tile emit.
	u8 emissive_light = 0;

    //! NOTE: this is a dirty workaround to handle uniforms setting in the drawlist
    //! When the custom materials support is done, it will be removed
    RenderThing thing;

    s32 bone_offset;
    s32 animate_normals;
};
