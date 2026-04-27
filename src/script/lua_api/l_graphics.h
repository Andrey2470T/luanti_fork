#pragma once

#include "l_base.h"

class ShaderSource;

class ModApiGraphics : public ModApiBase
{
private:
	// gfx.register_material
	static int l_register_material(lua_State *L);
public:
	static void Initialize(lua_State *L, int top);
};
