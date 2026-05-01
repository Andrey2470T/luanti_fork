#include "l_graphics.h"
#include "client/core/client.h"
#include "client/media/shader.h"
#include "nodedef.h"
#include "common/c_converter.h"
#include "l_internal.h"

void ModApiGraphics::read_constants(lua_State *L, ShaderConstants &constants)
{
	lua_getfield(L, -1, "constants");

	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}

	int t = lua_gettop(L);

	std::string constant_name = "";
	u8 constant_value;

	lua_pushnil(L);
	while (lua_next(L, t)) {
		if (lua_istable(L, -1)) {
			// Read the constant name
			lua_pushinteger(L, 1);
			lua_gettable(L, -2);

			if (lua_isstring(L, -1))
				constant_name = readParam<std::string>(L, -1);
			lua_pop(L, 2);

			if (!constant_name.empty()) {
				// Read the constant value (u8)
				lua_pushinteger(L, 2);
				lua_gettable(L, -2);

				if (lua_isnumber(L, -1)) {
					constant_value = readParam<s32>(L, -1);
					constants[constant_name] = constant_value;
				}
				lua_pop(L, 2);
			}
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 2);
}

void ModApiGraphics::read_shader_info(lua_State *L, ShaderInfo &info)
{
	info.vertex_shader = getstringfield_default(L, -1, "vertex", "opengl_vertex.glsl");
	info.geometry_shader = getstringfield_default(L, -1, "geometry", "opengl_geometry.glsl");
	info.fragment_shader = getstringfield_default(L, -1, "fragment", "opengl_fragment.glsl");

	read_constants(L, info.constants);

	getstringlistfield(L, -1, "vertex_includes", &info.vertex_includes);
	getstringlistfield(L, -1, "fragment_includes", &info.fragment_includes);

	std::unordered_map<std::string, scene::VertexDescriptor> mapNameToDesc = {
		{"vertex3d", scene::Vertex3D::FORMAT},
		{"vertex2tcoords", scene::Vertex2TCoords::FORMAT},
		{"vertextangents", scene::VertexTangents::FORMAT},
		{"vertex2d", scene::Vertex2D::FORMAT},
		{"vertex3dext", scene::Vertex3DExt::FORMAT}
	};
	info.vertex_desc = mapNameToDesc[getstringfield_default(L, -1, "vertex_type", "vertex3d")];
}

int ModApiGraphics::l_register_material(lua_State *L)
{
	if (!lua_isstring(L, 1) || !lua_istable(L, 2))
		return 0;

	std::string name = readParam<std::string>(L, 1);

	lua_getfield(L, 2, "shader");

	if (lua_istable(L, -1)) {
		ShaderInfo info = {name};
		read_shader_info(L, info);

		u32 id = getClient(L)->getShaderSource()->getShader(info, false);

		const_cast<NodeDefManager *>(getClient(L)->getNodeDefManager())->overrideShaderMaterials(name, id);
	}

	lua_pop(L, 1);

	return 1;
}

void ModApiGraphics::Initialize(lua_State *L, int top)
{
	API_FCT(register_material);
}
