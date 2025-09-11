#include "lighting.h"

#include "voxel.h"
#include "nodedef.h"
#include "client/map/mapblockmesh.h"
#include "settings.h"
#include "map.h"
#include "util/directiontables.h"

/*
    Calculate non-smooth lighting at interior of node.
    Single light bank.
*/
static u8 getInteriorLight(enum LightBank bank, MapNode n, s32 increment,
    const NodeDefManager *ndef)
{
    u8 light = n.getLight(bank, ndef->getLightingFlags(n));
    light = std::clamp(light + increment, 0, LIGHT_SUN);
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
static u16 getSmoothLightCombined(const v3s16 &p,
    const std::array<v3s16,8> &dirs, MeshMakeData *data)
{
    const NodeDefManager *ndef = data->m_nodedef;

    u16 ambient_occlusion = 0;
    u16 light_count = 0;
    u8 light_source_max = 0;
    u16 light_day = 0;
    u16 light_night = 0;
    bool direct_sunlight = false;

    auto add_node = [&] (u8 i, bool obstructed = false) -> bool {
        if (obstructed) {
            ambient_occlusion++;
            return false;
        }
        MapNode n = data->m_vmanip.getNodeNoExNoEmerge(p + dirs[i]);
        if (n.getContent() == CONTENT_IGNORE)
            return true;
        const ContentFeatures &f = ndef->get(n);
        if (f.light_source > light_source_max)
            light_source_max = f.light_source;
        // Check f.solidness because fast-style leaves look better this way
        if (f.param_type == CPT_LIGHT && f.solidness != 2) {
            u8 light_level_day = n.getLight(LIGHTBANK_DAY, f.getLightingFlags());
            u8 light_level_night = n.getLight(LIGHTBANK_NIGHT, f.getLightingFlags());
            if (light_level_day == LIGHT_SUN)
                direct_sunlight = true;
            light_day += decode_light(light_level_day);
            light_night += decode_light(light_level_night);
            light_count++;
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

    if (light_count == 0) {
        light_day = light_night = 0;
    } else {
        light_day /= light_count;
        light_night /= light_count;
    }

    // boost direct sunlight, if any
    if (direct_sunlight)
        light_day = 0xFF;

    // Boost brightness around light sources
    bool skip_ambient_occlusion_day = false;
    if (decode_light(light_source_max) >= light_day) {
        light_day = decode_light(light_source_max);
        skip_ambient_occlusion_day = true;
    }

    bool skip_ambient_occlusion_night = false;
    if(decode_light(light_source_max) >= light_night) {
        light_night = decode_light(light_source_max);
        skip_ambient_occlusion_night = true;
    }

    if (ambient_occlusion > 4) {
        static thread_local const float ao_gamma = rangelim(
            g_settings->getFloat("ambient_occlusion_gamma"), 0.25, 4.0);

        // Table of gamma space multiply factors.
        static thread_local const float light_amount[3] = {
            powf(0.75, 1.0 / ao_gamma),
            powf(0.5,  1.0 / ao_gamma),
            powf(0.25, 1.0 / ao_gamma)
        };

        //calculate table index for gamma space multiplier
        ambient_occlusion -= 5;

        if (!skip_ambient_occlusion_day)
            light_day = rangelim(round32(
                                     light_day * light_amount[ambient_occlusion]), 0, 255);
        if (!skip_ambient_occlusion_night)
            light_night = rangelim(round32(
                                       light_night * light_amount[ambient_occlusion]), 0, 255);
    }

    return light_day | (light_night << 8);
}

/*
    Calculate smooth lighting at the given corner of p.
    Both light banks.
    Node at p is solid, and thus the lighting is face-dependent.
*/
u16 getSmoothLightSolid(const v3s16 &p, const v3s16 &face_dir, const v3s16 &corner, MeshMakeData *data)
{
    return getSmoothLightTransparent(p + face_dir, corner - face_dir * 2, data);
}

/*
    Calculate smooth lighting at the given corner of p.
    Both light banks.
    Node at p is not solid, and the lighting is not face-dependent.
*/
u16 getSmoothLightTransparent(const v3s16 &p, const v3s16 &corner, MeshMakeData *data)
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

// Maps cuboid face and vertex indices to the corresponding light index
const u8 light_indices[6][4] = {
    {3, 7, 6, 2},
    {0, 4, 5, 1},
    {6, 7, 5, 4},
    {3, 2, 0, 1},
    {7, 3, 1, 5},
    {2, 6, 4, 0},
};

// Gets the base lighting values for a node
void getSmoothLightFrame(LightFrame &lframe, const v3s16 &p, MeshMakeData *data)
{
    for (int k = 0; k < 8; ++k)
        lframe.sunlight[k] = false;

    std::array<LightPair, 8> light_corners;
    for (int k = 0; k < 8; ++k)
        light_corners[k] = LightPair(getSmoothLightTransparent(p, light_dirs[k], data));

    for (int face = 0; face < 6; ++face) {
        for (int k = 0; k < 4; k++) {
            u8 index = light_indices[face][k];
            auto lp = light_corners[index];

            lframe.lightsDay[face][k] = lp.lightDay;
            lframe.lightsNight[face][k] = lp.lightNight;

            // If there is direct sunlight and no ambient occlusion at some corner,
            // mark the vertical edge (top and bottom corners) containing it.
            if (lp.lightDay == 255) {
                lframe.sunlight[k] = true;
                lframe.sunlight[k ^ 2] = true;
            }
        }
    }
}

// Distance of light extrapolation (for oversized nodes)
// After this distance, it gives up and considers light level constant
#define SMOOTH_LIGHTING_OVERSIZE 1.0

// Calculates vertex light level
//  vertex_pos - vertex position in the node (coordinates are clamped to [0.0, 1.0] or so)
LightInfo blendLight(const v3f &vertex_pos, LightFrame &lframe)
{
    // Light levels at (logical) node corners are known. Here,
    // trilinear interpolation is used to calculate light level
    // at a given point in the node.
    f32 x = std::clamp(vertex_pos.X / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
    f32 y = std::clamp(vertex_pos.Y / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
    f32 z = std::clamp(vertex_pos.Z / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
    f32 lightDay = 0.0; // daylight
    f32 lightNight = 0.0;
    f32 lightBoosted = 0.0; // daylight + direct sunlight, if any

    std::array<u8, 8> lights_days;
    std::array<u8, 8> lights_nights;

    for (int face = 0; face < 6; ++face) {
        for (int k = 0; k < 4; k++) {
            u8 index = light_indices[face][k];

            lights_days[index] = std::max(lights_days[index], lframe.lightsDay[face][k]);
            lights_nights[index] = std::max(lights_nights[index], lframe.lightsNight[face][k]);
        }
    }
    for (int k = 0; k < 8; ++k) {
        f32 dx = (k & 4) ? x : 1 - x;
        f32 dy = (k & 2) ? y : 1 - y;
        f32 dz = (k & 1) ? z : 1 - z;
        // Use direct sunlight (255), if any; use daylight otherwise.
        f32 light_boosted = lframe.sunlight[k] ? 255 : lights_days[k];
        lightDay += dx * dy * dz * lights_days[k];
        lightNight += dx * dy * dz * lights_nights[k];
        lightBoosted += dx * dy * dz * light_boosted;
    }
    return LightInfo{lightDay, lightNight, lightBoosted};
}

// Calculates vertex color to be used in mapblock mesh
//  vertex_pos - vertex position in the node (coordinates are clamped to [0.0, 1.0] or so)
img::color8 blendLightColor(const v3f &vertex_pos, const v3f &vertex_normal,
    LightFrame &lframe, u8 light_source)
{
    LightInfo light = blendLight(vertex_pos, lframe);
    return encode_light(light.getPair(MYMAX(0.0f, vertex_normal.Y)), light_source);
}

img::color8 encode_light(u16 light, u8 emissive_light)
{
    // Get components
    u32 day = (light & 0xff);
    u32 night = (light >> 8);
    // Add emissive light
    night += emissive_light * 2.5f;
    if (night > 255)
        night = 255;
    // Since we don't know if the day light is sunlight or
    // artificial light, assume it is artificial when the night
    // light bank is also lit.
    if (day < night)
        day = 0;
    else
        day = day - night;
    u32 sum = day + night;
    // Ratio of sunlight:
    u32 r;
    if (sum > 0)
        r = day * 255 / sum;
    else
        r = 0;
    // Average light:
    float b = (day + night) / 2;
    return img::color8(img::PF_RGBA8, b, b, b, r);
}

void get_sunlight_color(img::colorf *sunlight, u32 daynight_ratio)
{
    f32 rg = daynight_ratio / 1000.0f - 0.04f;
    f32 b = (0.98f * daynight_ratio) / 1000.0f + 0.078f;
    sunlight->R(rg);
    sunlight->G(rg);
    sunlight->B(b);
}

void final_color_blend(img::color8 *result,
    u16 light, u32 daynight_ratio)
{
    img::colorf dayLight;
    get_sunlight_color(&dayLight, daynight_ratio);
    final_color_blend(result,
        encode_light(light, 0), dayLight);
}

void final_color_blend(img::color8 *result,
    const img::color8 &data, const img::colorf &dayLight)
{
    static const img::colorf artificialColor(img::PF_RGBA32F, 1.04f, 1.04f, 1.04f);

    img::color8 c(data);
    f32 n = 1 - c.A();

    f32 r = c.R() * (c.A() * dayLight.R() + n * artificialColor.R()) * 2.0f;
    f32 g = c.G() * (c.A() * dayLight.G() + n * artificialColor.G()) * 2.0f;
    f32 b = c.B() * (c.A() * dayLight.B() + n * artificialColor.B()) * 2.0f;

    // Emphase blue a bit in darker places
    // Each entry of this array represents a range of 8 blue levels
    static const u8 emphase_blue_when_dark[32] = {
        1, 4, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    b += emphase_blue_when_dark[std::clamp((s32) ((r + g + b) / 3 * 255),
        0, 255) / 8] / 255.0f;

    result->R(r * 255);
    result->G(g * 255);
    result->B(b * 255);
}

std::vector<v3s16> getCornerPositions(v3s16 origin)
{
    std::vector<v3s16> positions(6);

    for (u8 k = 0; k < 6; k++)
        positions[k] = origin + g_6dirs[k];

    return positions;
}

u16 getMaxLightLevel(Map &map, const NodeDefManager *ndef, const std::vector<v3s16> &positions, s32 glow)
{
    u16 light_at_pos = 0;
    u8 light_at_pos_intensity = 0;
    bool pos_ok = false;

    for (auto &pos : positions) {
        bool this_ok;
        MapNode n = map.getNode(pos, &this_ok);
        if (this_ok) {
            // Get light level at the position plus the entity glow
            u16 this_light = getInteriorLight(n, glow, ndef);
            u8 this_light_intensity = MYMAX(this_light & 0xFF, this_light >> 8);
            if (this_light_intensity > light_at_pos_intensity) {
                light_at_pos = this_light;
                light_at_pos_intensity = this_light_intensity;
            }
            pos_ok = true;
        }
    }
    if (!pos_ok)
        light_at_pos = LIGHT_SUN;

    return light_at_pos;
}

img::color8 getLightColor(Map &map, const NodeDefManager *ndef, const std::vector<v3s16> &positions, s32 glow)
{
    u16 light = getMaxLightLevel(map, ndef, positions, glow);

    // Initialize with full alpha, otherwise entity won't be visible
    img::color8 light_color = img::white;

    // Encode light into color, adding a small boost
    // based on the entity glow.
    light_color = encode_light(light, glow);

    return light_color;
}

img::color8 getBlendedLightColor(Map &map, const NodeDefManager *ndef, const std::vector<v3s16> &positions, u32 daynight_ratio)
{
    u16 light = getMaxLightLevel(map, ndef, positions, 0);

    img::color8 light_color;
    final_color_blend(&light_color, light, daynight_ratio);

    return light_color;
}
