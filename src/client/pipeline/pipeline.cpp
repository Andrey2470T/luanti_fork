// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 x2048, Dmitry Kostenko <codeforsmile@gmail.com>

#include "pipeline.h"
#include "client/core/client.h"
#include "client/ui/hud.h"
#include <Render/Texture2D.h>
#include <Render/TextureCubeMap.h>
#include <Render/FrameBuffer.h>
#include <Render/DrawContext.h>
#include "log.h"
#include <Main/MainWindow.h>
#include "client/render/rendersystem.h"
#include "client/render/renderer.h"

TextureBuffer::~TextureBuffer()
{
	for (u32 index = 0; index < m_textures.size(); index++)
        delete m_textures[index];
	m_textures.clear();
}

render::Texture *TextureBuffer::getTexture(u8 index)
{
    return index >= m_textures.size() ? nullptr : m_textures[index];
}


void TextureBuffer::setTexture(u8 index, v2u size, const std::string &name, img::PixelFormat format,
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

void TextureBuffer::setTexture(u8 index, v2f scale_factor, const std::string &name, img::PixelFormat format,
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
	// remove extra textures
	if (m_textures.size() > m_definitions.size()) {
		for (unsigned i = m_definitions.size(); i < m_textures.size(); i++)
			if (m_textures[i])
                delete m_textures[i];

        m_textures.resize(m_definitions.size());
	}

	// add placeholders for new definitions
	while (m_textures.size() < m_definitions.size())
		m_textures.push_back(nullptr);

	// change textures to match definitions
	for (u32 i = 0; i < m_definitions.size(); i++) {
        render::Texture **ptr = &m_textures[i];

		ensureTexture(ptr, m_definitions[i], context);
		m_definitions[i].dirty = false;
	}

	RenderSource::reset(context);
}

void TextureBuffer::swapTextures(u8 texture_a, u8 texture_b)
{
	assert(m_definitions[texture_a].valid && m_definitions[texture_b].valid);
    std::swap(m_textures[texture_a], m_textures[texture_b]);
}


bool TextureBuffer::ensureTexture(render::Texture **texture, const TextureDefinition& definition, PipelineContext &context)
{
	bool modify;
    v2u size;
	if (definition.valid) {
		if (definition.fixed_size)
			size = definition.size;
		else
            size = v2u(
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
        delete *texture;
		*texture = nullptr;
	}

	if (definition.valid) {
        if (!definition.cubemap) {
            if (!definition.clear)
                *texture = (render::Texture *)new render::Texture2D(definition.name, size.X, size.Y, definition.format, definition.msaa);
            else
                *texture = (render::Texture *)new render::Texture2D(definition.name, std::make_unique<img::Image>(definition.format, size.X, size.Y));
        }
        else {
            if (!definition.clear)
                *texture = (render::Texture *)new render::TextureCubeMap(definition.name, size.X, size.Y, definition.format);
            else {
                std::array<img::Image *, render::CMF_COUNT> imgs;

                for (u8 i = 0; i < render::CMF_COUNT; i++)
                    imgs[i] = new img::Image(definition.format, size.X, size.Y);

                *texture = (render::Texture *)new render::TextureCubeMap(definition.name, imgs);
            }
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

TextureBufferOutput::TextureBufferOutput(TextureBuffer *_buffer, const std::unordered_map<u8, u8> &_texture_map)
	: buffer(_buffer), texture_map(_texture_map)
{}

TextureBufferOutput::TextureBufferOutput(TextureBuffer *_buffer, const std::unordered_map<u8, u8> &_texture_map, std::pair<u8, u8> _depth_stencil)
	: buffer(_buffer), texture_map(_texture_map), depth_stencil(_depth_stencil)
{}

void TextureBufferOutput::activate(PipelineContext &context)
{
    if (!render_target) {
        auto window = context.client->getRenderSystem()->getWindow();
        auto glParams = window->getGLParams();
        render_target = std::make_unique<render::FrameBuffer>(glParams.maxColorAttachments);
    }

    std::vector<render::Texture *> textures;
    std::vector<render::CubeMapFace> faces;
    v2u size(0, 0);
    for (auto p : texture_map) {
        auto texture = buffer->getTexture(p.first);
		textures.push_back(texture);
        faces.push_back((render::CubeMapFace)p.second);
        if (texture && size.X == 0)
			size = texture->getSize();
	}

    render::Texture *depth_texture = nullptr;
    render::CubeMapFace depth_face = render::CMF_COUNT;
    if (depth_stencil.first != NO_DEPTH_TEXTURE) {
        depth_texture = buffer->getTexture(depth_stencil.first);
        depth_face = (render::CubeMapFace)depth_stencil.second;
    }

    render_target->setColorTextures(textures, faces);
    render_target->setDepthStencilTexture(depth_texture, depth_face);

    auto ctxt = context.client->getRenderSystem()->getRenderer()->getContext();
    u16 clear = m_clear ? render::CBF_COLOR | render::CBF_DEPTH | render::CBF_STENCIL : render::CBF_NONE;
    ctxt->clearBuffers(clear, context.clear_color);
    ctxt->setFrameBuffer(render_target.get());
    ctxt->setViewportSize(recti(0, 0, size.X, size.Y));

	RenderTarget::activate(context);
}

render::FrameBuffer *TextureBufferOutput::getRenderTarget(PipelineContext &context)
{
	activate(context); // Needed to make sure that render_target is set up.
    return render_target.get();
}

u8 DynamicSource::getTextureCount()
{
	assert(isConfigured());
	return upstream->getTextureCount();
}

render::Texture *DynamicSource::getTexture(u8 index)
{
	assert(isConfigured());
	return upstream->getTexture(index);
}

void ScreenTarget::activate(PipelineContext &context)
{
    auto ctxt = context.client->getRenderSystem()->getRenderer()->getContext();
    u16 clear = m_clear ? render::CBF_COLOR | render::CBF_DEPTH | render::CBF_STENCIL : render::CBF_NONE;
    ctxt->clearBuffers(clear, context.clear_color);
    ctxt->setViewportSize(recti(0, 0, size.X, size.Y));

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
    size = context.client->getRenderSystem()->getWindowSize();
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

RenderSource *RenderPipeline::getInput()
{
	return &m_input;
}

RenderTarget *RenderPipeline::getOutput()
{
	return &m_output;
}

void RenderPipeline::run(PipelineContext &context)
{
    v2u original_size = context.target_size;
    context.target_size = v2u(original_size.X * scale.X, original_size.Y * scale.Y);

	for (auto &object : m_objects)
		object->reset(context);

	for (auto &step: m_pipeline)
		step->run(context);

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
