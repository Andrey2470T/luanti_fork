// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2025 Unified Lighting Refactor

#include "lighting.h"
#include "map.h"
#include "voxel.h"
#include "nodedef.h"
#include "client/map/mapblockmesh.h"
#include "settings.h"
#include "util/numeric.h"

// Face-to-corner mapping
// Faces: 0=Y+, 1=Y-, 2=X+, 3=X-, 4=Z+, 5=Z-
const u8 FACE_TO_CORNER[6][4] = {
	{3, 7, 6, 2},  // Y+ face
	{0, 4, 5, 1},  // Y- face
	{6, 7, 5, 4},  // X+ face
	{3, 2, 0, 1},  // X- face
	{7, 3, 1, 5},  // Z+ face
	{2, 6, 4, 0},  // Z- face
};

// Helper: decode light value (expand from 0-15 to 0-255)
static inline u8 decodeLightValue(u8 encoded) {
	if (encoded <= LIGHT_MAX)
		return decode_light(encoded);
	return 255; // LIGHT_SUN maps to 255
}

// Helper: Get light value from a node
static LightPair getNodeLight(MapNode node, const NodeDefManager *ndef) {
	ContentLightingFlags flags = ndef->getLightingFlags(node);
	
	u8 day_raw = node.getLight(LIGHTBANK_DAY, flags);
	u8 night_raw = node.getLight(LIGHTBANK_NIGHT, flags);
	
	u8 day = decodeLightValue(day_raw);
	u8 night = decodeLightValue(night_raw);
	
	// Boost by light source
	if (flags.light_source > 0) {
		u8 source = decodeLightValue(flags.light_source);
		day = std::max(day, source);
		night = std::max(night, source);
	}
	
	return LightPair(day, night);
}

// Calculate lighting for a single corner with ambient occlusion
VertexLight calculateCornerLight(
	const v3s16 &p,
	const v3s16 &corner_offset,
	MeshMakeData *data
) {
	const NodeDefManager *ndef = data->m_nodedef;
	
	// Positions to sample (using smooth lighting algorithm)
	// Center, 3 edges, 3 faces, 1 corner = 8 samples
	const std::array<v3s16, 8> sample_offsets = {{
		v3s16(0, 0, 0),                    // center
		v3s16(corner_offset.X, 0, 0),      // X edge
		v3s16(0, corner_offset.Y, 0),      // Y edge  
		v3s16(0, 0, corner_offset.Z),      // Z edge
		v3s16(corner_offset.X, corner_offset.Y, 0),  // XY face
		v3s16(corner_offset.X, 0, corner_offset.Z),  // XZ face
		v3s16(0, corner_offset.Y, corner_offset.Z),  // YZ face
		v3s16(corner_offset.X, corner_offset.Y, corner_offset.Z)  // corner
	}};
	
	u16 light_day_sum = 0;
	u16 light_night_sum = 0;
	u16 light_count = 0;
	u16 ambient_occlusion = 0;
	u8 max_light_source = 0;
	bool has_sunlight = false;
	
	// Lambda to add a node's contribution
	auto add_node = [&](int i, bool obstructed = false) -> bool {
		if (obstructed) {
			ambient_occlusion++;
			return false;
		}
		
		MapNode n = data->m_vmanip.getNodeNoExNoEmerge(p + sample_offsets[i]);
		if (n.getContent() == CONTENT_IGNORE)
			return true;
			
		const ContentFeatures &f = ndef->get(n);
		
		// Track max light source
		if (f.light_source > max_light_source)
			max_light_source = f.light_source;
		
		// Only transparent nodes contribute light
		if (f.param_type == CPT_LIGHT && f.solidness != 2) {
			ContentLightingFlags flags = f.getLightingFlags();
			u8 day_raw = n.getLight(LIGHTBANK_DAY, flags);
			u8 night_raw = n.getLight(LIGHTBANK_NIGHT, flags);
			
			if (day_raw == LIGHT_SUN)
				has_sunlight = true;
			
			light_day_sum += decodeLightValue(day_raw);
			light_night_sum += decodeLightValue(night_raw);
			light_count++;
		} else {
			ambient_occlusion++;
		}
		
		return f.light_propagates;
	};
	
	// Sample in order: center, edges, faces, corner
	// Track obstructions for proper AO
	bool obstructed[4] = {true, true, true, true};
	add_node(0);  // center
	
	bool opaque_x = !add_node(1);  // X edge
	bool opaque_y = !add_node(2);  // Y edge
	bool opaque_z = !add_node(3);  // Z edge
	
	// Faces are obstructed if both adjacent edges are opaque
	obstructed[0] = opaque_x && opaque_y;  // XY face
	obstructed[1] = opaque_x && opaque_z;  // XZ face
	obstructed[2] = opaque_y && opaque_z;  // YZ face
	
	for (int k = 0; k < 3; k++)
		if (add_node(k + 4, obstructed[k]))
			obstructed[3] = false;
	
	// Corner (with special wrap-around logic)
	if (add_node(7, obstructed[3])) {
		ambient_occlusion -= 3;
		for (int k = 0; k < 3; k++)
			add_node(k + 4, !obstructed[k]);
	}
	
	// Calculate average light
	u8 final_day, final_night;
	if (light_count == 0) {
		final_day = final_night = 0;
	} else {
		final_day = light_day_sum / light_count;
		final_night = light_night_sum / light_count;
	}
	
	// Apply direct sunlight boost
	if (has_sunlight)
		final_day = 255;
	
	// Boost by light sources
	u8 source_light = decodeLightValue(max_light_source);
	bool skip_ao_day = false;
	bool skip_ao_night = false;
	
	if (source_light >= final_day) {
		final_day = source_light;
		skip_ao_day = true;
	}
	if (source_light >= final_night) {
		final_night = source_light;
		skip_ao_night = true;
	}
	
	// Apply ambient occlusion
	if (ambient_occlusion > 4) {
		static thread_local const float ao_gamma = rangelim(
			g_settings->getFloat("ambient_occlusion_gamma"), 0.25, 4.0);
		static thread_local const float ao_factors[3] = {
			powf(0.75, 1.0 / ao_gamma),
			powf(0.50, 1.0 / ao_gamma),
			powf(0.25, 1.0 / ao_gamma)
		};
		
		int ao_index = ambient_occlusion - 5;
		if (!skip_ao_day)
			final_day = rangelim(round32(final_day * ao_factors[ao_index]), 0, 255);
		if (!skip_ao_night)
			final_night = rangelim(round32(final_night * ao_factors[ao_index]), 0, 255);
	}
	
	return VertexLight(LightPair(final_day, final_night), has_sunlight);
}

// Calculate uniform (non-smooth) lighting
LightPair calculateUniformLight(MapNode node, const NodeDefManager *ndef) {
	return getNodeLight(node, ndef);
}

LightPair calculateAverageLight(Map &map, const NodeDefManager *ndef, const std::vector<v3s16> &positions) {
    LightPair max_light;

    bool pos_ok = false;
    for (const auto &pos : positions) {
        auto node = map.getNode(pos, &pos_ok);

        if (!pos_ok)
            continue;
        auto light_pair = getNodeLight(node, ndef);

        if (light_pair.night > max_light.night)
            max_light = light_pair;
    }

    return max_light;
}

// Calculate face lighting (for solid nodes)
LightPair calculateFaceLight(
	const v3s16 &p,
	const v3s16 &face_dir,
	MeshMakeData *data
) {
	const NodeDefManager *ndef = data->m_nodedef;
	
	MapNode n1 = data->m_vmanip.getNodeNoExNoEmerge(p);
	MapNode n2 = data->m_vmanip.getNodeNoExNoEmerge(p + face_dir);
	
	if (n1.getContent() == CONTENT_IGNORE || n2.getContent() == CONTENT_IGNORE)
		return LightPair(0, 0);
	
	LightPair light1 = getNodeLight(n1, ndef);
	LightPair light2 = getNodeLight(n2, ndef);
	
	return light1.max(light2);
}

// Calculate complete lighting for a node
void calculateNodeLighting(
	NodeLighting &result,
	const v3s16 &p,
	bool smooth,
	MeshMakeData *data
) {
	if (!smooth) {
		// Uniform lighting
		MapNode node = data->m_vmanip.getNodeNoExNoEmerge(p);
		result.uniform = calculateUniformLight(node, data->m_nodedef);
		
		// Set all corners/faces to uniform
		for (int i = 0; i < 8; i++)
			result.corners[i] = VertexLight(result.uniform);
			
		for (int f = 0; f < 6; f++)
			for (int v = 0; v < 4; v++)
				result.faces[f][v] = VertexLight(result.uniform);
				
		return;
	}
	
	// Smooth lighting: calculate each corner
	for (int i = 0; i < 8; i++) {
		v3s16 offset = getCornerOffset(i);
		result.corners[i] = calculateCornerLight(p, offset, data);
	}
	
	// Map corners to faces
	for (int face = 0; face < 6; face++) {
		for (int vert = 0; vert < 4; vert++) {
			u8 corner_idx = FACE_TO_CORNER[face][vert];
			result.faces[face][vert] = result.corners[corner_idx];
		}
	}
	
	// Calculate uniform as average of all corners
	u16 day_sum = 0, night_sum = 0;
	for (int i = 0; i < 8; i++) {
		day_sum += result.corners[i].light.day;
		night_sum += result.corners[i].light.night;
	}
	result.uniform = LightPair(day_sum / 8, night_sum / 8);
}

// Interpolate lighting at a point within the node
VertexLight interpolateLight(
	const NodeLighting &lighting,
	const v3f &pos
) {
	// Trilinear interpolation
	// pos is in BS units relative to node center: [-0.5, 0.5]
	// Map to [0, 1] for interpolation
    const f32 OVERSIZE = 1.0f;  // allow slight extrapolation
    f32 x = std::clamp(pos.X / BS + 0.5f, 0.0f - OVERSIZE, 1.0f + OVERSIZE);
    f32 y = std::clamp(pos.Y / BS + 0.5f, 0.0f - OVERSIZE, 1.0f + OVERSIZE);
    f32 z = std::clamp(pos.Z / BS + 0.5f, 0.0f - OVERSIZE, 1.0f + OVERSIZE);
	
    f32 day = 0.0f;
    f32 night = 0.0f;
	bool any_sunlight = false;
	
	for (int k = 0; k < 8; k++) {
        f32 dx = (k & 4) ? x : (1.0f - x);
        f32 dy = (k & 2) ? y : (1.0f - y);
        f32 dz = (k & 1) ? z : (1.0f - z);
        f32 weight = dx * dy * dz;
		
		const VertexLight &vl = lighting.corners[k];
		day += weight * vl.light.day;
		night += weight * vl.light.night;
		
		if (vl.is_sunlight)
			any_sunlight = true;
	}
	
	return VertexLight(
		LightPair(
			static_cast<u8>(std::clamp(day, 0.0f, 255.0f)),
			static_cast<u8>(std::clamp(night, 0.0f, 255.0f))
		),
		any_sunlight
	);
}

// Encode light to vertex color (same as original encode_light)
img::color8 encodeVertexLight(
	LightPair light,
	u8 emissive,
	const v3f &normal
) {
	u32 day = light.day;
	u32 night = light.night;
	
	// Add emissive light
	night += emissive * 2.5f;
	if (night > 255)
		night = 255;
	
	// Artificial light subtraction
	if (day < night)
		day = 0;
	else
        day -= night;
	
	u32 sum = day + night;
	u32 ratio = (sum > 0) ? (day * 255 / sum) : 0;
	
    f32 avg = (day + night) / 2.0f;
	
	return img::color8(img::PF_RGBA8, avg, avg, avg, ratio);
}

// Encode with sunlight boost
img::color8 encodeVertexLightWithSun(
	const VertexLight &vlight,
	u8 emissive,
	const v3f &normal
) {
	LightPair light = vlight.light;
	
	// Apply sunlight boost for upward-facing surfaces
	if (vlight.is_sunlight && normal.Y > 0.0f) {
		light.day = 255;
	}
	
	return encodeVertexLight(light, emissive, normal);
}
