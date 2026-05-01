// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2013 Kahrl <kahrl@gmx.net>

#include <fstream>
#include <iterator>
#include "shader.h"
#include "Utils/irr_ptr.h"
#include "debug.h"
#include "filesys.h"
#include "util/container.h"
#include "util/thread.h"
#include "settings.h"
#include <Scene/ICameraSceneNode.h>
#include <Video/MaterialRenderer.h>
#include <Video/IShaderConstantSetCallBack.h>
#include "client/render/renderingengine.h"
#include "gettext.h"
#include "log.h"
#include "gamedef.h"
#include "client/render/tile.h"
#include "config.h"


/*
	ShaderCallback: Sets constants that can be used in shaders
*/

class ShaderCallback : public video::IShaderConstantSetCallBack
{
	std::vector<std::unique_ptr<IShaderUniformSetter>> m_setters;

public:
	template <typename Factories>
	ShaderCallback(const Factories &factories)
	{
		for (auto &&factory : factories) {
			auto *setter = factory->create();
			if (setter)
				m_setters.emplace_back(setter);
		}
	}

	virtual void OnSetUniforms(video::MaterialRenderer *renderer) override
	{
		for (auto &&setter : m_setters)
			setter->onSetUniforms(renderer);
	}

	virtual void OnSetMaterial(const video::SMaterial& material) override
	{
		for (auto &&setter : m_setters)
			setter->onSetMaterial(material);
	}
};


/*
	MainShaderConstantSetter: Sets some random general constants
*/

class MainShaderConstantSetter : public IShaderConstantSetter
{
public:
	MainShaderConstantSetter() = default;
	~MainShaderConstantSetter() = default;

	void onGenerate(const std::string &name, ShaderConstants &constants) override
	{
		constants["ENABLE_TONE_MAPPING"] = g_settings->getBool("tone_mapping") ? 1 : 0;

		if (g_settings->getBool("enable_dynamic_shadows")) {
			constants["ENABLE_DYNAMIC_SHADOWS"] = 1;
			if (g_settings->getBool("shadow_map_color"))
				constants["COLORED_SHADOWS"] = 1;

			if (g_settings->getBool("shadow_poisson_filter"))
				constants["POISSON_FILTER"] = 1;

			if (g_settings->getBool("enable_water_reflections"))
				constants["ENABLE_WATER_REFLECTIONS"] = 1;

			if (g_settings->getBool("enable_translucent_foliage"))
				constants["ENABLE_TRANSLUCENT_FOLIAGE"] = 1;

			// FIXME: The node specular effect is currently disabled due to mixed in-game
			// results. This shader should not be applied to all nodes equally. See #15898
			if (false)
				constants["ENABLE_NODE_SPECULAR"] = 1;

			s32 shadow_filter = g_settings->getS32("shadow_filters");
			constants["SHADOW_FILTER"] = shadow_filter;

			float shadow_soft_radius = std::max(1.f,
				g_settings->getFloat("shadow_soft_radius"));
			constants["SOFTSHADOWRADIUS"] = shadow_soft_radius;
		}

		if (g_settings->getBool("enable_bloom")) {
			constants["ENABLE_BLOOM"] = 1;
			if (g_settings->getBool("enable_bloom_debug"))
				constants["ENABLE_BLOOM_DEBUG"] = 1;
		}

		if (g_settings->getBool("enable_auto_exposure"))
			constants["ENABLE_AUTO_EXPOSURE"] = 1;

		if (g_settings->get("antialiasing") == "ssaa") {
			constants["ENABLE_SSAA"] = 1;
			u16 ssaa_scale = std::max<u16>(2, g_settings->getU16("fsaa"));
			constants["SSAA_SCALE"] = ssaa_scale;
		}

		if (g_settings->getBool("debanding"))
			constants["ENABLE_DITHERING"] = 1;

		if (g_settings->getBool("enable_volumetric_lighting"))
			constants["VOLUMETRIC_LIGHT"] = 1;
	}
};


/*
	MainShaderUniformSetter: Set basic uniforms required for almost everything
*/

class MainShaderUniformSetter : public IShaderUniformSetter
{
	using SamplerLayer_t = s32;

	CachedShaderSetting<f32, 16> m_world_view_proj{"mWorldViewProj"};
	CachedShaderSetting<f32, 16> m_world{"mWorld"};

	// Modelview matrix
	CachedShaderSetting<f32, 16> m_world_view{"mWorldView"};
	// Texture matrix
	CachedShaderSetting<f32, 16> m_texture{"mTexture"};

	CachedShaderSetting<SamplerLayer_t> m_texture0{"texture0"};
	CachedShaderSetting<SamplerLayer_t> m_texture1{"texture1"};
	CachedShaderSetting<SamplerLayer_t> m_texture2{"texture2"};
	CachedShaderSetting<SamplerLayer_t> m_texture3{"texture3"};

	// commonly used way to pass material color to shader
	video::SColor m_material_color;
	CachedShaderSetting<f32, 4> m_material_color_setting{"materialColor"};

public:
	~MainShaderUniformSetter() = default;

	virtual void onSetMaterial(const video::SMaterial& material) override
	{
		m_material_color = material.ColorParam;
	}

	virtual void onSetUniforms(video::MaterialRenderer *renderer) override
	{
		video::VideoDriver *driver = renderer->getVideoDriver();
		assert(driver);

		// Set world matrix
		core::matrix4 world = driver->getTransform(video::ETS_WORLD);
		m_world.set(world, renderer);

		// Set clip matrix
		core::matrix4 worldView;
		worldView = driver->getTransform(video::ETS_VIEW);
		worldView *= world;

		core::matrix4 worldViewProj;
		worldViewProj = driver->getTransform(video::ETS_PROJECTION);
		worldViewProj *= worldView;
		m_world_view_proj.set(worldViewProj, renderer);

		auto &texture = driver->getTransform(video::ETS_TEXTURE_0);
		m_world_view.set(worldView, renderer);
		m_texture.set(texture, renderer);

		SamplerLayer_t tex_id;
		tex_id = 0;
		m_texture0.set(tex_id, renderer);
		tex_id = 1;
		m_texture1.set(tex_id, renderer);
		tex_id = 2;
		m_texture2.set(tex_id, renderer);
		tex_id = 3;
		m_texture3.set(tex_id, renderer);

		video::SColorf colorf(m_material_color);
		m_material_color_setting.set(colorf, renderer);
	}
};


class MainShaderUniformSetterFactory : public IShaderUniformSetterFactory
{
public:
	virtual IShaderUniformSetter* create()
		{ return new MainShaderUniformSetter(); }
};


class ShaderGenerator
{
	ShaderSource *src;
public:
	ShaderInfo info;

	ShaderGenerator(ShaderSource *_src, const ShaderInfo &_info)
		: src(_src), info(_info)
	{}

	void generate();
private:
	std::string readIncludeShader(const std::string &name);
	std::string generateMainHeader();
	std::string generateVertexHeader();
	std::string generateFragmentHeader();

	/// @brief outputs a constant to an ostream
	inline void putConstant(std::ostream &os, const ShaderConstants::mapped_type &it)
	{
		if (auto *ival = std::get_if<int>(&it); ival)
			os << *ival;
		else
			os << std::get<float>(it);
	}
};


ShaderSource::ShaderSource()
{
	m_main_thread = std::this_thread::get_id();

	// Add a dummy ShaderInfo as the first index, named ""
	m_shaders.emplace_back();

	// Add global stuff
	addShaderConstantSetter(new MainShaderConstantSetter());
	addShaderUniformSetterFactory(new MainShaderUniformSetterFactory());
}

ShaderSource::~ShaderSource()
{
	// Delete materials
	u32 n = 0;
	for (ShaderInfo &i : m_shaders) {
		if (!i.name.empty()) {
			RenderingEngine::get_video_driver()->deleteShaderMaterial(i.material);
			n++;
		}
	}

	infostream << "~ShaderSource() cleaned up " << n << " materials" << std::endl;
}

const ShaderInfo& ShaderSource::getShaderInfo(u32 id) {
	std::lock_guard<std::mutex> lock(m_shader_mutex);
	static ShaderInfo empty;
	if (id >= m_shaders.size())
		return empty;
	return m_shaders[id];
}

u32 ShaderSource::getShader(const ShaderInfo &info, bool apply_shadows, bool force_recompile)
{
	// Empty name means shader 0
	if (info.name.empty()) {
		infostream<<"getShaderIdDirect(): name is empty"<<std::endl;
		return 0;
	}

	ShaderInfo info_c = info;

	if (apply_shadows && g_settings->getBool("enable_dynamic_shadows")) {
		info_c.vertex_includes.insert(
			info_c.vertex_includes.end(), {"shadow_uniforms", "shadow_vertex"});
		info_c.fragment_includes.insert(
			info_c.fragment_includes.end(), {"shadow_uniforms", "shadow_common", "shadow_depth", "shadow_filter"});
	}

	// Check if already have such instance
	u32 i = 0;
	for (; i < m_shaders.size(); i++) {
		auto &curInfo = m_shaders[i];
		// The shader name must be unique
		if (curInfo.name == info_c.name) {
			if (force_recompile)
				break;
			else
				return i;
		}
	}

	if (std::this_thread::get_id() == m_main_thread) {
		/* Let the constant setters do their job and emit constants */
		for (auto &setter : m_constant_setters)
			setter->onGenerate(info_c.name, info_c.constants);

		ShaderGenerator gen(this, info_c);
		gen.generate();

		if (force_recompile && i < m_shaders.size())
			m_shaders.at(i) = gen.info;
		else {
			m_shaders.emplace_back(gen.info);
			i = m_shaders.size()-1;
		}

		return i;
	}

	errorstream << "ShaderSource::getShader(): generating a new shader material from "
		"other thread not implemented" << std::endl;

	return 0;
}

u32 ShaderSource::getShader(
	const ShaderInfo &info, MaterialType material_type,
	NodeDrawType drawtype, bool apply_shadows, bool force_recompile)
{
	ShaderInfo info_c = info;
	info_c.basic_name = info.name;
	info_c.name += "_";
	info_c.name += std::to_string((s32)material_type);
	info_c.name += "_";
	info_c.name += std::to_string((s32)drawtype);

	info_c.constants["MATERIAL_TYPE"] = (s32)material_type;
	info_c.constants["DRAWTYPE"] = (s32)drawtype;

	video::E_MATERIAL_TYPE base_mat = video::EMT_SOLID;
	switch (material_type) {
		case TILE_MATERIAL_ALPHA:
		case TILE_MATERIAL_PLAIN_ALPHA:
		case TILE_MATERIAL_LIQUID_TRANSPARENT:
		case TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT:
			base_mat = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			break;
		case TILE_MATERIAL_BASIC:
		case TILE_MATERIAL_PLAIN:
		case TILE_MATERIAL_WAVING_LEAVES:
		case TILE_MATERIAL_WAVING_PLANTS:
		case TILE_MATERIAL_WAVING_LIQUID_BASIC:
			base_mat = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
			break;
		default:
			break;
	}

	info_c.base_material = base_mat;

	return getShader(info_c, apply_shadows, force_recompile);
}

// Shaders loaded from remote media know only the files names, not the shaders ones themselves
#define GET_COMBINED_NAME(shader_name, filename) \
	shader_name.empty() ? filename : shader_name + DIR_DELIM + filename;

void ShaderSource::insertSourceShader(
	const std::string &name_of_shader,
	const std::string &filename,
	const std::string &program)
{
	std::string combined = GET_COMBINED_NAME(name_of_shader, filename);
	m_source_cache[combined] = program;
}

std::string ShaderSource::getOrLoadSource(
	const std::string &name_of_shader,
	const std::string &rel_path,
	const std::string &filename)
{
	std::string combined = GET_COMBINED_NAME(name_of_shader, filename);

	auto it = m_source_cache.find(combined);
	if (it != m_source_cache.end())
		return it->second;

	// Then try to find by just a filename if this shader was sent from server
	it = m_source_cache.find(filename);
	if (it != m_source_cache.end())
		return it->second;

	std::string path = getShaderPath(rel_path, filename);
	if (path.empty())
		return "";

	std::string content;
	fs::ReadFile(path, content, true);

	if (!content.empty())
		m_source_cache[combined] = content;

	return content;
}

void ShaderSource::addShaderConstantSetter(IShaderConstantSetter *setter) {
	m_constant_setters.emplace_back(setter);
}

void ShaderSource::addShaderUniformSetterFactory(IShaderUniformSetterFactory *factory) {
	m_uniform_factories.emplace_back(factory);
}

std::string ShaderSource::getShaderPath(
	const std::string &name_of_shader, const std::string &filename)
{
	std::string combined = name_of_shader + DIR_DELIM + filename;

	std::vector<std::string> default_paths = {
		g_settings->get("shader_path"),
		porting::path_share + DIR_DELIM + "client" + DIR_DELIM + "shaders"
	};

	for (const auto &path : default_paths) {
		std::string fullpath = path + DIR_DELIM + combined;
		if (fs::PathExists(fullpath))
			return fullpath;
	}

	return "";
}

std::string ShaderGenerator::readIncludeShader(const std::string &name)
{
	return src->getOrLoadSource("", "includes", name + ".glsl");
}

std::string ShaderGenerator::generateMainHeader()
{
	std::ostringstream shaders_header;
	shaders_header
		<< std::noboolalpha
		<< std::showpoint; // for GLSL ES


	if (RenderingEngine::get_video_driver()->getDriverType() == video::EDT_OPENGL3) {
		shaders_header << "#version 150\n";
	} else {
		shaders_header << "#version 100\n";
	}

	for (auto &it : info.constants) {
		// spaces could cause duplicates
		assert(trim(it.first) == it.first);
		shaders_header << "#define " << it.first << ' ';
		putConstant(shaders_header, it.second);
		shaders_header << '\n';
	}

	return shaders_header.str();
}

std::string ShaderGenerator::generateVertexHeader()
{
	std::string header;
	header = R"(
		precision mediump float;

		uniform highp mat4 mWorldView;
		uniform highp mat4 mWorldViewProj;
		uniform mediump mat4 mTexture;
	)";

	// Generate vertex inputs
	for (auto &attr : info.vertex_desc.Attributes) {
		std::string inAttr = "in ";

		if (attr.Count == 1) {
			switch (attr.Type) {
			case scene::VertexAttribute::Type::FLOAT:
				inAttr += "float ";
				break;
			case scene::VertexAttribute::Type::INT:
				inAttr += "int ";
				break;
			default:
				break;
			}
		}
		else {
			assert(attr.Count <= 4);

			inAttr += "vec";
			inAttr += std::to_string(attr.Count);

			if (attr.Type == scene::VertexAttribute::Type::INT)
				inAttr += "i";
			inAttr += " ";
		}

		inAttr += attr.Name;
		inAttr += ";\n";

		header += inAttr;
	}

	// Our vertex color has components reversed compared to what OpenGL
	// normally expects, so we need to take that into account.
	header += "#define inColor (inColor.bgra)\n";

	// Append the files contents from /include shader subpath
	for (auto &include : info.vertex_includes) {
		auto content = readIncludeShader(include);

		if (content.empty())
			continue;

		header += content;
		header += "\n";
	}

	return header;
}

std::string ShaderGenerator::generateFragmentHeader()
{
	std::string header;
	header = R"(
		precision mediump float;
	)";

	// map legacy semantic texture names to texture identifiers
	header += R"(
		#define baseTexture texture0
		#define normalTexture texture1
		#define textureFlags texture2
	)";

	// Allow for multiple color outputs

	u8 colorAttachments = RenderingEngine::get_video_driver()->getFeatures().ColorAttachment;
	for (u8 k = 0; k < colorAttachments; k++) {
		std::string outColor = "out vec4 outColor";
		outColor += std::to_string(k);
		outColor += ";\n";
		header += outColor;
	}

	// Append the files contents from /include shader subpath
	for (auto &include : info.fragment_includes) {
		auto content = readIncludeShader(include);

		if (content.empty())
			continue;

		header += content;
		header += "\n";
	}

	return header;
}

void ShaderGenerator::generate()
{
	// fixed pipeline materials don't make sense here
	assert(info.base_material != video::EMT_TRANSPARENT_VERTEX_ALPHA &&
			info.base_material != video::EMT_ONETEXTURE_BLEND);
	info.material = info.base_material;

	if (info.basic_name.empty())
		info.basic_name = info.name;

	if (info.base_material == video::EMT_TRANSPARENT_ALPHA_CHANNEL)
		info.constants["USE_DISCARD"] = 1;
	else if (info.base_material == video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF)
		info.constants["USE_DISCARD_REF"] = 1;

	auto main_header = generateMainHeader();
	auto vertex_header = generateVertexHeader();
	auto fragment_header = generateFragmentHeader();
	std::string geometry_header = "";

	/// Unique name of this shader, for debug/logging
	std::string log_name = info.name;
	for (auto &it : info.constants) {
		if (log_name.size() > 60) { // it shouldn't be too long
			log_name.append("...");
			break;
		}
		std::ostringstream oss;
		putConstant(oss, it.second);
		log_name.append(" ").append(it.first).append("=").append(oss.str());
	}

	const char *final_header = "#line 0\n"; // reset the line counter for meaningful diagnostics

	std::string vertex_shader = src->getOrLoadSource(info.name, info.basic_name, info.vertex_shader);
	std::string fragment_shader = src->getOrLoadSource(info.name, info.basic_name, info.fragment_shader);
	std::string geometry_shader = src->getOrLoadSource(info.name, info.basic_name, info.geometry_shader);

	vertex_shader = main_header + vertex_header + final_header + vertex_shader;
	fragment_shader = main_header + fragment_header + final_header + fragment_shader;

	if (!geometry_shader.empty()) {
		geometry_shader = main_header + geometry_header + final_header + geometry_shader;
	}

	auto cb = make_irr<ShaderCallback>(src->m_uniform_factories);
	infostream << "Compiling high level shaders for " << log_name << std::endl;
	s32 shadermat = RenderingEngine::get_video_driver()->addHighLevelShaderMaterial(
		vertex_shader, fragment_shader, geometry_shader,
		log_name, scene::EPT_TRIANGLES, scene::EPT_TRIANGLES, 0,
		cb.get(), info.base_material, info.vertex_desc);
	if (shadermat == -1) {
		errorstream << "generateShader(): failed to generate shaders for "
			<< log_name << ", addHighLevelShaderMaterial failed." << std::endl;
		dumpShaderProgram(warningstream, "Vertex", vertex_shader);
		dumpShaderProgram(warningstream, "Fragment", fragment_shader);
		dumpShaderProgram(warningstream, "Geometry", geometry_shader);
		throw ShaderException(
			fmtgettext("Failed to compile the \"%s\" shader.", log_name.c_str()) +
			strgettext("\nCheck debug.txt for details."));
	}

	// Apply the newly created material type
	info.material = (video::E_MATERIAL_TYPE) shadermat;
}

void dumpShaderProgram(std::ostream &output_stream,
		const std::string &program_type, std::string_view program)
{
	output_stream << program_type << " shader program:" << std::endl <<
		"----------------------------------" << std::endl;
	size_t pos = 0;
	size_t prev = 0;
	s16 line = 1;
	while ((pos = program.find('\n', prev)) != std::string::npos) {
		output_stream << line++ << ": "<< program.substr(prev, pos - prev) <<
			std::endl;
		prev = pos + 1;
	}
	output_stream << line << ": " << program.substr(prev) << std::endl <<
		"End of " << program_type << " shader program." << std::endl <<
		" " << std::endl;
}
