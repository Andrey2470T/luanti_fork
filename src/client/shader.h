// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2013 Kahrl <kahrl@gmx.net>

#pragma once

#include <MaterialRenderer.h>
#include "irrlichttypes_bloated.h"
#include <string>
#include <map>
#include <variant>
#include "nodedef.h"

/*
	shader.{h,cpp}: Shader handling stuff.
*/

/*
	Gets the path to a shader by first checking if the file
	  name_of_shader/filename
	exists in shader_path and if not, using the data path.

	If not found, returns "".

	Utilizes a thread-safe cache.
*/
std::string getShaderPath(const std::string &name_of_shader,
		const std::string &filename);

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
	virtual void onSetMaterial(const video::SMaterial& material)
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
	set(const f32 value[count], video::MaterialRenderer *renderer)
	{
		if (has_been_set && std::equal(m_sent, m_sent + count, value))
			return;
		renderer->setUniformFloatArray(m_name, {value[0], value[1], value[2], value[3]});

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
	video::E_MATERIAL_TYPE base_material = video::EMT_SOLID;
	// Material ID the shader has received from Irrlicht
	video::E_MATERIAL_TYPE material = video::EMT_SOLID;
	// Input constants
	ShaderConstants input_constants;
};

class IShaderSource {
public:
	IShaderSource() = default;
	virtual ~IShaderSource() = default;

	/**
	 * @brief returns information about an existing shader
	 *
	 * Use this to get the material ID to plug into `video::SMaterial`.
	 */
	virtual const ShaderInfo &getShaderInfo(u32 id) = 0;

	/**
	 * Generates or gets a shader.
	 *
	 * Note that the input constants are not for passing the entire world into
	 * the shader. Use `IShaderConstantSetter` to handle user settings.
	 * @param name name of the shader (directory on disk)
	 * @param input_const primary key constants for this shader
	 * @param base_mat base material to use
	 * @return shader ID
	 * @note `base_material` only controls alpha behavior
	 */
	virtual u32 getShader(const std::string &name,
		const ShaderConstants &input_const, video::E_MATERIAL_TYPE base_mat) = 0;

	/// @brief Helper: Generates or gets a shader suitable for nodes and entities
	u32 getShader(const std::string &name,
		MaterialType material_type, NodeDrawType drawtype = NDT_NORMAL);

	/**
	 * Helper: Generates or gets a shader for common, general use.
	 * @param name name of the shader
	 * @param blendAlpha enable alpha blending for this material?
	 * @return shader ID
	 */
	inline u32 getShaderRaw(const std::string &name, bool blendAlpha = false)
	{
		auto base_mat = blendAlpha ? video::EMT_TRANSPARENT_ALPHA_CHANNEL :
			video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		return getShader(name, ShaderConstants(), base_mat);
	}
};

class IWritableShaderSource : public IShaderSource {
public:
	IWritableShaderSource() = default;
	virtual ~IWritableShaderSource() = default;

	virtual void processQueue()=0;
	virtual void insertSourceShader(const std::string &name_of_shader,
		const std::string &filename, const std::string &program)=0;
	virtual void rebuildShaders()=0;

	/// @note Takes ownership of @p setter.
	virtual void addShaderConstantSetter(IShaderConstantSetter *setter) = 0;

	/// @note Takes ownership of @p setter.
	virtual void addShaderUniformSetterFactory(IShaderUniformSetterFactory *setter) = 0;
};

IWritableShaderSource *createShaderSource();

void dumpShaderProgram(std::ostream &output_stream,
	const std::string &program_type, std::string_view program);
