#include "renderer.h"
#include <Render/Texture2D.h>
#include <Image/ImageModifier.h>
#include "Image/Converting.h"
#include "client/mesh/meshbuffer.h"
#include "client/media/resource.h"
#include "client/render/rendersystem.h"
#include "datatexture.h"
#include "porting.h"
#include "settings.h"

// These binding points are reserved and can't be used for other UBOs
#define MATRIX_UBO_BINDING_POINT 0
#define FOG_UBO_BINDING_POINT 1

img::ImageModifier *g_imgmodifier = new img::ImageModifier();

const img::color8 Renderer::menu_sky_color = img::color8(img::PF_RGBA8, 140, 186, 250, 255);

void FpsControl::reset()
{
    last_time = porting::getTimeUs();
}

void FpsControl::limit(core::MainWindow *wnd, f32 *dtime)
{
    const float fps_limit = wnd->isFocused()
            ? g_settings->getFloat("fps_max")
            : g_settings->getFloat("fps_max_unfocused");
    const u64 frametime_min = 1000000.0f / std::max(fps_limit, 1.0f);

    u64 time = porting::getTimeUs();

    if (time > last_time) // Make sure time hasn't overflowed
        busy_time = time - last_time;
    else
        busy_time = 0;

    if (busy_time < frametime_min) {
        sleep_time = frametime_min - busy_time;
        porting::preciseSleepUs(sleep_time);
    } else {
        sleep_time = 0;
    }

    // Read the timer again to accurately determine how long we actually slept,
    // rather than calculating it by adding sleep_time to time.
    time = porting::getTimeUs();

    if (time > last_time) // Make sure last_time hasn't overflowed
        *dtime = (time - last_time) / 1000000.0f;
    else
        *dtime = 0;

    last_time = time;
}

Renderer::Renderer(ResourceCache *res_cache, const recti &viewportSize, u32 maxTexUnits)
    : context(std::make_unique<DrawContext>(viewportSize, maxTexUnits)), resCache(res_cache)
{
    createDefaultShaders();
    createUBOs();
}

void Renderer::setRenderState(bool mode3d)
{
    use3DMode = mode3d;

    if (use3DMode) {
        context->enableDepthTest(true);
        context->setDepthFunc(CF_LESS);

        context->enableCullFace(true);
        context->setCullMode(CM_BACK);
    }
    else {
        context->enableDepthTest(false);
        context->enableCullFace(false);
    }
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
    }
}

void Renderer::setUniformBlocks(Shader *shader)
{
    shader->setUniformBlock("mMatrices", matrix_buffer.get());
    shader->setUniformBlock("mFogParams", fog_buffer.get());
}

void Renderer::setClipRect(const recti &clipRect)
{
    if (clipRect != recti()) {
        context->enableScissorTest(true);
        context->setScissorBox(clipRect);
    }
    else
        context->enableScissorTest(false);
}

void Renderer::disableScissorTest()
{
    context->enableScissorTest(false);
}

bool Renderer::fogEnabled() const
{
    ByteArray &byteArr = fog_buffer->getUniformsData();
    return byteArr.getUInt32(0, 0);
}

void Renderer::getFogParams(FogType &type, img::colorf &color, f32 &start, f32 &end, f32 &density) const
{
    ByteArray &byteArr = fog_buffer->getUniformsData();

    type = (FogType)byteArr.getUInt32(0, 1);
    color = byteArr.getColorf(0, 2);
    start = byteArr.getFloat(0, 3);
    end = byteArr.getFloat(0, 4);
    density = byteArr.getFloat(0, 5);
}

void Renderer::enableFog(bool enable)
{
    if (fogEnabled() == enable)
        return;

    ByteArray &byteArr = fog_buffer->getUniformsData();

    byteArr.setUInt32((u32)enable, 0, 0);

    fog_buffer->uploadSubData(0, sizeof(u32));
}

void Renderer::setFogParams(FogType type, img::colorf color, f32 start, f32 end, f32 density)
{
    FogType curType;
    img::colorf curColor;
    f32 curStart, curEnd, curDensity;

    getFogParams(curType, curColor, curStart, curEnd, curDensity);

    if (curType == type && curColor == color && curStart == start && curEnd == end && curDensity == density)
        return;

    ByteArray &byteArr = fog_buffer->getUniformsData();

    byteArr.setUInt32((u32)type, 1, 0);
    byteArr.setColorf(color, 2, 0);
    byteArr.setFloat(start, 3, 0);
    byteArr.setFloat(end, 4, 0);
    byteArr.setFloat(density, 5, 0);

    fog_buffer->uploadSubData(sizeof(u32), byteArr.bytesCount()-sizeof(u32));
}

matrix4 Renderer::getTransformMatrix(TMatrix type) const
{
    switch (type) {
    case TMatrix::World:
        return worldM;
    case TMatrix::View:
        return viewM;
    case TMatrix::Projection:
        return projM;
    case TMatrix::Texture0:
        return texM;
    };

    return matrix4();
}

void Renderer::setTransformMatrix(TMatrix type, const matrix4 &mat)
{
    ByteArray &byteArr = matrix_buffer->getUniformsData();

    auto curMatrix = getTransformMatrix(type);

    if (curMatrix == mat)
        return;

    switch (type) {
    case TMatrix::World: {
        worldM = mat;

        byteArr.setM4x4(projM * viewM * worldM, 0, 0);
        byteArr.setM4x4(viewM * worldM, 1, 0);
        byteArr.setM4x4(worldM, 2, 0);
        matrix_buffer->uploadSubData(0, sizeof(f32) * 16 * 3);
        break;
    }
    case TMatrix::View: {
        viewM = mat;

        byteArr.setM4x4(projM * viewM * worldM, 0, 0);
        byteArr.setM4x4(viewM * worldM, 1, 0);
        matrix_buffer->uploadSubData(0, sizeof(f32) * 16 * 2);
        break;
    }
    case TMatrix::Projection: {
        projM = mat;

        byteArr.setM4x4(projM * viewM * worldM, 0, 0);
        matrix_buffer->uploadSubData(0, sizeof(f32) * 16);
        break;
    }
    case TMatrix::Texture0:
        texM = mat;

        byteArr.setM4x4(mat, 3, 0);
        matrix_buffer->uploadSubData(sizeof(f32) * 16 * 3, sizeof(f32) * 16);
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

    stats.drawcalls++;
    stats.drawn_primitives += calcPrimitiveCount(type, count.value());
}

void Renderer::updateStats(f32 dtime)
{
    f32 jitter;
    Jitter *jp;

    /* Time average and jitter calculation
     */
    jp = &stats.dtime_jitter;
    jp->avg = jp->avg * 0.96 + dtime * 0.04;

    jitter = dtime - jp->avg;

    if (jitter > jp->max)
        jp->max = jitter;

    jp->counter += dtime;

    if (jp->counter > 0.0) {
        jp->counter -= 3.0;
        jp->max_sample = jp->max;
        jp->max_fraction = jp->max_sample / (jp->avg + 0.001);
        jp->max = 0.0;
    }

    /* Busytime average and jitter calculation
     */
    jp = &stats.busy_time_jitter;
    jp->avg = jp->avg + stats.fps.getBusyMs() * 0.02;

    jitter = stats.fps.getBusyMs() - jp->avg;

    if (jitter > jp->max)
        jp->max = jitter;
    if (jitter < jp->min)
        jp->min = jitter;

    jp->counter += dtime;

    if (jp->counter > 0.0) {
        jp->counter -= 3.0;
        jp->max_sample = jp->max;
        jp->min_sample = jp->min;
        jp->max = 0.0;
        jp->min = 0.0;
    }
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
    ByteArray matrix_ba({"MatrixUBO",
        {
            {"WVPM", ByteArrayElementType::MAT4},
            {"WVM", ByteArrayElementType::MAT4},
            {"WM", ByteArrayElementType::MAT4},
            {"TM", ByteArrayElementType::MAT4}
        }}, 1);

    matrix_ba.setM4x4(projM * viewM * worldM, 0, 0);
    matrix_ba.setM4x4(viewM * worldM, 1, 0);
    matrix_ba.setM4x4(worldM, 2, 0);
    matrix_ba.setM4x4(texM, 3, 0);

    matrix_buffer = std::make_unique<UniformBuffer>(MATRIX_UBO_BINDING_POINT, matrix_ba);

    ByteArray fog_ba({"FogUBO",
        {
            {"Enable", ByteArrayElementType::U32},
            {"Type", ByteArrayElementType::U32},
            {"Color", ByteArrayElementType::COLORF},
            {"Start", ByteArrayElementType::FLOAT},
            {"End", ByteArrayElementType::FLOAT},
            {"Density", ByteArrayElementType::FLOAT}
        }}, 1);

    fog_ba.setUInt32(0, 0, 0);
    fog_ba.setUInt32(0, 1, 0);
    fog_ba.setColorf(img::colorf(), 2, 0);
    fog_ba.setFloat(0.0f, 3, 0);
    fog_ba.setFloat(0.0f, 4, 0);
    fog_ba.setFloat(0.0f, 5, 0);

    fog_buffer = std::make_unique<UniformBuffer>(FOG_UBO_BINDING_POINT, fog_ba);
}

u32 Renderer::calcPrimitiveCount(PrimitiveType type, u32 count)
{
    switch (type) {
    case render::PT_POINTS:
        return count;
    case render::PT_POINT_SPRITES:
        return count;
    case render::PT_LINE_STRIP:
        return count - 1;
    case render::PT_LINE_LOOP:
        return count;
    case render::PT_LINES:
        return count / 2;
    case render::PT_TRIANGLE_STRIP:
        return count - 2;
    case render::PT_TRIANGLE_FAN:
        return count - 2;
    case render::PT_TRIANGLES:
        return count / 3;
    default:
        return 0;
    }
}
