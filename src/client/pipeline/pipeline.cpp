// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 x2048, Dmitry Kostenko <codeforsmile@gmail.com>

#include "pipeline.h"
#include "client/core/client.h"
#include "client/ui/hud.h"
#include "Video/RenderTarget.h"
#include "Image/SColor.h"

#include <vector>
#include <memory>


TextureBuffer::~TextureBuffer()
{
	for (u32 index = 0; index < m_textures.size(); index++)
		m_driver->removeTexture(m_textures[index]);
	m_textures.clear();
}

video::GLTexture *TextureBuffer::getTexture(u8 index)
{
	if (index >= m_textures.size())
		return nullptr;
	return m_textures[index];
}

const TextureBufferDefinition &TextureBuffer::getTextureDef(u8 index)
{
	static TextureBufferDefinition default_;
	if (index >= m_definitions.size())
		return default_;
	return m_definitions[index];
}


void TextureBuffer::setTexture(
	u8 index, core::dimension2du size, const std::string &name, video::ECOLOR_FORMAT format,
	bool clear, u8 msaa, bool cubemap)
{
	assert(index != NO_DEPTH_TEXTURE);

	if (m_definitions.size() <= index)
		m_definitions.resize(index + 1);

	auto &definition = m_definitions[index];
	definition.valid = true;
	definition.dirty = true;
	definition.fixed_size = true;
	definition.size = size;
	definition.name = name;
	definition.format = format;
	definition.clear = clear;
	definition.msaa = msaa;
	definition.cubemap = cubemap;
}

void TextureBuffer::setTexture(
	u8 index, v2f scale_factor, const std::string &name, video::ECOLOR_FORMAT format,
	bool clear, u8 msaa, bool cubemap)
{
	assert(index != NO_DEPTH_TEXTURE);

	if (m_definitions.size() <= index)
		m_definitions.resize(index + 1);

	auto &definition = m_definitions[index];
	definition.valid = true;
	definition.dirty = true;
	definition.fixed_size = false;
	definition.scale_factor = scale_factor;
	definition.name = name;
	definition.format = format;
	definition.clear = clear;
	definition.msaa = msaa;
	definition.cubemap = cubemap;
}

void TextureBuffer::reset(PipelineContext &context)
{
	if (!m_driver)
		m_driver = context.device->getVideoDriver();

	// remove extra textures
	if (m_textures.size() > m_definitions.size()) {
		for (unsigned i = m_definitions.size(); i < m_textures.size(); i++)
			if (m_textures[i])
				m_driver->removeTexture(m_textures[i]);

		m_textures.set_used(m_definitions.size());
	}

	// add placeholders for new definitions
	while (m_textures.size() < m_definitions.size())
		m_textures.push_back(nullptr);

	// change textures to match definitions
	for (u32 i = 0; i < m_definitions.size(); i++) {
		video::GLTexture **ptr = &m_textures[i];

		ensureTexture(ptr, m_definitions[i], context);
		m_definitions[i].dirty = false;
	}

	RenderSource::reset(context);
}

void TextureBuffer::swapTextures(u8 texture_a, u8 texture_b)
{
	assert(m_definitions[texture_a].valid && m_definitions[texture_b].valid);

	video::GLTexture *temp = m_textures[texture_a];
	m_textures[texture_a] = m_textures[texture_b];
	m_textures[texture_b] = temp;
}


bool TextureBuffer::ensureTexture(video::GLTexture **texture, const TextureBufferDefinition &definition, PipelineContext &context)
{
	bool modify;
	core::dimension2du size;
	if (definition.valid) {
		if (definition.fixed_size)
			size = definition.size;
		else
			size = core::dimension2du(
					(u32)(context.target_size.X * definition.scale_factor.X),
					(u32)(context.target_size.Y * definition.scale_factor.Y));

		modify = definition.dirty || (*texture == nullptr) || (*texture)->getSize() != size;
	}
	else {
		modify = (*texture != nullptr);
	}

	if (!modify)
		return false;

	if (*texture) {
		m_driver->removeTexture(*texture);
		*texture = nullptr;
	}

	if (definition.valid) {
		if (!m_driver->queryTextureFormat(definition.format)) {
			errorstream << "Failed to create texture \"" << definition.name
				<< "\": unsupported format " << video::pixelFormatsInfo[definition.format].name
				<< std::endl;
			return false;
		}

		if (definition.clear) {
			// We're not able to clear a render target texture
			// We're not able to create a normal texture with MSAA
			// (could be solved by more refactoring in Irrlicht, but not needed for now)
			sanity_check(definition.msaa < 1);

			video::Image *image = new video::Image(definition.format, size);
			// Cannot use image->fill because it's not implemented for all formats.
			std::memset(image->getData(), 0, video::getDataSizeFromFormat(definition.format, size.Width, size.Height));
			*texture = m_driver->addTexture(definition.name.c_str(), image);
			image->drop();
		} else if (definition.msaa > 0) {
			*texture = m_driver->addRenderTargetTextureMs(size, definition.msaa, definition.name.c_str(), definition.format);
		} else {
			if (definition.cubemap)
				*texture = m_driver->addRenderTargetTextureCubemap(size, definition.name.c_str(), definition.format);
			else
				*texture = m_driver->addRenderTargetTexture(size, definition.name.c_str(), definition.format);
		}

		if (!*texture) {
			errorstream << "Failed to create texture \"" << definition.name
				<< "\"" << std::endl;
			return false;
		}
	}

	return true;
}

TextureBufferOutput::TextureBufferOutput(TextureBuffer *_buffer, u8 _texture_index)
	: buffer(_buffer), texture_map({{_texture_index, 0}})
{}

TextureBufferOutput::TextureBufferOutput(TextureBuffer *_buffer, const std::vector<u8> &_texture_map, u8 _depth_stencil)
	: buffer(_buffer), depth_stencil(_depth_stencil, 0)
{
	for (auto &i : _texture_map)
		texture_map.emplace_back(i, 0);
}

TextureBufferOutput::TextureBufferOutput(TextureBuffer *_buffer, const std::vector<std::pair<u8, u8> > &_texture_map)
	: buffer(_buffer), texture_map(_texture_map)
{}

TextureBufferOutput::TextureBufferOutput(TextureBuffer *_buffer, const std::vector<std::pair<u8, u8> > &_texture_map, std::pair<u8, u8> _depth_stencil)
	: buffer(_buffer), texture_map(_texture_map), depth_stencil(_depth_stencil)
{}

TextureBufferOutput::~TextureBufferOutput()
{
	if (render_target && driver)
		render_target->drop();
}

void TextureBufferOutput::activate(PipelineContext &context)
{
	if (!driver)
		driver = context.device->getVideoDriver();

	if (!render_target)
		render_target = new video::RenderTarget(driver);

	std::vector<video::GLTexture *> textures;
	std::vector<video::E_CUBEMAP_FACE> faces;
	core::dimension2du size(0, 0);
	for (auto &p :  texture_map) {
		video::GLTexture *texture = buffer->getTexture(p.first);
		textures.push_back(texture);
		faces.push_back((video::E_CUBEMAP_FACE)p.second);
		if (texture && size.Width == 0)
			size = texture->getSize();
	}

	video::GLTexture *depth_texture = nullptr;
	video::E_CUBEMAP_FACE depth_face = video::ECMF_POS_X;
	if (depth_stencil.first != NO_DEPTH_TEXTURE) {
		depth_texture = buffer->getTexture(depth_stencil.first);
		depth_face = (video::E_CUBEMAP_FACE)depth_stencil.second;
	}

	render_target->setColorTextures(textures, faces);
	render_target->setDepthStencilTexture(depth_texture, depth_face);

	driver->setRenderTargetEx(render_target, m_clear ? video::ECBF_ALL : video::ECBF_NONE, context.clear_color);
	driver->OnResize(size);

	RenderTarget::activate(context);
}

video::RenderTarget *TextureBufferOutput::getIrrRenderTarget(PipelineContext &context)
{
	activate(context); // Needed to make sure that render_target is set up.
	return render_target;
}

u8 DynamicSource::getTextureCount()
{
	assert(isConfigured());
	return upstream->getTextureCount();
}

video::GLTexture *DynamicSource::getTexture(u8 index)
{
	assert(isConfigured());
	return upstream->getTexture(index);
}

void ScreenTarget::activate(PipelineContext &context)
{
	auto driver = context.device->getVideoDriver();
	driver->setRenderTargetEx(nullptr, m_clear ? video::ECBF_ALL : video::ECBF_NONE, context.clear_color);
	driver->OnResize(size);
	RenderTarget::activate(context);
}

void DynamicTarget::activate(PipelineContext &context)
{
	if (!isConfigured())
		throw std::logic_error("Dynamic render target is not configured before activation.");
	upstream->activate(context);
}

void ScreenTarget::reset(PipelineContext &context)
{
	RenderTarget::reset(context);
	size = context.device->getVideoDriver()->getScreenSize();
}

SetRenderTargetStep::SetRenderTargetStep(RenderStep *_step, RenderTarget *_target)
	: step(_step), target(_target)
{
}

void SetRenderTargetStep::run(PipelineContext &context)
{
	step->setRenderTarget(target);
}

SwapTexturesStep::SwapTexturesStep(TextureBuffer *_buffer, u8 _texture_a, u8 _texture_b)
		: buffer(_buffer), texture_a(_texture_a), texture_b(_texture_b)
{
}

void SwapTexturesStep::run(PipelineContext &context)
{
	buffer->swapTextures(texture_a, texture_b);
}

RenderStep *RenderPipeline::addStep(const std::string &name, RenderStep *step)
{
	m_pipeline.emplace_back(name, step);
	return step;
}

RenderStep *RenderPipeline::getStep(const std::string &name, bool recursive)
{
	RenderStep *found = nullptr;

	for (auto &step_p : m_pipeline) {
		if (step_p.first == name) {
			found = step_p.second;
			break;
		}

		if (!recursive)
			continue;
		auto to_pipeline = dynamic_cast<RenderPipeline *>(step_p.second);

		if (to_pipeline) {
			auto nested_found = to_pipeline->getStep(name);

			if (nested_found) {
				found = nested_found;
				break;
			}
		}
	}
	return found;
}

TextureBuffer *RenderPipeline::createTextureBuffer(const std::string &name)
{
	auto tbuf = createOwned<TextureBuffer>();
	m_texture_buffers.emplace(name, tbuf);
	return tbuf;
}

TextureBuffer *RenderPipeline::getTextureBuffer(const std::string &name, bool recursive)
{
	auto found_it = m_texture_buffers.find(name);

	if (found_it != m_texture_buffers.end())
		return found_it->second;
	
	if (!recursive)
		return nullptr;
	for (auto &step_p : m_pipeline) {
		auto to_pipeline = dynamic_cast<RenderPipeline *>(step_p.second);

		if (to_pipeline) {
			auto nested_found = to_pipeline->getTextureBuffer(name);

			if (nested_found)
				return nested_found;
		}
	}

	return nullptr;
}

void RenderPipeline::run(PipelineContext &context)
{
	v2u32 original_size = context.target_size;
	context.target_size = v2u32(original_size.X * scale.X, original_size.Y * scale.Y);

	for (auto &object : m_objects)
		object->reset(context);

	for (auto &step: m_pipeline)
		step.second->run(context);

	context.target_size = original_size;
}

void RenderPipeline::setRenderSource(RenderSource *source)
{
	m_input.setRenderSource(source);
}

void RenderPipeline::setRenderTarget(RenderTarget *target)
{
	m_output.setRenderTarget(target);
}
