#include "c_gfx_content.h"
#include "util/numeric.h"
#include "common/c_converter.h"
#include "common/c_types.h"
#include "common/helper.h"
#include "lua_api/l_internal.h"
#include "lighting.h"
#include "skyparams.h"

void read_lighting(lua_State *L, int index, Lighting &lighting)
{
	luaL_checktype(L, index, LUA_TTABLE);

	lua_getfield(L, index, "shadows");
	if (lua_istable(L, -1)) {
		getfloatfield(L, -1, "intensity", lighting.shadow_intensity);
		getcolorfield(L, -1, "tint", lighting.shadow_tint);
	}
	lua_pop(L, 1); // shadows

	getfloatfield(L, index, "saturation", lighting.saturation);

	lua_getfield(L, index, "exposure");
	if (lua_istable(L, -1)) {
		lighting.exposure.luminance_min       = getfloatfield_default(L, -1, "luminance_min",       lighting.exposure.luminance_min);
		lighting.exposure.luminance_max       = getfloatfield_default(L, -1, "luminance_max",       lighting.exposure.luminance_max);
		lighting.exposure.exposure_correction = getfloatfield_default(L, -1, "exposure_correction", lighting.exposure.exposure_correction);
		lighting.exposure.speed_dark_bright   = getfloatfield_default(L, -1, "speed_dark_bright",   lighting.exposure.speed_dark_bright);
		lighting.exposure.speed_bright_dark   = getfloatfield_default(L, -1, "speed_bright_dark",   lighting.exposure.speed_bright_dark);
		lighting.exposure.center_weight_power = getfloatfield_default(L, -1, "center_weight_power", lighting.exposure.center_weight_power);
	}
	lua_pop(L, 1); // exposure

	lua_getfield(L, index, "volumetric_light");
	if (lua_istable(L, -1)) {
		getfloatfield(L, -1, "strength", lighting.volumetric_light_strength);
		lighting.volumetric_light_strength = rangelim(lighting.volumetric_light_strength, 0.0f, 1.0f);
	}
	lua_pop(L, 1); // volumetric_light

	lua_getfield(L, index, "bloom");
	if (lua_istable(L, -1)) {
		lighting.bloom_strength_factor = getfloatfield_default(L, -1, "strength_factor", lighting.bloom_strength_factor);
		lighting.bloom_radius          = getfloatfield_default(L, -1, "radius",          lighting.bloom_radius);
	}
	lua_pop(L, 1); // bloom
}

void push_lighting(lua_State *L, const Lighting &lighting)
{
	lua_newtable(L); // "shadows"
	lua_pushnumber(L, lighting.shadow_intensity);
	lua_setfield(L, -2, "intensity");
	push_ARGB8(L, lighting.shadow_tint);
	lua_setfield(L, -2, "tint");
	lua_setfield(L, -2, "shadows");
	lua_pushnumber(L, lighting.saturation);
	lua_setfield(L, -2, "saturation");
	lua_newtable(L); // "exposure"
	lua_pushnumber(L, lighting.exposure.luminance_min);
	lua_setfield(L, -2, "luminance_min");
	lua_pushnumber(L, lighting.exposure.luminance_max);
	lua_setfield(L, -2, "luminance_max");
	lua_pushnumber(L, lighting.exposure.exposure_correction);
	lua_setfield(L, -2, "exposure_correction");
	lua_pushnumber(L, lighting.exposure.speed_dark_bright);
	lua_setfield(L, -2, "speed_dark_bright");
	lua_pushnumber(L, lighting.exposure.speed_bright_dark);
	lua_setfield(L, -2, "speed_bright_dark");
	lua_pushnumber(L, lighting.exposure.center_weight_power);
	lua_setfield(L, -2, "center_weight_power");
	lua_setfield(L, -2, "exposure");
	lua_newtable(L); // "volumetric_light"
	lua_pushnumber(L, lighting.volumetric_light_strength);
	lua_setfield(L, -2, "strength");
	lua_setfield(L, -2, "volumetric_light");
	lua_newtable(L); // "bloom"
	// NOTE: 'intensity' param was deleted in 1.4.0
	lua_pushnumber(L, 0.0f);
	lua_setfield(L, -2, "intensity");
	lua_pushnumber(L, lighting.bloom_strength_factor);
	lua_setfield(L, -2, "strength_factor");
	lua_pushnumber(L, lighting.bloom_radius);
	lua_setfield(L, -2, "radius");
	lua_setfield(L, -2, "bloom");
}

void read_skyboxparams(lua_State *L, int index, SkyboxParams &sky_params)
{
	luaL_checktype(L, index, LUA_TTABLE);

	lua_getfield(L, index, "base_color");
	if (!lua_isnil(L, -1))
		read_color(L, -1, &sky_params.bgcolor);
	lua_pop(L, 1);

	lua_getfield(L, index, "body_orbit_tilt");
	if (!lua_isnil(L, -1)) {
		sky_params.body_orbit_tilt = rangelim(readParam<float>(L, -1), -60.0f, 60.0f);
	}
	lua_pop(L, 1);

	lua_getfield(L, index, "type");
	if (!lua_isnil(L, -1))
		sky_params.type = luaL_checkstring(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "textures");
	sky_params.textures.clear();
	if (lua_istable(L, -1) && sky_params.type == "skybox") {
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			// Key is at index -2 and value at index -1
			sky_params.textures.emplace_back(readParam<std::string>(L, -1));
			// Removes the value, but keeps the key for iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	// Validate that we either have six or zero textures
	if (sky_params.textures.size() != 6 && !sky_params.textures.empty())
		throw LuaError("Skybox expects 6 textures!");

	sky_params.clouds = getboolfield_default(L, index, "clouds", sky_params.clouds);

	lua_getfield(L, index, "sky_color");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "day_sky");
		read_color(L, -1, &sky_params.sky_color.day_sky);
		lua_pop(L, 1);

		lua_getfield(L, -1, "day_horizon");
		read_color(L, -1, &sky_params.sky_color.day_horizon);
		lua_pop(L, 1);

		lua_getfield(L, -1, "dawn_sky");
		read_color(L, -1, &sky_params.sky_color.dawn_sky);
		lua_pop(L, 1);

		lua_getfield(L, -1, "dawn_horizon");
		read_color(L, -1, &sky_params.sky_color.dawn_horizon);
		lua_pop(L, 1);

		lua_getfield(L, -1, "night_sky");
		read_color(L, -1, &sky_params.sky_color.night_sky);
		lua_pop(L, 1);

		lua_getfield(L, -1, "night_horizon");
		read_color(L, -1, &sky_params.sky_color.night_horizon);
		lua_pop(L, 1);

		lua_getfield(L, -1, "indoors");
		read_color(L, -1, &sky_params.sky_color.indoors);
		lua_pop(L, 1);

		// Prevent flickering clouds at dawn/dusk:
		sky_params.fog_sun_tint = video::SColor(255, 255, 255, 255);
		lua_getfield(L, -1, "fog_sun_tint");
		read_color(L, -1, &sky_params.fog_sun_tint);
		lua_pop(L, 1);

		sky_params.fog_moon_tint = video::SColor(255, 255, 255, 255);
		lua_getfield(L, -1, "fog_moon_tint");
		read_color(L, -1, &sky_params.fog_moon_tint);
		lua_pop(L, 1);

		lua_getfield(L, -1, "fog_tint_type");
		if (!lua_isnil(L, -1))
			sky_params.fog_tint_type = luaL_checkstring(L, -1);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	lua_getfield(L, index, "fog");
	if (lua_istable(L, -1)) {
		sky_params.fog_distance = getintfield_default(L, -1,
			"fog_distance", sky_params.fog_distance);
		sky_params.fog_start = getfloatfield_default(L, -1,
			"fog_start", sky_params.fog_start);

		lua_getfield(L, -1, "fog_color");
		read_color(L, -1, &sky_params.fog_color);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

static void push_sky_color(lua_State *L, const SkyboxParams &params)
{
	lua_newtable(L);
	if (params.type == "regular") {
		push_ARGB8(L, params.sky_color.day_sky);
		lua_setfield(L, -2, "day_sky");
		push_ARGB8(L, params.sky_color.day_horizon);
		lua_setfield(L, -2, "day_horizon");
		push_ARGB8(L, params.sky_color.dawn_sky);
		lua_setfield(L, -2, "dawn_sky");
		push_ARGB8(L, params.sky_color.dawn_horizon);
		lua_setfield(L, -2, "dawn_horizon");
		push_ARGB8(L, params.sky_color.night_sky);
		lua_setfield(L, -2, "night_sky");
		push_ARGB8(L, params.sky_color.night_horizon);
		lua_setfield(L, -2, "night_horizon");
		push_ARGB8(L, params.sky_color.indoors);
		lua_setfield(L, -2, "indoors");
	}
	push_ARGB8(L, params.fog_sun_tint);
	lua_setfield(L, -2, "fog_sun_tint");
	push_ARGB8(L, params.fog_moon_tint);
	lua_setfield(L, -2, "fog_moon_tint");
	lua_pushstring(L, params.fog_tint_type.c_str());
	lua_setfield(L, -2, "fog_tint_type");
}

void push_skyboxparams(lua_State *L, const SkyboxParams &skybox_params)
{
	lua_newtable(L);
	push_ARGB8(L, skybox_params.bgcolor);
	lua_setfield(L, -2, "base_color");
	lua_pushlstring(L, skybox_params.type.c_str(), skybox_params.type.size());
	lua_setfield(L, -2, "type");

	if (skybox_params.body_orbit_tilt != SkyboxParams::INVALID_SKYBOX_TILT) {
		lua_pushnumber(L, skybox_params.body_orbit_tilt);
		lua_setfield(L, -2, "body_orbit_tilt");
	}
	lua_newtable(L);
	s16 i = 1;
	for (const std::string &texture : skybox_params.textures) {
		lua_pushlstring(L, texture.c_str(), texture.size());
		lua_rawseti(L, -2, i++);
	}
	lua_setfield(L, -2, "textures");
	lua_pushboolean(L, skybox_params.clouds);
	lua_setfield(L, -2, "clouds");

	push_sky_color(L, skybox_params);
	lua_setfield(L, -2, "sky_color");

	lua_newtable(L); // fog
	lua_pushinteger(L, skybox_params.fog_distance >= 0 ? skybox_params.fog_distance : -1);
	lua_setfield(L, -2, "fog_distance");
	lua_pushnumber(L, skybox_params.fog_start >= 0 ? skybox_params.fog_start : -1.0f);
	lua_setfield(L, -2, "fog_start");
	lua_setfield(L, -2, "fog");
}

void read_sunparams(lua_State *L, int index, SunParams &sun_params)
{
	luaL_checktype(L, index, LUA_TTABLE);
	sun_params.visible = getboolfield_default(L, index,   "visible", sun_params.visible);
	sun_params.texture = getstringfield_default(L, index, "texture", sun_params.texture);
	sun_params.tonemap = getstringfield_default(L, index, "tonemap", sun_params.tonemap);
	sun_params.sunrise = getstringfield_default(L, index, "sunrise", sun_params.sunrise);
	sun_params.sunrise_visible = getboolfield_default(L, index, "sunrise_visible", sun_params.sunrise_visible);
	sun_params.scale   = getfloatfield_default(L, index,  "scale",   sun_params.scale);
}

void push_sunparams(lua_State *L, const SunParams &sun_params)
{
	lua_newtable(L);
	lua_pushboolean(L, sun_params.visible);
	lua_setfield(L, -2, "visible");
	lua_pushstring(L, sun_params.texture.c_str());
	lua_setfield(L, -2, "texture");
	lua_pushstring(L, sun_params.tonemap.c_str());
	lua_setfield(L, -2, "tonemap");
	lua_pushstring(L, sun_params.sunrise.c_str());
	lua_setfield(L, -2, "sunrise");
	lua_pushboolean(L, sun_params.sunrise_visible);
	lua_setfield(L, -2, "sunrise_visible");
	lua_pushnumber(L, sun_params.scale);
	lua_setfield(L, -2, "scale");
}

void read_moonparams(lua_State *L, int index, MoonParams &moon_params)
{
	luaL_checktype(L, index, LUA_TTABLE);
	moon_params.visible = getboolfield_default(L, index,   "visible", moon_params.visible);
	moon_params.texture = getstringfield_default(L, index, "texture", moon_params.texture);
	moon_params.tonemap = getstringfield_default(L, index, "tonemap", moon_params.tonemap);
	moon_params.scale   = getfloatfield_default(L, index,  "scale",   moon_params.scale);
}

void push_moonparams(lua_State *L, const MoonParams &moon_params)
{
	lua_newtable(L);
	lua_pushboolean(L, moon_params.visible);
	lua_setfield(L, -2, "visible");
	lua_pushstring(L, moon_params.texture.c_str());
	lua_setfield(L, -2, "texture");
	lua_pushstring(L, moon_params.tonemap.c_str());
	lua_setfield(L, -2, "tonemap");
	lua_pushnumber(L, moon_params.scale);
	lua_setfield(L, -2, "scale");
}

void read_starparams(lua_State *L, int index, StarParams &star_params)
{
	luaL_checktype(L, index, LUA_TTABLE);
	star_params.visible = getboolfield_default(L, index, "visible", star_params.visible);
	star_params.count   = getintfield_default(L, index,  "count",   star_params.count);

	lua_getfield(L, index, "star_color");
	if (!lua_isnil(L, -1))
		read_color(L, -1, &star_params.starcolor);
	lua_pop(L, 1);

	star_params.scale = getfloatfield_default(L, index,
		"scale", star_params.scale);
	star_params.day_opacity = getfloatfield_default(L, index,
		"day_opacity", star_params.day_opacity);
}

void push_starparams(lua_State *L, const StarParams &star_params)
{
	lua_newtable(L);
	lua_pushboolean(L, star_params.visible);
	lua_setfield(L, -2, "visible");
	lua_pushnumber(L, star_params.count);
	lua_setfield(L, -2, "count");
	push_ARGB8(L, star_params.starcolor);
	lua_setfield(L, -2, "star_color");
	lua_pushnumber(L, star_params.scale);
	lua_setfield(L, -2, "scale");
	lua_pushnumber(L, star_params.day_opacity);
	lua_setfield(L, -2, "day_opacity");
}

void read_cloudparams(lua_State *L, int index, CloudParams &cloud_params)
{
	luaL_checktype(L, index, LUA_TTABLE);
	cloud_params.density = getfloatfield_default(L, index, "density", cloud_params.density);

	lua_getfield(L, index, "color");
	if (!lua_isnil(L, -1))
		read_color(L, -1, &cloud_params.color_bright);
	lua_pop(L, 1);
	lua_getfield(L, index, "ambient");
	if (!lua_isnil(L, -1))
		read_color(L, -1, &cloud_params.color_ambient);
	lua_pop(L, 1);
	lua_getfield(L, index, "shadow");
	if (!lua_isnil(L, -1))
		read_color(L, -1, &cloud_params.color_shadow);
	lua_pop(L, 1);

	cloud_params.height    = getfloatfield_default(L, index, "height",    cloud_params.height);
	cloud_params.thickness = getfloatfield_default(L, index, "thickness", cloud_params.thickness);

	lua_getfield(L, index, "speed");
	if (lua_istable(L, -1)) {
		v2f new_speed;
		new_speed.X = getfloatfield_default(L, -1, "x", 0);
		new_speed.Y = getfloatfield_default(L, -1, "z", 0);
		cloud_params.speed = new_speed;
	}
	lua_pop(L, 1);
}

void push_cloudparams(lua_State *L, const CloudParams &cloud_params)
{
	lua_newtable(L);
	lua_pushnumber(L, cloud_params.density);
	lua_setfield(L, -2, "density");
	push_ARGB8(L, cloud_params.color_bright);
	lua_setfield(L, -2, "color");
	push_ARGB8(L, cloud_params.color_ambient);
	lua_setfield(L, -2, "ambient");
	push_ARGB8(L, cloud_params.color_shadow);
	lua_setfield(L, -2, "shadow");
	lua_pushnumber(L, cloud_params.height);
	lua_setfield(L, -2, "height");
	lua_pushnumber(L, cloud_params.thickness);
	lua_setfield(L, -2, "thickness");
	lua_newtable(L);
	lua_pushnumber(L, cloud_params.speed.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, cloud_params.speed.Y);
	lua_setfield(L, -2, "y");
	lua_setfield(L, -2, "speed");
}
