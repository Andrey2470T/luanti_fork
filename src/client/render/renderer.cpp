#include "renderer.h"
#include <Render/Texture2D.h>
#include <Image/ImageModifier.h>
#include "client/mesh/meshbuffer.h"
#include "client/media/resource.h"
#include "datatexture.h"

// These binding points are reserved and can't be used for other UBOs
#define MATRIX_UBO_BINDING_POINT 0
#define FOG_UBO_BINDING_POINT 1

img::ImageModifier *g_imgmodifier = new img::ImageModifier();

const img::color8 Renderer::menu_sky_color = img::color8(img::PF_RGBA8, 140, 186, 250, 255);

void Renderer::setRenderState(bool mode3d)
{
    use3DMode = mode3d;

    context->enableDepthTest(true);
    context->setDepthFunc(CF_LESS);

    context->enableCullFace(true);
    context->setCullMode(CM_BACK);
}

void Renderer::setBlending(bool transparent, bool glBlend)
{
    transparentPass = transparent;
    useGLBlend = glBlend;

    if (transparentPass) {
        if (useGLBlend)
            context->setBlendMode(GLBlendMode::ALPHA);
        else
            context->enableBlend(false);
    }
    else
        context->setBlendMode(GLBlendMode::NORMAL);
}

void Renderer::setDefaultShader(bool transparent, bool glBlend)
{
    setBlending(transparent, glBlend);

    std::string name;
    if (!use3DMode)
        name = "standard2D";
    else {
        if (!transparent)
            name = "solid3D";
        else {
            if (useGLBlend)
                name = "transparent3D";
            else
                name = "textureBlend3D";
        }
    }

    setShader(defaultShaders[name]);
}

void Renderer::setTexture(Texture2D *tex)
{
    context->setActiveUnit(0, tex);
    curTexture = tex;
}

void Renderer::setShader(Shader *shd)
{
    context->setShader(shd);
    curShader = shd;
}

void Renderer::setDefaultUniforms(f32 thickness, u8 alphaDiscard, f32 alphaRef, img::BlendMode blendMode)
{
    if (!curShader)
        return;

    curShader->setUniformFloat("mThickness", thickness);

    bool useTexture = curTexture != nullptr;
    curShader->setUniformInt("mTextureUsage0", (s32)useTexture);

    if (!use3DMode) {
        auto cur_viewport = context->getViewportSize();
        matrix4 proj;
        f32 xInv2 = 2.0f / cur_viewport.getWidth();
        f32 yInv2 = 2.0f / cur_viewport.getHeight();
        proj.setScale({ xInv2, -yInv2, 0.0f });
        proj.setTranslation({ -1.0f, 1.0f, 0.0f });
        curShader->setUniform4x4Matrix("mProjection", proj);
    }
    else {
        if (transparentPass) {
            if (useGLBlend) {
                curShader->setUniformInt("mAlphaDiscard", alphaDiscard);
                curShader->setUniformFloat("mAlphaRef", alphaRef);
            }
            else {
                curShader->setUniformInt("mBlendMode", (s32)blendMode);
            }
        }

        setUniformBlocks();
    }
}

void Renderer::setUniformBlocks()
{
    curShader->setUniformBlock("mMatrices", matrix_buffer.get());
    curShader->setUniformBlock("mFogParams", fog_buffer.get());
}

void Renderer::setClipRect(const recti &clipRect)
{
    recti viewport = context->getViewportSize();

    context->enableScissorTest(true);
    context->setScissorBox(recti(
        clipRect.ULC.X, viewport.getHeight()-clipRect.LRC.Y,
        clipRect.getWidth(), clipRect.getHeight()));
}

void Renderer::disableScissorTest()
{
    context->enableScissorTest(false);
}

bool Renderer::fogEnabled() const
{
    auto byteArr = fog_buffer->getUniformsData();
    return byteArr.getInt(0);
}

void Renderer::getFogParams(FogType &type, img::color8 &color, f32 &start, f32 &end, f32 &density) const
{
    auto byteArr = fog_buffer->getUniformsData();

    type = (FogType)byteArr.getInt(1);
    color = img::getColor8(&byteArr, 2);
    start = byteArr.getFloat(6);
    end = byteArr.getFloat(7);
    density = byteArr.getFloat(8);
}

void Renderer::enableFog(bool enable)
{
    if (fogEnabled() == enable)
        return;

    auto byteArr = fog_buffer->getUniformsData();

    byteArr.setInt((s32)enable, 0);

    fog_buffer->uploadSubData(0, sizeof(s32));
}

void Renderer::setFogParams(FogType type, img::color8 color, f32 start, f32 end, f32 density)
{
    FogType curType;
    img::color8 curColor;
    f32 curStart, curEnd, curDensity;

    getFogParams(curType, curColor, curStart, curEnd, curDensity);

    if (curType == type && curColor == color && curStart == start && curEnd == end && curDensity == density)
        return;

    auto byteArr = fog_buffer->getUniformsData();

    byteArr.setInt((s32)type, 1);
    img::setColor8(&byteArr, color, 2);
    byteArr.setFloat(start, 6);
    byteArr.setFloat(end, 7);
    byteArr.setFloat(density, 8);

    fog_buffer->uploadSubData(sizeof(s32), fog_buffer_size-sizeof(s32));
}

matrix4 Renderer::getTransformMatrix(TMatrix type) const
{
    auto byteArr = matrix_buffer->getUniformsData();

    switch (type) {
    case TMatrix::World:
        return worldM;
    case TMatrix::View:
        return viewM;
    case TMatrix::Projection:
        return projM;
    case TMatrix::Texture0:
        return byteArr.getM4x4(48);
    };
}

void Renderer::setTransformMatrix(TMatrix type, const matrix4 &mat)
{
    auto byteArr = matrix_buffer->getUniformsData();

    auto curMatrix = getTransformMatrix(type);

    if (curMatrix == mat)
        return;

    switch (type) {
    case TMatrix::World: {
        worldM = mat;

        byteArr.setM4x4(projM * viewM * worldM, 0);
        byteArr.setM4x4(viewM * worldM, 16);
        byteArr.setM4x4(worldM, 32);
        break;
    }
    case TMatrix::View: {
        viewM = mat;

        byteArr.setM4x4(projM * viewM * worldM, 0);
        byteArr.setM4x4(viewM * worldM, 16);
        break;
    }
    case TMatrix::Projection: {
        projM = mat;

        byteArr.setM4x4(projM * viewM * worldM, 0);
        break;
    }
    case TMatrix::Texture0:
        byteArr.setM4x4(mat, 48);
        break;
    };
}

void Renderer::setDataTexParams(DataTexture *tex)
{
    curShader->setUniformInt("mSampleCount", tex->sampleCount);
    curShader->setUniformInt("mSampleDim", tex->sampleDim);
    curShader->setUniformInt("mDataTexDim", tex->texDim);
}

void Renderer::draw(MeshBuffer *buffer, PrimitiveType type,
    u32 offset, std::optional<u32> count)
{
    auto vao = buffer->getVAO();

    if (!count.has_value()) {
        switch (type) {
        case render::PT_POINTS:
        case render::PT_POINT_SPRITES:
            count = buffer->getVertexCount();
            break;
        case render::PT_LINE_STRIP:
        case render::PT_LINE_LOOP:
        case render::PT_LINES:
        case render::PT_TRIANGLE_STRIP:
        case render::PT_TRIANGLE_FAN:
        case render::PT_TRIANGLES:
            count = buffer->getIndexCount();
            break;
        default:
            break;
        }
    }
    vao->draw(type, count.value(), offset);
}

void Renderer::createDefaultShaders()
{
    auto standard2d = resCache->getOrLoad<render::Shader>(ResourceType::SHADER, "standard2D");
    auto solid3d = resCache->getOrLoad<render::Shader>(ResourceType::SHADER, "solid3D");
    auto transparent3d = resCache->getOrLoad<render::Shader>(ResourceType::SHADER, "transparent3D");
    auto textureBlend3d = resCache->getOrLoad<render::Shader>(ResourceType::SHADER, "textureBlend3D");

    defaultShaders["standard2D"] = standard2d;
    defaultShaders["solid3D"] = solid3d;
    defaultShaders["transparent3D"] = transparent3d;
    defaultShaders["textureBlend3D"] = textureBlend3d;
}

void Renderer::createUBOs()
{
    ByteArray matrix_ba(16*4, matrix_buffer_size);
    matrix_buffer = std::make_unique<UniformBuffer>(MATRIX_UBO_BINDING_POINT, matrix_ba);

    ByteArray fog_ba(9, fog_buffer_size);
    fog_buffer = std::make_unique<UniformBuffer>(FOG_UBO_BINDING_POINT, fog_ba);
}
