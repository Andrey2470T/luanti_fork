#pragma once

#include <Utils/Rect.h>
#include <Render/DrawContext.h>
#include <Core/MainWindow.h>

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
class DataTexture;
struct FpsControl;

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

struct FpsControl {
    FpsControl() : last_time(0), busy_time(0), sleep_time(0) {}

    void reset();

    void limit(core::MainWindow *wnd, f32 *dtime);

    u32 getBusyMs() const { return busy_time / 1000; }

    // all values in microseconds (us)
    u64 last_time, busy_time, sleep_time;
};

struct Jitter {
    f32 max, min, avg, counter, max_sample, min_sample, max_fraction;
};

struct DrawStats
{
    u32 drawcalls;
    u32 drawn_primitives;
    f32 drawtime; // in us

    FpsControl fps;
    Jitter dtime_jitter, busy_time_jitter;
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

    DrawStats stats;

    // Necessary as the matrix_buffer saves combined matrices (e.g. projection * view)
    matrix4 projM, viewM, worldM;

    bool use3DMode = true; // if false, then this is the 2d mode
    bool transparentPass = false;
    bool useGLBlend = true;

    const u32 matrix_buffer_size = sizeof(f32) * 16 * 4;
    const u32 fog_buffer_size = sizeof(s32)+sizeof(s32)+(u32)img::pixelFormatInfo[img::PF_RGBA8].size+sizeof(f32)*3;
public:
    const static img::color8 menu_sky_color;

    Renderer(ResourceCache *res_cache, const recti &viewportSize, u32 maxTexUnits);

    DrawContext *getContext() const
    {
        return context.get();
    }

    DrawStats &getDrawStats()
    {
        return stats;
    }

    void setRenderState(bool mode3d);
    void setBlending(bool transparent=false, bool glBlend=true);
    void setDefaultShader(bool transparent=false, bool glBlend=true);

    void setTexture(Texture2D *tex);
    void setShader(Shader *shd);
    Shader *getShader() const
    {
        return curShader;
    }
    void setDefaultUniforms(f32 thickness=1.0f, u8 alphaDiscard=0, f32 alphaRef=0.5f, img::BlendMode blendMode=img::BM_COUNT);

    void setUniformBlocks();
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

    // Data texture parameters
    void setDataTexParams(DataTexture *tex);

    void draw(MeshBuffer *buffer, PrimitiveType type=PT_TRIANGLES,
        u32 offset=0, std::optional<u32> count=std::nullopt);

    void updateStats(f32 dtime);
private:
    void createDefaultShaders();
    void createUBOs();

    u32 calcPrimitiveCount(PrimitiveType type, u32 count);
};
