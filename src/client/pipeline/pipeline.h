// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 x2048, Dmitry Kostenko <codeforsmile@gmail.com>
#pragma once

#include <BasicIncludes.h>

class RenderSource;
class RenderTarget;
class RenderStep;
class Client;
class Hud;
class ShadowRenderer;

namespace render
{
class Texture;
class FrameBuffer;
}

namespace main
{
class MainWindow;
}

struct PipelineContext
{
    PipelineContext(Client *_client, Hud *_hud, ShadowRenderer *_shadow_renderer, img::color8 _color, v2u _target_size)
        : client(_client), hud(_hud), shadow_renderer(_shadow_renderer), clear_color(_color), target_size(_target_size)
	{
	}

	Client *client;
	Hud *hud;
	ShadowRenderer *shadow_renderer;
    img::color8 clear_color;
    v2u target_size;

	bool show_hud {true};
	bool draw_wield_tool {true};
	bool draw_crosshair {true};
};

/**
 * Base object that can be owned by Pipeline
 *
 */
class PipelineObject
{
public:
    virtual ~PipelineObject() = default;
	virtual void reset(PipelineContext &context) {}
};

/**
 * Represents a source of rendering information such as textures
 */
class RenderSource : virtual public PipelineObject
{
public:
	/**
	 * Return the number of textures in the source.
	 */
	virtual u8 getTextureCount() = 0;

	/**
	 * Get a texture by index.
	 * Returns nullptr is the texture does not exist.
	 */
    virtual render::Texture *getTexture(u8 index) = 0;
};

/**
 *	Represents a render target (screen or framebuffer)
 */
class RenderTarget : virtual public PipelineObject
{
public:
	/**
	 * Activate the render target and configure OpenGL state for the output.
	 * This is usually done by @see RenderStep implementations.
	 */
	virtual void activate(PipelineContext &context)
	{
		m_clear = false;
	}

	/**
	 * Resets the state of the object for the next pipeline iteration
	 */
	virtual void reset(PipelineContext &context) override
	{
		m_clear = true;
	}

protected:
	bool m_clear {true};
};

/**
 * Texture buffer represents a framebuffer with a multiple attached textures.
 *
 * @note Use of TextureBuffer requires use of gl_FragData[] in the shader
 */
class TextureBuffer : public RenderSource
{
public:
	virtual ~TextureBuffer() override;

	/**
	 * Configure fixed-size texture for the specific index
	 *
	 * @param index index of the texture
	 * @param size width and height of the texture in pixels
	 * @param height height of the texture in pixels
	 * @param name unique name of the texture
	 * @param format color format
	 */
    void setTexture(
        u8 index, v2u size, const std::string& name, img::PixelFormat format,
        bool clear = false, u8 msaa = 0, bool cubemap = false);

	/**
	 * Configure relative-size texture for the specific index
	 *
	 * @param index index of the texture
	 * @param scale_factor relation of the texture dimensions to the screen dimensions
	 * @param name unique name of the texture
	 * @param format color format
	 */
    void setTexture(
        u8 index, v2f scale_factor, const std::string& name, img::PixelFormat format,
        bool clear = false, u8 msaa = 0, bool cubemap = false);

	virtual u8 getTextureCount() override { return m_textures.size(); }
    virtual render::Texture *getTexture(u8 index) override;
	virtual void reset(PipelineContext &context) override;
	void swapTextures(u8 texture_a, u8 texture_b);
private:
	static const u8 NO_DEPTH_TEXTURE = 255;

	struct TextureDefinition
	{
		bool valid { false };
		bool fixed_size { false };
		bool dirty { false };
		bool clear { false };
		v2f scale_factor;
        v2u size;
		std::string name;
        img::PixelFormat format;
		u8 msaa;
        bool cubemap { false };
	};

	/**
	 * Make sure the texture in the given slot matches the texture definition given the current context.
	 * @param textureSlot address of the texture pointer to verify and populate.
	 * @param definition logical definition of the texture
	 * @param context current context of the rendering pipeline
	 * @return true if a new texture was created and put into the slot
	 * @return false if the slot was not modified
	 */
    bool ensureTexture(render::Texture **textureSlot, const TextureDefinition& definition, PipelineContext &context);

	std::vector<TextureDefinition> m_definitions;
    std::vector<render::Texture *> m_textures;
};

/**
 * Targets output to designated texture in texture buffer
 * Maps texture number with cubemap face (or ignored if this is not a cubemap)
 */
class TextureBufferOutput : public RenderTarget
{
public:
	TextureBufferOutput(TextureBuffer *buffer, u8 texture_index);
    TextureBufferOutput(TextureBuffer *buffer, const std::unordered_map<u8, u8> &texture_map);
    TextureBufferOutput(TextureBuffer *buffer, const std::unordered_map<u8, u8> &texture_map, std::pair<u8, u8> depth_stencil);
    virtual ~TextureBufferOutput() override {};
	void activate(PipelineContext &context) override;

    render::FrameBuffer *getRenderTarget(PipelineContext &context);

private:
	static const u8 NO_DEPTH_TEXTURE = 255;

	TextureBuffer *buffer;
    std::unordered_map<u8, u8> texture_map;
    std::pair<u8, u8> depth_stencil { NO_DEPTH_TEXTURE, 0 };
    std::unique_ptr<render::FrameBuffer> render_target;
};

class DynamicSource : public RenderSource
{
public:
	bool isConfigured() { return upstream != nullptr; }
	void setRenderSource(RenderSource *value) { upstream = value; }

	/**
	 * Return the number of textures in the source.
	 */
	virtual u8 getTextureCount() override;

	/**
	 * Get a texture by index.
	 * Returns nullptr is the texture does not exist.
	 */
    virtual render::Texture *getTexture(u8 index) override;
private:
	RenderSource *upstream { nullptr };
};

/**
 * Implements direct output to screen framebuffer.
 */
class ScreenTarget : public RenderTarget
{
public:
	virtual void activate(PipelineContext &context) override;
	virtual void reset(PipelineContext &context) override;
private:
    v2u size;
};

class DynamicTarget : public RenderTarget
{
public:
	bool isConfigured() { return upstream != nullptr; }
	void setRenderTarget(RenderTarget *value) { upstream = value; }
	virtual void activate(PipelineContext &context) override;
private:
	RenderTarget *upstream { nullptr };
};

/**
 * Base class for rendering steps in the pipeline
 */
class RenderStep : virtual public PipelineObject
{
public:
	/**
	 * Assigns render source to this step.
	 *
	 * @param source source of rendering information
	 */
	virtual void setRenderSource(RenderSource *source) = 0;

	/**
	 * Assigned render target to this step.
	 *
	 * @param target render target to send output to.
	 */
	virtual void setRenderTarget(RenderTarget *target) = 0;

	/**
	 * Runs the step. This method is invoked by the pipeline.
	 */
	virtual void run(PipelineContext &context) = 0;
};

/**
 * Provides default empty implementation of supporting methods in a rendering step.
 */
class TrivialRenderStep : public RenderStep
{
public:
	virtual void setRenderSource(RenderSource *source) override {}
	virtual void setRenderTarget(RenderTarget *target) override {}
	virtual void reset(PipelineContext &) override {}
};

/**
 * Dynamically changes render target of another step.
 *
 * This allows re-running parts of the pipeline with different outputs
 */
class SetRenderTargetStep : public TrivialRenderStep
{
public:
	SetRenderTargetStep(RenderStep *step, RenderTarget *target);
	virtual void run(PipelineContext &context) override;
private:
	RenderStep *step;
	RenderTarget *target;
};

/**
 * Swaps two textures in the texture buffer.
 *
 */
class SwapTexturesStep : public TrivialRenderStep
{
public:
	SwapTexturesStep(TextureBuffer *buffer, u8 texture_a, u8 texture_b);
	virtual void run(PipelineContext &context) override;
private:
	TextureBuffer *buffer;
	u8 texture_a;
	u8 texture_b;
};

/**
 * Render Pipeline provides a flexible way to execute rendering steps in the engine.
 *
 * RenderPipeline also implements @see RenderStep, allowing for nesting of the pipelines.
 */
class Pipeline : public RenderStep
{
public:
	/**
	 * Add a step to the end of the pipeline
	 *
	 * @param step reference to a @see RenderStep implementation.
	 */
	RenderStep *addStep(RenderStep *step)
	{
		m_pipeline.push_back(step);
		return step;
	}

	/**
	 * Create and add a step managed by the pipeline and return a pointer
	 * to the step for further configuration.
	 *
	 * @tparam T Type of the step to be added.
	 * @tparam Args Types of the constructor parameters
	 * @param args Constructor parameters
	 * @return RenderStep* Pointer to the created step for further configuration.
	 */
	template<typename T, typename... Args>
	T *addStep(Args&&... args) {
		T* result = new T(std::forward<Args>(args)...);
		m_objects.emplace_back(result);
		addStep(result);
		return result;
	}

	RenderSource *getInput();
	RenderTarget *getOutput();

	v2f getScale() { return scale; }
	void setScale(v2f value) { scale = value; }

	virtual void reset(PipelineContext &context) override {}
	virtual void run(PipelineContext &context) override;

	virtual void setRenderSource(RenderSource *source) override;
	virtual void setRenderTarget(RenderTarget *target) override;
private:
	std::vector<RenderStep *> m_pipeline;
    std::vector< std::unique_ptr<PipelineObject> > m_objects;
	DynamicSource m_input;
	DynamicTarget m_output;
	v2f scale { 1.0f, 1.0f };
};
