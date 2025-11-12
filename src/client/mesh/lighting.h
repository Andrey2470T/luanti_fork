// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2025 Unified Lighting Refactor

#pragma once

#include <BasicIncludes.h>
#include <Image/Color.h>

class MapNode;
class NodeDefManager;
class MeshMakeData;

// Unified light pair structure
struct LightPair {
	u8 day;
	u8 night;

	LightPair() : day(0), night(0) {}
	explicit LightPair(u16 value) : day(value & 0xff), night(value >> 8) {}
	LightPair(u8 d, u8 n) : day(d), night(n) {}
	
	operator u16() const { return day | (night << 8); }
	
	LightPair max(const LightPair &other) const {
		return LightPair(std::max(day, other.day), std::max(night, other.night));
	}
};

// Lighting configuration for a vertex
struct VertexLight {
	LightPair light;
	bool is_sunlight;  // true if daylight comes from direct sun
	
	VertexLight() : light(), is_sunlight(false) {}
	VertexLight(LightPair l, bool sun = false) : light(l), is_sunlight(sun) {}
};

// Complete lighting information for a node
struct NodeLighting {
	// Corner lighting (8 corners of a cube)
	VertexLight corners[8];
	
	// Face lighting (6 faces, 4 vertices each)
	// Derived from corners but cached for convenience
	VertexLight faces[6][4];
	
	// Uniform lighting (for non-smooth lighting)
	LightPair uniform;
	
	NodeLighting() : uniform() {
		for (int i = 0; i < 8; i++)
			corners[i] = VertexLight();
		for (int f = 0; f < 6; f++)
			for (int v = 0; v < 4; v++)
				faces[f][v] = VertexLight();
	}
};

// Maps corner indices (0-7) to their position offsets
// Bit 0 = X, Bit 1 = Y, Bit 2 = Z
// 0 = (-1,-1,-1), 1 = (-1,-1,+1), ..., 7 = (+1,+1,+1)
inline v3s16 getCornerOffset(int corner_index) {
	return v3s16(
		(corner_index & 4) ? 1 : -1,
		(corner_index & 2) ? 1 : -1,
		(corner_index & 1) ? 1 : -1
	);
}

// Maps face index (0-5) and vertex index (0-3) to corner index (0-7)
// Faces: 0=Y+, 1=Y-, 2=X+, 3=X-, 4=Z+, 5=Z-
extern const u8 FACE_TO_CORNER[6][4];

// Calculate lighting for a node at position p
// smooth: use smooth lighting (interpolate from neighbors)
// solid: node is solid (affects which neighbors to sample)
void calculateNodeLighting(
	NodeLighting &result,
	const v3s16 &p,
	bool smooth,
	MeshMakeData *data
);

// Calculate lighting for a single corner
VertexLight calculateCornerLight(
	const v3s16 &p,
	const v3s16 &corner_offset,
	MeshMakeData *data
);

// Calculate uniform (non-smooth) lighting for a node
LightPair calculateUniformLight(
	MapNode node,
	const NodeDefManager *ndef
);

// Calculate lighting at a face (for solid nodes)
LightPair calculateFaceLight(
	const v3s16 &p,
	const v3s16 &face_dir,
	MeshMakeData *data
);

// Interpolate lighting at a point within a node
// pos: position in BS units relative to node center [-0.5, 0.5]
VertexLight interpolateLight(
	const NodeLighting &lighting,
	const v3f &pos
);

// Encode light to vertex color
// light: the light pair (day/night)
// emissive: light source strength (0-15)
// normal: surface normal (used for sunlight boost on top faces)
img::color8 encodeVertexLight(
	LightPair light,
	u8 emissive,
	const v3f &normal = v3f(0, 0, 0)
);

// Encode with sunlight boost for top-facing surfaces
img::color8 encodeVertexLightWithSun(
	const VertexLight &vlight,
	u8 emissive,
	const v3f &normal = v3f(0, 0, 0)
);
