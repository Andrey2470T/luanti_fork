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

struct LightPair {
    u8 lightDay;
    u8 lightNight;

    LightPair() = default;
    explicit LightPair(u16 value) : lightDay(value & 0xff), lightNight(value >> 8) {}
    LightPair(u8 valueA, u8 valueB) : lightDay(valueA), lightNight(valueB) {}
    LightPair(float valueA, float valueB) :
        lightDay(std::clamp(round32(valueA), 0, 255)),
        lightNight(std::clamp(round32(valueB), 0, 255)) {}
    operator u16() const { return lightDay | lightNight << 8; }
};

struct LightInfo {
    float light_day;
    float light_night;
    float light_boosted;

    LightPair getPair(float sunlight_boost = 0.0) const
    {
        return LightPair(
            (1 - sunlight_boost) * light_day
                + sunlight_boost * light_boosted,
            light_night);
    }
};

// Structure saving light values for each vertex at each cuboid face
struct LightFrame {
    u8 lightsDay[6][4];
    u8 lightsNight[6][4];
    //f32 lightsDay[8];
    //f32 lightsNight[8];
    bool sunlight[8];
};

extern const v3s16 light_dirs[8];
extern const u8 light_indices[6][4];

void getSmoothLightFrame(LightFrame &lframe, const v3s16 &p, MeshMakeData *data);
LightInfo blendLight(const v3f &vertex_pos, LightFrame &lframe);
img::color8 blendLightColor(const v3f &vertex_pos, const v3f &vertex_normal,
    LightFrame &lframe, u8 light_source);

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
