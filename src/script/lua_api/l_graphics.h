#pragma once

#include "l_base.h"
#include "client/media/shader.h"
#include "client/render/material.h"
#include "client/pipeline/pipeline.h"

class UniformSetter : public ModApiBase
{
private:
	video::MaterialRenderer *m_renderer = nullptr;

	// garbage collector
	static int gc_object(lua_State *L);

	static int l_set(lua_State *L);

	static video::MaterialRenderer *getobject(lua_State *L);

public:
	UniformSetter(video::MaterialRenderer *renderer)
		: m_renderer(renderer)
	{}

	static void create(lua_State *L,  video::MaterialRenderer *renderer);
	static void Register(lua_State *L);

	static const char className[];
};

class ModApiGraphics : public ModApiBase
{
private:
	// gfx.register_material
	static int l_register_material(lua_State *L);

	static void read_basic_state(lua_State *L, MaterialStorageEntry &entry);
	static void read_constants(lua_State *L, ShaderConstants &constants);
	static void read_shader_info(lua_State *L, ShaderInfo &info, bool &applyShadows);

	static int l_set_lighting(lua_State *L);
	static int l_get_lighting(lua_State *L);

	static int l_set_sky(lua_State *L);
	static int l_get_sky(lua_State *L);

	static int l_set_sun(lua_State *L);
	static int l_get_sun(lua_State *L);

	static int l_set_moon(lua_State *L);
	static int l_get_moon(lua_State *L);

	static int l_set_stars(lua_State *L);
	static int l_get_stars(lua_State *L);

	static int l_set_clouds(lua_State *L);
	static int l_get_clouds(lua_State *L);

	static int l_override_day_night_ratio(lua_State *L);
	static int l_get_day_night_ratio(lua_State *L);

	static void read_texture_def(lua_State *L, TextureBufferDefinition &texdef);
	static int l_create_texture_buffer(lua_State *L);
public:
	static void Initialize(lua_State *L, int top);
};
