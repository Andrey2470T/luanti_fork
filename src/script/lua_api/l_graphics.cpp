#include "l_graphics.h"
#include "client/core/client.h"
#include "client/media/shader.h"
#include "client/media/texturesource.h"
#include "nodedef.h"
#include "common/c_converter.h"
#include "l_internal.h"

int UniformSetter::gc_object(lua_State *L) {
	auto *o = *(UniformSetter **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

int UniformSetter::l_set(lua_State *L)
{
	auto ref = getobject(L);
	auto name = readParam<std::string>(L, 2);

	if (name.empty())
		return 0;

	auto type = readParam<std::string>(L, 3);

	if (type.empty())
		return 0;

	if (type == "f32") {
		auto val = readParam<f32>(L, 4);
		ref->setUniformFloat(name, val);
	}
	else if (type == "s32") {
		auto val = readParam<s32>(L, 4);
		ref->setUniformInt(name, val);
	}
	else if (type == "v2f") {
		auto val = readParam<v2f>(L, 4);
		ref->setUniform2Float(name, val);
	}
	else if (type == "v3f") {
		auto val = readParam<v3f>(L, 4);
		ref->setUniform3Float(name, val);
	}
	else if (type == "v2i") {
		auto val = readParam<core::vector2di>(L, 4);
		ref->setUniform2Int(name, val);
	}
	else if (type == "v3i") {
		auto val = readParam<core::vector3di>(L, 4);
		ref->setUniform3Int(name, val);
	}
	else if (type == "mat4") {
		// TODO
	}

	return 1;
}

video::MaterialRenderer *UniformSetter::getobject(lua_State *L)
{
	UniformSetter *ref = checkObject<UniformSetter>(L, 1);
	assert(ref);
	return ref->m_renderer;
}

void UniformSetter::create(lua_State *L,  video::MaterialRenderer *renderer)
{
	auto *o = new UniformSetter(renderer);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void UniformSetter::Register(lua_State *L)
{
	static const luaL_Reg methods[] = {
		luamethod(UniformSetter, set),
		{0, 0}
	};
	static const luaL_Reg metamethods[] = {
		{"__gc", gc_object},
		{0, 0}
	};
	registerClass<UniformSetter>(L, methods, metamethods);
}

const char UniformSetter::className[] = "Setter";


void ModApiGraphics::read_pbr(lua_State *L, PBRTextures &textures)
{
	read_texture(L, "metallic", &textures.Metallic);
	read_texture(L, "roughness", &textures.Roughness);
	read_texture(L, "ambient_occlusion", &textures.AO);
	read_texture(L, "normal", &textures.Normal);
	read_texture(L, "emission", &textures.Emission);
}

void ModApiGraphics::read_texture(lua_State *L, const std::string &name, video::GLTexture **tex)
{
	std::string texname;
	getstringfield(L, -1, name.c_str(), texname);

	if (texname.empty())
		return;

	*tex = getClient(L)->getTextureSource()->getTexture(texname);
}

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

	lua_getfield(L, -1, "on_set_uniforms");

	if (lua_isfunction(L, -1)) {
		lua_getglobal(L, "gfx");
		lua_getfield(L, -1, "uniform_setters");

		lua_pushstring(L, info.name.c_str());
		lua_pushvalue(L, -4);
		lua_settable(L, -3);

		lua_pop(L, 3);
	}
}

int ModApiGraphics::l_register_material(lua_State *L)
{
	if (!lua_isstring(L, 1) || !lua_istable(L, 2))
		return 0;

	std::string name = readParam<std::string>(L, 1);

	auto mat_st = getClient(L)->getMaterialStorage();
	auto foundMat = mat_st->getMaterial(name);

	// Dont allow to re-register
	if (!foundMat.Name.empty())
		return 0;

	MaterialStorageEntry entry;
	entry.Name = name;

	// Shader table
	lua_getfield(L, 2, "shader");

	if (lua_istable(L, -1)) {
		ShaderInfo info = {name};
		read_shader_info(L, info);

		entry.ShaderID = getClient(L)->getShaderSource()->getShader(info, false);

		// It is necessary, because Game::afterContentReceived() is called before the client scripting creation
		const_cast<NodeDefManager *>(getClient(L)->getNodeDefManager())->overrideShaderMaterials(name, entry.ShaderID);
	}

	lua_pop(L, 1);

	// PBR table
	lua_getfield(L, 2, "pbr");

	if (lua_istable(L, -1)) {
		read_pbr(L, entry.PBR);
	}

	lua_pop(L, 1);

	mat_st->addMaterial(entry);

	return 1;
}

void ModApiGraphics::Initialize(lua_State *L, int top)
{
	API_FCT(register_material);

	UniformSetter::Register(L);

	lua_newtable(L);
	lua_setfield(L, top, "uniform_setters");
}
