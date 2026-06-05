#pragma once

extern "C" {
#include <lua.h>
}

#include <iostream>
#include <vector>
#include <array>

#include "irrlichttypes_bloated.h"
#include "util/string.h"

struct Lighting;
struct SkyboxParams;
struct SunParams;
struct MoonParams;
struct StarParams;
struct CloudParams;

void read_lighting(lua_State *L, int index, Lighting &lighting);
void push_lighting(lua_State *L, const Lighting &lighting);

void read_skyboxparams(lua_State *L, int index, SkyboxParams &sky_params);
void push_skyboxparams(lua_State *L, const SkyboxParams &skybox_params);

void read_sunparams(lua_State *L, int index, SunParams &sun_params);
void push_sunparams(lua_State *L, const SunParams &sun_params);

void read_moonparams(lua_State *L, int index, MoonParams &moon_params);
void push_moonparams(lua_State *L, const MoonParams &moon_params);

void read_starparams(lua_State *L, int index, StarParams &star_params);
void push_starparams(lua_State *L, const StarParams &star_params);

void read_cloudparams(lua_State *L, int index, CloudParams &cloud_params);
void push_cloudparams(lua_State *L, const CloudParams &cloud_params);

