// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#pragma once
#include "pipeline.h"
#include <Render/Shader.h>

/**
 * Offset camera for a specific eye in stereo rendering mode
 */
class OffsetCameraStep : public TrivialRenderStep
{
public:
	OffsetCameraStep(f32 eye_offset);
	OffsetCameraStep(bool right_eye);

	void run(PipelineContext &context) override;
	void reset(PipelineContext &context) override;
private:
    matrix4 base_transform;
    matrix4 move;
};

/**
 * Implements a pipeline step that renders the 3D scene
 */
class Draw3D : public RenderStep
{
public:
    virtual void setRenderSource(RenderSource *) override {}
    virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }

    virtual void reset(PipelineContext &context) override {}
    virtual void run(PipelineContext &context) override;

private:
    RenderTarget *m_target {nullptr};
};

/**
 * Implements a pipeline step that renders the game HUD
 */
class DrawHUD : public RenderStep
{
public:
    virtual void setRenderSource(RenderSource *) override {}
    virtual void setRenderTarget(RenderTarget *) override {}

    virtual void reset(PipelineContext &context) override {}
    virtual void run(PipelineContext &context) override;
};

/*class RenderShadowMapStep : public TrivialRenderStep
{
public:
    virtual void run(PipelineContext &context) override;
};*/


class RenderSystem;
class MeshBuffer;

class ScreenQuad
{
    RenderSystem *rndsys;
    std::unique_ptr<MeshBuffer> quad;

    RenderSource *textures;
    std::vector<u8> texture_map;

    render::Shader *shader;
    bool use_default = true;

    bool update_tex_params = true;
    std::map<u32, std::pair<render::TextureMinFilter, render::TextureMagFilter>> update_filters;

    v2u prev_size;

    f32 user_exposure_compensation;
    bool bloom_enabled;
    bool volumetric_light_enabled;
public:
    bool set_postprocess_uniforms = false;

    ScreenQuad(RenderSystem *_rndsys, RenderSource *_textures);

    MeshBuffer *getBuffer() const
    {
        return quad.get();
    }

    void updateQuad(std::optional<v2u> offset=std::nullopt, std::optional<v2u> size=std::nullopt);

    void setTextureMap(const std::vector<u8> &map)
    {
        texture_map = map;
    }
    void setShader(bool set_default=true, std::optional<render::Shader *> new_shader=std::nullopt);

    void configureTexturesSettings();
    void setBilinearFilter(u8 index, bool value);

    void setPostprocessUniforms(Client *client);

    void render(Client *client);
};

RenderStep *create3DStage(Client *client, v2f scale);

img::PixelFormat selectColorFormat();
img::PixelFormat selectDepthFormat(u32 bits, bool stencil);
