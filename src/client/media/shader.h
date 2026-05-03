// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2013 Kahrl <kahrl@gmx.net>

#pragma once

#include <Video/MaterialRenderer.h>
#include "irrlichttypes_bloated.h"
#include <mutex>
#include <thread>
#include <string>
#include <map>
#include <variant>
#include "nodedef.h"

/*
	Abstraction for pushing constants (or what we pretend is) into
	shaders. These end up as `#define` prepended to the shader source.
*/

// Shader constants are either an int or a float in GLSL
typedef std::map<std::string, std::variant<int, float>> ShaderConstants;

class IShaderConstantSetter {
public:
	virtual ~IShaderConstantSetter() = default;
	/**
	 * Called when the final shader source is being generated
	 * @param name name of the shader
	 * @param constants current set of constants, free to modify
	 */
	virtual void onGenerate(const std::string &name, ShaderConstants &constants) = 0;
};

/*
	Abstraction for updating uniforms used by shaders
*/

class IShaderUniformSetter {
public:
	virtual ~IShaderUniformSetter() = default;
	virtual void onSetUniforms(video::MaterialRenderer *renderer) = 0;
	virtual void onSetMaterial(video::SMaterial& material)
	{ }
};


class IShaderUniformSetterFactory {
public:
	virtual ~IShaderUniformSetterFactory() = default;
	virtual IShaderUniformSetter* create() = 0;
};


template <typename T, std::size_t count = 1>
class CachedShaderSetting {
	std::string m_name;
	T m_sent[count];
	bool has_been_set = false;
public:
	CachedShaderSetting(const std::string &name) :
		m_name(name)
	{}

	/* Type specializations */

	/*
	 * T2 looks redundant here but it is necessary so the compiler won't
	 * resolve the templates at class instantiation and then fail because
	 * some of these methods don't have valid types (= are not usable).
	 * ref: <https://stackoverflow.com/a/6972771>
	 *
	 * Note: a `bool dummy` template parameter would have been easier but MSVC
	 * does not like that. Also make sure not to define different specializations
	 * with the same parameters, MSVC doesn't like that either.
	 * I extend my thanks to Microsoft®
	 */
#define SPECIALIZE(_type, _count_expr) \
	template<typename T2 = T> \
	std::enable_if_t<std::is_same_v<T, T2> && std::is_same_v<T2, _type> && (_count_expr)>

	SPECIALIZE(f32, count == 1)
	set(const f32 value, video::MaterialRenderer *renderer)
	{
		f32 value_v[count] = {value};
		if (has_been_set && std::equal(m_sent, m_sent + count, value_v))
			return;
		renderer->setUniformFloat(m_name, value);

		std::copy(value_v, value_v + count, m_sent);
		has_been_set = true;
	}
	SPECIALIZE(s32, count == 1)
	set(const s32 value, video::MaterialRenderer *renderer)
	{
		s32 value_v[count] = {value};
		if (has_been_set && std::equal(m_sent, m_sent + count, value_v))
			return;
		renderer->setUniformInt(m_name, value);

		std::copy(value_v, value_v + count, m_sent);
		has_been_set = true;
	}

	SPECIALIZE(f32, count == 2)
	set(const v2f value, video::MaterialRenderer *renderer)
	{
		f32 value_v[count] = {value.X, value.Y};
		if (has_been_set && std::equal(m_sent, m_sent + count, value_v))
			return;
		renderer->setUniform2Float(m_name, value);

		std::copy(value_v, value_v + count, m_sent);
		has_been_set = true;
	}

	SPECIALIZE(f32, count == 3)
	set(const v3f value, video::MaterialRenderer *renderer)
	{
		f32 value_v[count] = {value.X, value.Y, value.Z};
		if (has_been_set && std::equal(m_sent, m_sent + count, value_v))
			return;
		renderer->setUniform3Float(m_name, value);

		std::copy(value_v, value_v + count, m_sent);
		has_been_set = true;
	}
	SPECIALIZE(f32, count == 4)
	set(f32 value[count], video::MaterialRenderer *renderer)
	{
		if (has_been_set && std::equal(m_sent, m_sent + count, value))
			return;
		renderer->setUniform4Float(m_name, value);

		std::copy(value, value + count, m_sent);
		has_been_set = true;
	}

	SPECIALIZE(f32, count == 3 || count == 4)
	set(const video::SColorf value, video::MaterialRenderer *renderer)
	{
		f32 value_v[4] = {value.r, value.g, value.b, value.a};

		if (has_been_set && std::equal(m_sent, m_sent + count, value_v))
			return;
		if constexpr (count == 3)
			renderer->setUniformColorfRGB(m_name, value);
		else
			renderer->setUniformColorfRGBA(m_name, value);

		std::copy(value_v, value_v + count, m_sent);
		has_been_set = true;
	}

	SPECIALIZE(f32, count == 16)
	set(const core::matrix4 &value, video::MaterialRenderer *renderer)
	{
		if (has_been_set && std::equal(m_sent, m_sent + count, value.pointer()))
			return;
		renderer->setUniform4x4Matrix(m_name, value);

		std::copy(value.pointer(), value.pointer() + count, m_sent);
		has_been_set = true;
	}

#undef SPECIALIZE
};

template <typename T, std::size_t count>
class CachedStructShaderSetting {
	std::string m_name;
	T m_sent[count];
	bool has_been_set = false;
	std::array<const std::string, count> m_fields;
public:
	CachedStructShaderSetting(const std::string &name, std::array<const std::string, count> &&fields) :
		m_name(name), m_fields(std::move(fields))
	{}

	void set(const T value[count], video::MaterialRenderer *renderer)
	{
		if (has_been_set && std::equal(m_sent, m_sent + count, value))
			return;

		for (std::size_t i = 0; i < count; i++) {
			std::string uniform_name = std::string(m_name) + "." + m_fields[i];

			renderer->setUniformFloat(uniform_name, value[i]);
		}

		std::copy(value, value + count, m_sent);
		has_been_set = true;
	}
};

/*
	ShaderSource creates and caches shaders.

	A "shader" could more precisely be called a "shader material" and comprises
	a vertex, fragment and optional geometry shader.
	It is uniquely identified by a name, base material and the input constants.
*/

struct ShaderInfo {
	std::string name;
	// Vertex includes
	std::vector<std::string> vertex_includes = {};
	// Fragment includes
	std::vector<std::string> fragment_includes = {};
	bool transparent = false;
	// Vertex shader filename
	std::string vertex_shader = "opengl_vertex.glsl";
	// Geometry shader filename
	std::string geometry_shader = "opengl_geometry.glsl";
	// Fragment shader filename
	std::string fragment_shader = "opengl_fragment.glsl";
	// Input constants
	ShaderConstants constants = {};
	// Vertex includes
	// Vertex Type Descriptor
	scene::VertexDescriptor vertex_desc = scene::Vertex3D::FORMAT;
	// Material ID the shader has received from Irrlicht
	video::E_MATERIAL_TYPE material = video::EMT_SOLID;
	std::string basic_name = "";
};

class ClientScripting;

class ShaderSource {
	ClientScripting *m_script = nullptr;
public:
	ShaderSource();
	~ShaderSource();

	void setScripting(ClientScripting *script)
	{
		m_script = script;
	}
	/**
	 * @brief returns information about an existing shader
	 *
	 * Use this to get the material ID to plug into `video::SMaterial`.
	 */
	const ShaderInfo& getShaderInfo(u32 id);

	/**
	 * Generates or gets a shader.
	 *
	 * Note that the input constants are not for passing the entire world into
	 * the shader. Use `IShaderConstantSetter` to handle user settings.
	 * @param name name of the shader (directory on disk)
	 * @param input_const primary key constants for this shader
	 * @return shader ID
	 */
	u32 getShader(const ShaderInfo &info, bool apply_shadows=false, bool force_recompile=false);

	/// @brief Helper: Generates or gets a shader suitable for nodes and entities
	u32 getShader(
		const ShaderInfo &info, MaterialType material_type,
		NodeDrawType drawtype=NDT_NORMAL, bool apply_shadows=true, bool force_recompile=false);

	void insertSourceShader(const std::string &name_of_shader,
		const std::string &filename, const std::string &program);

	std::string getOrLoadSource(
		const std::string &name_of_shader,
		const std::string &rel_path,
		const std::string &filename);

	void addShaderConstantSetter(IShaderConstantSetter *setter);
	void addShaderUniformSetterFactory(IShaderUniformSetterFactory *setter);

	static std::string getShaderPath(const std::string &name_of_shader, const std::string &filename);

	friend class ShaderGenerator;
	friend class ShaderCallback;

private:
	std::thread::id m_main_thread;
	std::mutex m_shader_mutex;
	std::vector<ShaderInfo> m_shaders;

	// Source cache (simple map)
	std::map<std::string, std::string> m_source_cache;

	// Setters
	std::vector<std::unique_ptr<IShaderConstantSetter>> m_constant_setters;
	std::vector<std::unique_ptr<IShaderUniformSetterFactory>> m_uniform_factories;
};

void dumpShaderProgram(std::ostream &output_stream,
	const std::string &program_type, std::string_view program);
