#pragma once

#include "mapnode.h"

class NodeDefManager;
struct MeshMakeData;

extern const v3s16 light_dirs[8];

struct LightPair {
	u8 lightDay;
	u8 lightNight;
	f32 ambientOcclusion = 1.0f;

	LightPair() = default;
	explicit LightPair(u16 value) : lightDay(value & 0xff), lightNight(value >> 8) {}
	LightPair(u8 valueA, u8 valueB) : lightDay(valueA), lightNight(valueB) {}
	LightPair(float valueA, float valueB) :
		lightDay(core::clamp(core::round32(valueA), 0, 255)),
		lightNight(core::clamp(core::round32(valueB), 0, 255)) {}
	operator u16() const { return lightDay | lightNight << 8; }
};

struct LightInfo {
	f32 light_day;
	f32 light_night;
	f32 light_boosted;
	f32 ambient_occlusion = 1.0f;

	LightPair getPair(float sunlight_boost = 0.0) const
	{
		return LightPair(
			(1 - sunlight_boost) * light_day
			+ sunlight_boost * light_boosted,
			light_night);
	}
};

struct LightFrame {
	f32 lightsDay[8];
	f32 lightsNight[8];
	f32 ambientOcclusion[8] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
	bool sunlight[8];
};

void getSmoothLightFrame(LightFrame &lframe, const v3s16 &p, MeshMakeData *data);
LightInfo blendLight(const LightFrame &lframe, const v3f &vertex_pos);
video::SColor blendLightColor(const LightFrame &lframe, const v3f &vertex_pos, u8 light_source);
video::SColor blendLightColor(const LightFrame &lframe, const v3f &vertex_pos, const v3f &vertex_normal, u8 light_source);

/*!
 * Encodes light of a node.
 * The result is not the final color, but a
 * half-baked vertex color.
 * You have to multiply the resulting color
 * with the node's color.
 *
 * \param light the first 8 bits are day light,
 * the last 8 bits are night light
 * \param emissive_light amount of light the surface emits,
 * from 0 to LIGHT_SUN.
 */
video::SColor encode_light(u16 light, u8 emissive_light, f32 ambient_occlusion=1.0f);

// Compute light at node
u16 getInteriorLight(MapNode n, s32 increment, const NodeDefManager *ndef);
u16 getFaceLight(MapNode n, MapNode n2, const NodeDefManager *ndef);
u16 getSmoothLightSolid(const v3s16 &p, const v3s16 &face_dir, const v3s16 &corner, MeshMakeData *data, f32 &ambient_occlusion_f);
u16 getSmoothLightTransparent(const v3s16 &p, const v3s16 &corner, MeshMakeData *data, f32 &ambient_occlusion_f);
