#pragma once

#include "FilesystemVersions.h"
#include <Utils/Rect.h>
#include <Render/DrawContext.h>

namespace render
{
class Texture2D;
}

namespace img
{
class ImageModifier;
}

extern img::ImageModifier *g_imgmodifier;

class MeshBuffer;
class ResourceCache;

using namespace render;

enum class FogType : u8
{
    Exp,
    Linear,
    Exp2
};

enum class TMatrix : u8
{
    World,
    View,
    Projection,
    Texture0
};

class Renderer
{
    std::unique_ptr<DrawContext> context;
    ResourceCache *resCache;

    std::unordered_map<std::string, Shader *> defaultShaders;

    Texture2D *curTexture = nullptr;
    Shader *curShader = nullptr;

    std::unique_ptr<render::UniformBuffer> matrix_buffer;
    std::unique_ptr<render::UniformBuffer> fog_buffer;

    // Necessary as the matrix_buffer saves combined matrices (e.g. projection * view)
    matrix4 projM, viewM, worldM;

    bool use3DMode = true; // if false, then this is the 2d mode
    bool transparentPass = false;
    bool useGLBlend = true;

    static const u32 matrix_buffer_size = sizeof(f32) * 16 * 4;
    static const u32 fog_buffer_size = sizeof(s32)+sizeof(s32)+(u32)img::pixelFormatInfo[img::PF_RGBA8].size+sizeof(f32)*3;
public:
    Renderer(ResourceCache *res_cache, const recti &viewportSize, u32 maxTexUnits)
        : context(std::make_unique<DrawContext>(viewportSize, maxTexUnits)), resCache(res_cache)
    {
        createDefaultShaders();
        createUBOs();
    }

    DrawContext *getContext() const
    {
        return context.get();
    }

    void setRenderState(bool mode3d);
    void setDefaultShader(bool transparent=false, bool glBlend=true);

    void setTexture(Texture2D *tex);
    void setDefaultUniforms(f32 thickness, u8 alphaDiscard, f32 alphaRef, img::BlendMode blendMode);

    void setClipRect(const recti &clipRect);
    void disableScissorTest();

    // Fog UBO settings
    bool fogEnabled() const;
    void getFogParams(FogType &type, img::color8 &color, f32 &start, f32 &end, f32 &density) const;
    void enableFog(bool enable);
    void setFogParams(FogType type, img::color8 color, f32 start, f32 end, f32 density);

    // Transformation matrices UBO settings
    matrix4 getTransformMatrix(TMatrix type) const;
    void setTransformMatrix(TMatrix type, const matrix4 &mat);

    void draw(MeshBuffer *buffer, PrimitiveType type=PT_TRIANGLES,
        u32 offset=0, u32 count=1);
private:
    void createDefaultShaders();
    void createUBOs();
};
