#pragma once

#include "l_base.h"
#include "client/media/shader.h"

class ModApiGraphics : public ModApiBase
{
private:
	// gfx.register_material
	static int l_register_material(lua_State *L);

	static void read_constants(lua_State *L, ShaderConstants &constants);
	static void read_shader_info(lua_State *L, ShaderInfo &info);
public:
	static void Initialize(lua_State *L, int top);
};
