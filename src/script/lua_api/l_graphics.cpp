#include "l_graphics.h"
#include "client/core/client.h"
#include "client/media/shader.h"
#include "nodedef.h"
#include "common/c_converter.h"
#include "l_internal.h"

int ModApiGraphics::l_register_material(lua_State *L)
{
	if (!lua_isstring(L, 1) || !lua_istable(L, 2))
		return 0;

	std::string name = readParam<std::string>(L, 1);

	lua_getfield(L, 2, "shader");

	if (lua_istable(L, -1)) {
		std::string vertex, geometry, fragment;

		vertex = getstringfield_default(L, -1, "vertex", "opengl_vertex.glsl");
		geometry = getstringfield_default(L, -1, "geometry", "opengl_geometry.glsl");
		fragment = getstringfield_default(L, -1, "fragment", "opengl_fragment.glsl");

		u32 id = getClient(L)->getShaderSource()->getShader({
			name, {}, {}, video::EMT_SOLID, vertex, geometry, fragment}, false);

		const_cast<NodeDefManager *>(getClient(L)->getNodeDefManager())->overrideShaderMaterials(name, id);
	}

	lua_pop(L, 1);

	return 1;
}

void ModApiGraphics::Initialize(lua_State *L, int top)
{
	API_FCT(register_material);
}
