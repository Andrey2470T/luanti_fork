#include "l_graphics.h"
#include "client/core/client.h"
#include "client/media/shader.h"
#include "client/media/texturesource.h"
#include "client/render/renderingengine.h"
#include "client/player/localplayer.h"
#include "nodedef.h"
#include "common/c_converter.h"
#include "l_internal.h"
#include "common/c_content.h"
#include "common/c_gfx_content.h"

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
		auto val = readParam<core::matrix4>(L, 4);
		ref->setUniform4x4Matrix(name, val);
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

void ModApiGraphics::read_basic_state(lua_State *L, MaterialStorageEntry &entry)
{
	// Blend
	lua_getfield(L, 2, "blend");

	if (lua_istable(L, -1)) {
		entry.StateFlags |= MF_BLEND;

		bool enable = getboolfield_default(L, -1, "enable", false);

		std::unordered_map<std::string, video::E_BLEND_MODE> mapNameToBM = {
			{"alpha", video::EBM_ALPHA}, {"add", video::EBM_ADD},
			{"subtract", video::EBM_SUBTRACT}, {"revsubtract", video::EBM_REVSUBTRACT},
			{"multiply", video::EBM_MULTIPLY}, {"screen", video::EBM_SCREEN}
		};
		if (!enable)
			entry.Blend = video::EBM_NONE;
		else {
			std::string mode = getstringfield_default(L, -1, "mode", "alpha");
			entry.Blend = mapNameToBM[mode];
		}
	}

	lua_pop(L, 1);

	std::unordered_map<std::string, video::E_COMPARISON_FUNC> mapNameToCF = {
		{"lessequal", video::ECFN_LESSEQUAL}, {"equal", video::ECFN_EQUAL},
		{"less", video::ECFN_LESS}, {"notequal", video::ECFN_NOTEQUAL},
		{"greaterequal", video::ECFN_GREATEREQUAL}, {"greater", video::ECFN_GREATER},
		{"always", video::ECFN_ALWAYS}, {"never", video::ECFN_NEVER}
	};

	// Depth
	lua_getfield(L, 2, "depth");

	if (lua_istable(L, -1)) {
		entry.StateFlags |= MF_DEPTH;

		bool enable = getboolfield_default(L, -1, "enable", false);

		if (!enable)
			entry.Depth = video::ECFN_DISABLED;
		else {
			std::string func = getstringfield_default(L, -1, "func", "less");
			entry.Depth = mapNameToCF[func];
		}
	}

	lua_pop(L, 1);

	// Stencil
	lua_getfield(L, 2, "stencil");

	if (lua_istable(L, -1)) {
		entry.StateFlags |= MF_STENCIL;

		bool enable = getboolfield_default(L, -1, "enable", false);

		if (!enable)
			entry.Depth = video::ECFN_DISABLED;
		else {
			std::string func = getstringfield_default(L, -1, "func", "less");
			entry.Depth = mapNameToCF[func];
		}
	}

	lua_pop(L, 1);

	// Culling
	lua_getfield(L, 2, "cull");

	if (lua_istable(L, -1)) {
		entry.StateFlags |= MF_CULL;

		entry.Cull.enable = getboolfield_default(L, -1, "enable", false);

		if (entry.Cull.enable) {
			std::string mode = getstringfield_default(L, -1, "mode", "back");

			std::unordered_map<std::string, video::E_CULL_MODE> mapNameToCM = {
				{"back", video::ECM_BACK}, {"front", video::ECM_FRONT}, {"back_and_front", video::ECM_FRONT_AND_BACK}
			};

			entry.Cull.mode = mapNameToCM[mode];
		}
	}

	lua_pop(L, 1);
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

void ModApiGraphics::read_shader_info(lua_State *L, ShaderInfo &info, bool &applyShadows)
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

	applyShadows = getboolfield_default(L, -1, "apply_shadows", false);

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

	// Blend, depth, stencil and cull tables
	read_basic_state(L, entry);

	// Shader table
	lua_getfield(L, 2, "shader");

	if (lua_istable(L, -1)) {
		ShaderInfo info = {name};
		info.transparent = entry.Blend == video::EBM_ALPHA; // enable/disable transparency, otherwise no alpha blending
		bool apply_shadows;
		read_shader_info(L, info, apply_shadows);

		entry.ShaderID = getClient(L)->getShaderSource()->getShader(info, apply_shadows);
	}

	lua_pop(L, 1);

	// Table with secondary textures
	std::vector<std::string> textures;
	getstringlistfield(L, 2, "samplers", &textures);

	auto texSrc = getClient(L)->getTextureSource();
	if (!textures.empty()) {
		for (const auto &texName : textures)
		entry.Samplers.emplace_back(texSrc->getTexture(texName));
	}

	mat_st->addMaterial(entry);

	return 1;
}

int ModApiGraphics::l_set_lighting(lua_State *L)
{
	auto &lighting = getClient(L)->getEnv().getLocalPlayer()->getLighting();

	// reset all params
	if (!lua_istable(L, 1)) {
		static Lighting default_lighting;
		lighting = default_lighting;
		return 1;
	}

	lua_getfield(L, 1, "skycolors");
	if (lua_istable(L, -1)) {
		getcolorfield(L, -1, "night", lighting.skycolors.night);
		getcolorfield(L, -1, "sunrise", lighting.skycolors.sunrise);
		getcolorfield(L, -1, "day", lighting.skycolors.day);
		getcolorfield(L, -1, "sunset", lighting.skycolors.sunset);
	}
	lua_pop(L, 1);

	getcolorfield(L, 1, "ambient_color", lighting.ambient_color);

	read_lighting(L, 1, lighting);

	return 1;
}

int ModApiGraphics::l_get_lighting(lua_State *L)
{
	const auto &lighting = getClient(L)->getEnv().getLocalPlayer()->getLighting();

	lua_newtable(L); // result

	lua_newtable(L); // "skycolors"
	push_ARGB8(L, lighting.skycolors.night);
	lua_setfield(L, -2, "night");
	push_ARGB8(L, lighting.skycolors.sunrise);
	lua_setfield(L, -2, "sunrise");
	push_ARGB8(L, lighting.skycolors.day);
	lua_setfield(L, -2, "day");
	push_ARGB8(L, lighting.skycolors.sunset);
	lua_setfield(L, -2, "sunset");
	lua_setfield(L, -2, "skycolors");

	push_ARGB8(L, lighting.ambient_color); // ambient color
	lua_setfield(L, -2, "ambient_color");

	push_lighting(L, lighting);

	return 1;
}

int ModApiGraphics::l_set_sky(lua_State *L)
{
	auto &skybox_params = getClient(L)->getEnv().getLocalPlayer()->getSkyboxParams();
	// reset all params
	if (!lua_istable(L, 1)) {
		skybox_params = SkyboxDefaults::getSkyDefaults();
		return 1;
	}

	read_skyboxparams(L, 1, skybox_params);

	return 1;
}

int ModApiGraphics::l_get_sky(lua_State *L)
{
	const auto &skybox_params = getClient(L)->getEnv().getLocalPlayer()->getSkyboxParams();
	push_skyboxparams(L, skybox_params);

	return 1;
}

int ModApiGraphics::l_set_sun(lua_State *L)
{
	auto &sun_params = getClient(L)->getEnv().getLocalPlayer()->getSunParams();
	// reset all params
	if (!lua_istable(L, 1)) {
		sun_params = SkyboxDefaults::getSunDefaults();
		return 1;
	}

	read_sunparams(L, 1, sun_params);

	return 1;
}

int ModApiGraphics::l_get_sun(lua_State *L)
{
	const auto &sun_params = getClient(L)->getEnv().getLocalPlayer()->getSunParams();
	push_sunparams(L, sun_params);

	return 1;
}

int ModApiGraphics::l_set_moon(lua_State *L)
{
	auto &moon_params = getClient(L)->getEnv().getLocalPlayer()->getMoonParams();
	// reset all params
	if (!lua_istable(L, 1)) {
		moon_params = SkyboxDefaults::getMoonDefaults();
		return 1;
	}

	read_moonparams(L, 1, moon_params);

	return 1;
}

int ModApiGraphics::l_get_moon(lua_State *L)
{
	const auto &moon_params = getClient(L)->getEnv().getLocalPlayer()->getMoonParams();
	push_moonparams(L, moon_params);

	return 1;
}

int ModApiGraphics::l_set_stars(lua_State *L)
{
	auto &star_params = getClient(L)->getEnv().getLocalPlayer()->getStarParams();
	// reset all params
	if (!lua_istable(L, 1)) {
		star_params = SkyboxDefaults::getStarDefaults();
		return 1;
	}

	read_starparams(L, 1, star_params);

	return 1;
}

int ModApiGraphics::l_get_stars(lua_State *L)
{
	const auto &star_params = getClient(L)->getEnv().getLocalPlayer()->getStarParams();
	push_starparams(L, star_params);

	return 1;
}

int ModApiGraphics::l_set_clouds(lua_State *L)
{
	auto &cloud_params = getClient(L)->getEnv().getLocalPlayer()->getCloudParams();
	// reset all params
	if (!lua_istable(L, 1)) {
		cloud_params = SkyboxDefaults::getCloudDefaults();
		return 1;
	}

	read_cloudparams(L, 1, cloud_params);

	return 1;
}

int ModApiGraphics::l_get_clouds(lua_State *L)
{
	const auto &cloud_params = getClient(L)->getEnv().getLocalPlayer()->getCloudParams();
	push_cloudparams(L, cloud_params);

	return 1;
}

int ModApiGraphics::l_override_day_night_ratio(lua_State *L)
{
	bool do_override = false;
	float ratio = 0.0f;

	if (!lua_isnoneornil(L, 2)) {
		do_override = true;
		ratio = readParam<float>(L, 2);
		luaL_argcheck(L, ratio >= 0.0f && ratio <= 1.0f, 1,
			"value must be between 0 and 1");
	}

	getClient(L)->getEnv().getLocalPlayer()->overrideDayNightRatio(do_override, ratio);

	return 1;
}

int ModApiGraphics::l_get_day_night_ratio(lua_State *L)
{
	bool do_override;
	float ratio;
	getClient(L)->getEnv().getLocalPlayer()->getDayNightRatio(&do_override, &ratio);

	if (do_override)
		lua_pushnumber(L, ratio);
	else
		lua_pushnil(L);

	return 1;
}

void ModApiGraphics::read_texture_def(lua_State *L, TextureBufferDefinition &texdef)
{
	std::string type = getstringfield_default(L, -1, "type", "2d");
	texdef.cubemap = type == "cubemap";

	getstringfield(L, -1, "name", texdef.name);

	lua_getfield(L, -1, "resolution");

	if (lua_istable(L, -1)) {
		u32 w, h;
		getintfield(L, -1, "x", w);
		getintfield(L, -1, "y", h);
		texdef.size = {w, h};
	}
	lua_pop(L, 1);

	std::unordered_map<std::string, video::ECOLOR_FORMAT> mapStrToEnumFormat = {
		{"argb8", video::ECF_A8R8G8B8}, {"rgb8", video::ECF_R8G8B8},
		{"d16", video::ECF_D16}, {"d32", video::ECF_D32}, {"d24s32", video::ECF_D24S8}
	};

	texdef.format = mapStrToEnumFormat[getstringfield_default(L, -1, "format", "argb8")];
	texdef.msaa = getintfield_default(L, -1, "msaa", 0);
}

int ModApiGraphics::l_create_texture_buffer(lua_State *L)
{
	if (!lua_isstring(L, 1) || !lua_istable(L, 2))
		return 0;

	std::string name = readParam<std::string>(L, 1);

	std::vector<TextureBufferDefinition> definitions;

	int t = lua_gettop(L);

	lua_pushnil(L);
	while (lua_next(L, t)) {
		if (lua_istable(L, -1)) {
			TextureBufferDefinition newDef;
			read_texture_def(L, newDef);
			definitions.push_back(newDef);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	auto tbuf = getClient(L)->getRenderingEngine()->getPipeline()->createTextureBuffer(name);

	for (u32 i = 0; i < definitions.size(); i++) {
		auto &def = definitions.at(i);
		tbuf->setTexture(i, def.size, def.name, def.format, false, def.msaa, def.cubemap);
	}

	return 1;
}

void ModApiGraphics::Initialize(lua_State *L, int top)
{
	API_FCT(register_material);
	API_FCT(set_lighting);
	API_FCT(get_lighting);
	API_FCT(set_sky);
	API_FCT(get_sky);
	API_FCT(set_sun);
	API_FCT(get_sun);
	API_FCT(set_moon);
	API_FCT(get_moon);
	API_FCT(set_stars);
	API_FCT(get_stars);
	API_FCT(set_clouds);
	API_FCT(get_clouds);
	API_FCT(override_day_night_ratio);
	API_FCT(get_day_night_ratio);
	API_FCT(create_texture_buffer);

	UniformSetter::Register(L);

	lua_newtable(L);
	lua_setfield(L, top, "uniform_setters");
}
