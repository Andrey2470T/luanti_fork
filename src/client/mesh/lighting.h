#pragma once

#include <BasicIncludes.h>
#include <Image/Color.h>

class MapNode;
class NodeDefManager;
class MeshMakeData;

// Compute light at node
u16 getInteriorLight(MapNode n, s32 increment, const NodeDefManager *ndef);
u16 getFaceLight(MapNode n, MapNode n2, const NodeDefManager *ndef);
u16 getSmoothLightSolid(const v3s16 &p, const v3s16 &face_dir, const v3s16 &corner, MeshMakeData *data);
u16 getSmoothLightTransparent(const v3s16 &p, const v3s16 &corner, MeshMakeData *data);

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
img::color8 encode_light(u16 light, u8 emissive_light);

/*!
 * Returns the sunlight's color from the current
 * day-night ratio.
 */
void get_sunlight_color(img::colorf *sunlight, u32 daynight_ratio);

/*!
 * Gives the final  SColor shown on screen.
 *
 * \param result output color
 * \param light first 8 bits are day light, second 8 bits are
 * night light
 */
void final_color_blend(img::color8 *result,
    u16 light, u32 daynight_ratio);

/*!
 * Gives the final  SColor shown on screen.
 *
 * \param result output color
 * \param data the half-baked vertex color
 * \param dayLight color of the sunlight
 */
void final_color_blend(img::color8 *result,
    const img::color8 &data, const img::colorf &dayLight);
