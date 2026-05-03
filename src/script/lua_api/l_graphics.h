#pragma once

#include "l_base.h"
#include "client/media/shader.h"
#include "client/render/material.h"

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
	static void read_pbr(lua_State *L, PBRTextures &textures);
	static void read_texture(lua_State *L, const std::string &name, video::GLTexture **tex);

	static void read_constants(lua_State *L, ShaderConstants &constants);
	static void read_shader_info(lua_State *L, ShaderInfo &info);
public:
	static void Initialize(lua_State *L, int top);
};
