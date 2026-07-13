#include "lighting.h"
#include "client/mesh/mapblock_mesh.h"
#include "client/render/floodfill.h"
#include "constants.h"
#include "mesh.h"
#include "settings.h"
#include "util/numeric.h"

// Distance of light extrapolation (for oversized nodes)
// After this distance, it gives up and considers light level constant
#define SMOOTH_LIGHTING_OVERSIZE 1.0

// Maps light index to corner direction
const v3s16 light_dirs[8] = {
	v3s16(-1, -1, -1),
	v3s16(-1, -1,  1),
	v3s16(-1,  1, -1),
	v3s16(-1,  1,  1),
	v3s16( 1, -1, -1),
	v3s16( 1, -1,  1),
	v3s16( 1,  1, -1),
	v3s16( 1,  1,  1),
};

// Gets the base lighting values for a node
void getSmoothLightFrame(LightFrame &lframe, const v3s16 &p, MeshMakeData *data)
{
	for (int k = 0; k < 8; ++k)
		lframe.sunlight[k] = false;
	for (int k = 0; k < 8; ++k) {
		LightPair light = getSmoothLightTransparent(p, light_dirs[k], data);

		lframe.lightsDay[k] = light.lightDay;
		lframe.lightsNight[k] = light.lightNight;
		lframe.blockLights[k] = light.blockLight;
		lframe.ambientOcclusion[k] = light.ambientOcclusion;
		// If there is direct sunlight and no ambient occlusion at some corner,
		// mark the vertical edge (top and bottom corners) containing it.
		if (light.lightDay * light.ambientOcclusion == 255) {
			lframe.sunlight[k] = true;
			lframe.sunlight[k ^ 2] = true;
		}
	}
}

// Calculates vertex light level
//  vertex_pos - vertex position in the node (coordinates are clamped to [0.0, 1.0] or so)
LightInfo blendLight(const LightFrame &lframe, const v3f &vertex_pos)
{
	// Light levels at (logical) node corners are known. Here,
	// trilinear interpolation is used to calculate light level
	// at a given point in the node.
	f32 x = core::clamp(vertex_pos.X / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
	f32 y = core::clamp(vertex_pos.Y / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
	f32 z = core::clamp(vertex_pos.Z / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
	f32 lightDay = 0.0; // daylight
	f32 lightNight = 0.0;
	f32 lightBoosted = 0.0; // daylight + direct sunlight, if any
	u16 blockLightRed = 0;
	u16 blockLightGreen = 0;
	u16 blockLightBlue = 0;
	f32 ambientOcclusion = 0.0;

	for (int k = 0; k < 8; ++k) {
		f32 dx = (k & 4) ? x : 1 - x;
		f32 dy = (k & 2) ? y : 1 - y;
		f32 dz = (k & 1) ? z : 1 - z;
		// Use direct sunlight (255), if any; use daylight otherwise.
		f32 light_boosted = lframe.sunlight[k] ? 255 : lframe.lightsDay[k];
		lightDay += dx * dy * dz * lframe.lightsDay[k];
		lightNight += dx * dy * dz * lframe.lightsNight[k];
		lightBoosted += dx * dy * dz * light_boosted;
		blockLightRed += (u16)(dx * dy * dz * (lframe.blockLights[k] >> 10 & 0x1f)) & 0x1f;
		blockLightGreen += (u16)(dx * dy * dz * (lframe.blockLights[k] >> 5 & 0x1f)) & 0x1f;
		blockLightBlue += (u16)(dx * dy * dz * (lframe.blockLights[k] & 0x1f)) & 0x1f;
		ambientOcclusion += dx * dy * dz * lframe.ambientOcclusion[k];
	}

	u16 blockLight = blockLightRed << 10 | blockLightGreen << 5 | blockLightBlue;
	return LightInfo{
		std::min({lightDay, 255.0f}), std::min({lightNight, 255.0f}),
		std::min({lightBoosted, 255.0f}), blockLight,
		std::min({ambientOcclusion, 1.0f})
	};
}

// Calculates vertex color to be used in mapblock mesh
//  vertex_pos - vertex position in the node (coordinates are clamped to [0.0, 1.0] or so)
video::SColor blendLightColor(
	const LightFrame &lframe, const v3f &vertex_pos, u8 light_source, u16 &block_light)
{
	LightInfo light = blendLight(lframe, vertex_pos);
	block_light = light.block_light;
	return encode_light(light.getPair(), light_source, light.ambient_occlusion);
}

video::SColor blendLightColor(
	const LightFrame &lframe, const v3f &vertex_pos,
	const v3f &vertex_normal, u8 light_source, u16 &block_light)
{
	LightInfo light = blendLight(lframe, vertex_pos);
	block_light = light.block_light;
	video::SColor color = encode_light(light.getPair(MYMAX(0.0f, vertex_normal.Y)),
		light_source, light.ambient_occlusion);
	if (!light_source)
		applyFacesShading(color, vertex_normal);
	return color;
}

video::SColor encode_light(u16 light, u8 emissive_light, f32 ambient_occlusion)
{
	u16 skyLight = light & 0xff;
	u16 blockLight = (light >> 8);
	emissive_light = decode_light(emissive_light);
	blockLight += emissive_light;
	blockLight = std::min<u16>(blockLight, 255);

	return video::SColor(emissive_light, skyLight, blockLight, ambient_occlusion*255);
}

void encode_block_light(f32 &light_f, u16 light)
{
	u32 light_32 = light;
	std::memcpy(&light_f, &light_32, sizeof(light_f));
}

video::SColor encode_material_light(u8 skyLight, u16 blockLight, u8 emissive_light)
{
    emissive_light = decode_light(emissive_light);
	u8 blockLightRed = ((blockLight >> 10) + emissive_light) & 0x1f;
	u8 blockLightGreen = ((blockLight >> 5 & 0x1f) + emissive_light) & 0x1f;
	u8 blockLightBlue = ((blockLight & 0x1f) + emissive_light) & 0x1f;

	return video::SColor(skyLight, blockLightRed, blockLightGreen, blockLightBlue);
}

/*
	Calculate non-smooth lighting at interior of node.
	Single light bank.
*/
static u8 getInteriorLight(enum LightBank bank, MapNode n, s32 increment,
	const NodeDefManager *ndef)
{
	u8 light = n.getLight(bank, ndef->getLightingFlags(n));
	light = rangelim(light + increment, 0, LIGHT_SUN);
	return decode_light(light);
}

/*
	Calculate non-smooth lighting at interior of node.
	Both light banks.
*/
u16 getInteriorLight(MapNode n, s32 increment, const NodeDefManager *ndef)
{
	u16 day = getInteriorLight(LIGHTBANK_DAY, n, increment, ndef);
	u16 night = getInteriorLight(LIGHTBANK_NIGHT, n, increment, ndef);
	return day | (night << 8);
}

/*
	Calculate non-smooth lighting at face of node.
	Single light bank.
*/
static u8 getFaceLight(enum LightBank bank, MapNode n, MapNode n2, const NodeDefManager *ndef)
{
	ContentLightingFlags f1 = ndef->getLightingFlags(n);
	ContentLightingFlags f2 = ndef->getLightingFlags(n2);

	u8 light;
	u8 l1 = n.getLight(bank, f1);
	u8 l2 = n2.getLight(bank, f2);
	if(l1 > l2)
		light = l1;
	else
		light = l2;

	// Boost light level for light sources
	u8 light_source = MYMAX(f1.light_source, f2.light_source);
	if(light_source > light)
		light = light_source;

	return decode_light(light);
}

/*
	Calculate non-smooth lighting at face of node.
	Both light banks.
*/
u16 getFaceLight(MapNode n, MapNode n2, const NodeDefManager *ndef)
{
	u16 day = getFaceLight(LIGHTBANK_DAY, n, n2, ndef);
	u16 night = getFaceLight(LIGHTBANK_NIGHT, n, n2, ndef);
	return day | (night << 8);
}

/*
	Calculate smooth lighting at the XYZ- corner of p.
	Both light banks
*/
static LightPair getSmoothLightCombined(const v3s16 &p,
	const std::array<v3s16,8> &dirs, MeshMakeData *data)
{
	const NodeDefManager *ndef = data->m_nodedef;

	u16 ambient_occlusion = 0;
	u8 light_source_max = 0;
	u8 light_day_max = 0;
	u8 light_night_max = 0;
	u16 block_light_max = 0;

	auto add_node = [&] (u8 i, bool obstructed = false) -> bool {
		if (obstructed) {
			ambient_occlusion++;
			return false;
		}
		v3s16 pos(p + dirs[i]);
		MapNode n = data->m_vmanip.getNodeNoExNoEmerge(pos);
		if (n.getContent() == CONTENT_IGNORE)
			return true;
		const ContentFeatures &f = ndef->get(n);
		light_source_max = std::max(light_source_max, f.light_source);

		// Check f.solidness because fast-style leaves look better this way
		if (f.param_type == CPT_LIGHT && f.solidness != 2) {
			u8 light_level_day = n.getLight(LIGHTBANK_DAY, f.getLightingFlags());
			u8 light_level_night = n.getLight(LIGHTBANK_NIGHT, f.getLightingFlags());

			light_day_max = std::max(decode_light(light_level_day), light_day_max);
			light_night_max = std::max(decode_light(light_level_night), light_night_max);

			auto block_light = data->m_blocklight_fill->getLight(pos);
			block_light_max = data->m_blocklight_fill->maxLight(block_light, block_light_max);

		} else {
			ambient_occlusion++;
		}
		return f.light_propagates;
	};

	bool obstructed[4] = { true, true, true, true };
	add_node(0);
	bool opaque1 = !add_node(1);
	bool opaque2 = !add_node(2);
	bool opaque3 = !add_node(3);
	obstructed[0] = opaque1 && opaque2;
	obstructed[1] = opaque1 && opaque3;
	obstructed[2] = opaque2 && opaque3;
	for (u8 k = 0; k < 3; ++k)
		if (add_node(k + 4, obstructed[k]))
			obstructed[3] = false;
	if (add_node(7, obstructed[3])) { // wrap light around nodes
		ambient_occlusion -= 3;
		for (u8 k = 0; k < 3; ++k)
			add_node(k + 4, !obstructed[k]);
	}

	// Skip the AO entirely if the node at 'p' has the block light below the max light source
	bool skip_ambient_occlusion = decode_light(light_source_max) > light_night_max;

	light_night_max = std::max(light_night_max, decode_light(light_source_max));

	LightPair lp;

	if (!skip_ambient_occlusion && ambient_occlusion > 4) {
		static thread_local const float ao_gamma = rangelim(
			g_settings->getFloat("ambient_occlusion_gamma"), 0.25, 4.0);

		// Table of gamma space multiply factors.
		static thread_local const float light_amount[3] = {
			powf(0.75, 1.0 / ao_gamma),
			powf(0.5,  1.0 / ao_gamma),
			powf(0.25, 1.0 / ao_gamma)
		};

		//calculate table index for gamma space multiplier
		lp.ambientOcclusion = light_amount[ambient_occlusion-5];
	}

	lp.lightDay = light_day_max;
	lp.lightNight = light_night_max;
	lp.blockLight = block_light_max;

	return lp;
}

/*
	Calculate smooth lighting at the given corner of p.
	Both light banks.
	Node at p is solid, and thus the lighting is face-dependent.
*/
LightPair getSmoothLightSolid(const v3s16 &p, const v3s16 &face_dir, const v3s16 &corner, MeshMakeData *data)
{
	return getSmoothLightTransparent(p + face_dir, corner - 2 * face_dir, data);
}

/*
	Calculate smooth lighting at the given corner of p.
	Both light banks.
	Node at p is not solid, and the lighting is not face-dependent.
*/
LightPair getSmoothLightTransparent(const v3s16 &p, const v3s16 &corner, MeshMakeData *data)
{
	const std::array<v3s16,8> dirs = {{
		// Always shine light
		v3s16(0,0,0),
		v3s16(corner.X,0,0),
		v3s16(0,corner.Y,0),
		v3s16(0,0,corner.Z),

		// Can be obstructed
		v3s16(corner.X,corner.Y,0),
		v3s16(corner.X,0,corner.Z),
		v3s16(0,corner.Y,corner.Z),
		v3s16(corner.X,corner.Y,corner.Z)
	}};
	return getSmoothLightCombined(p, dirs, data);
}
