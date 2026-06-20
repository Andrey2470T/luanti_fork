// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "irr_v2d.h"
#include "irrlichttypes.h"
#include "tileanimation.h"
#include <Video/Texture.h>
#include <vector>
#include <Video/SMaterial.h>

enum MaterialType : u8 {
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

bool isTransparentLayer(MaterialType type);

// Material flags
// Should backface culling be enabled?
#define MATERIAL_FLAG_BACKFACE_CULLING 0x01
// Should a crack be drawn?
#define MATERIAL_FLAG_CRACK 0x02
// Should the crack be drawn on transparent pixels (unset) or not (set)?
// Ignored if MATERIAL_FLAG_CRACK is not set.
#define MATERIAL_FLAG_CRACK_OVERLAY 0x04
#define MATERIAL_FLAG_ANIMATION 0x08
//#define MATERIAL_FLAG_HIGHLIGHTED 0x10
#define MATERIAL_FLAG_TILEABLE_HORIZONTAL 0x20
#define MATERIAL_FLAG_TILEABLE_VERTICAL 0x40

// Stores information for drawing an animated tile
struct AnimationInfo
{
	AnimationInfo() = default;
	AnimationInfo(const TileAnimationParams &params, v2u32 img_size)
	{
		params.getFrames(&m_frames_rects, &m_frame_length_ms, img_size);
	};
	AnimationInfo(const AnimationInfo &other)
	{
		m_frames_rects = other.m_frames_rects;
		m_frame_length_ms = other.m_frame_length_ms;
	}

	bool operator==(const AnimationInfo &other) const
	{
		return (m_frame_length_ms == other.m_frame_length_ms &&
			m_frames_rects.size() == other.m_frames_rects.size());
	}
	bool updateFrame(f32 animation_time);
	core::rect<u32> getCurrentFrameRect() const
	{
		return m_frames_rects.at(m_frame);
	}

	size_t hash() const {
		size_t seed = 0;
		seed ^= std::hash<u16>{}(m_frame_length_ms) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<size_t>{}(m_frames_rects.size()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
private:
	u16 m_frame = 0; // last animation frame
	u16 m_frame_length_ms = 0;
	std::vector<core::rect<u32>> m_frames_rects;
};

/**
 * We have two tile layers:
 * layer 0 = base
 * layer 1 = overlay
 */
#define MAX_TILE_LAYERS 2

//! Defines a layer of a tile.
struct TileLayer
{
	TileLayer() = default;

	/*!
	 * Two layers are equal if they can be merged.
	 */
	bool operator==(const TileLayer &other) const
	{
		return
			texture_id == other.texture_id &&
			shader_id == other.shader_id &&
			anim_info == other.anim_info &&
			material_type == other.material_type &&
			material_flags == other.material_flags &&
			has_color == other.has_color &&
			color == other.color &&
			scale == other.scale &&
			need_polygon_offset == other.need_polygon_offset &&
			override_material == other.override_material;
	}

	/*!
	 * Two tiles are not equal if they must have different vertices.
	 */
	bool operator!=(const TileLayer &other) const
	{
		return !(*this == other);
	}

	/**
	 * Set some material parameters accordingly.
	 * @note does not set `MaterialType`
	 * @param material material to mody
	 * @param layer index of this layer in the `TileSpec`
	 */
	void applyMaterialOptions(video::SMaterial &material, int layer) const;

	/// @return is this layer uninitalized?
	bool empty() const
	{
		return !shader_id && !texture_id;
	}

	/// @return is this layer semi-transparent?
	bool isTransparent() const
	{
		return isTransparentLayer(material_type);
	}

	// Ordered for size, please do not reorder

	video::GLTexture *texture = nullptr;

	u32 shader_id = 0;

	u32 texture_id = 0;

	AnimationInfo anim_info;

	MaterialType material_type = TILE_MATERIAL_BASIC;
	u8 material_flags =
		//0 // <- DEBUG, Use the one below
		MATERIAL_FLAG_BACKFACE_CULLING |
		MATERIAL_FLAG_TILEABLE_HORIZONTAL|
		MATERIAL_FLAG_TILEABLE_VERTICAL;

	u8 scale = 1;

	/// does this tile need to have a positive polygon offset set?
	/// @see TileLayer::applyMaterialOptions
	bool need_polygon_offset = false;

	/*!
	 * The color of the tile, or if the tile does not own
	 * a color then the color of the node owning this tile.
	 */
	video::SColor color = 0xffffffff;

	//! If true, the tile has its own color.
	bool has_color = false;

	// Name of custom material with which the mapblock meshes will be overrided
	std::string override_material = "";
};

enum class TileRotation: u8 {
	None,
	R90,
	R180,
	R270,
};

/*!
 * Defines a face of a node. May have up to two layers.
 */
struct TileSpec
{
	TileSpec() = default;

	//! If true, the tile rotation is ignored.
	bool world_aligned = false;
	//! Tile rotation.
	TileRotation rotation = TileRotation::None;
	//! This much light does the tile emit.
	u8 emissive_light = 0;
	//! The first is base texture, the second is overlay.
	TileLayer layers[MAX_TILE_LAYERS];
};
