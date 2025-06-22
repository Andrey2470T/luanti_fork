#pragma once

#include "FilesystemVersions.h"
#include <Utils/Rect.h>
#include <Image/ImageModifier.h>

extern img::ImageModifier *g_imgmodifier;

namespace render
{
    class Texture2D;
	class Shader;
	class DrawContext;
}

namespace img
{
    class Image;
    class ImageModifier;
}

class MeshBuffer;
class ResourceCache;
struct Image2D9Slice;
class UIShape;

using namespace render;

class Renderer2D
{
	DrawContext *Context;
    ResourceCache *resCache;
	
    Shader* NoTexShader2D;
    Shader* Shader2D;

    Texture2D *texture0;
	
public:
    Renderer2D(DrawContext *ctxt, ResourceCache *res_cache)
        : Context(ctxt), resCache(res_cache)
    {
        init2DShaders();
    }
    
    DrawContext *getContext() const
    {
        return Context;
    }
    // Draws n count of adjacent primitives of the same type
    // 'offset_n' is a primitive number with which to start drawing
    // 'count_n' is a primitive count which needs to draw
    void drawPrimitives(UIShape *shape, MeshBuffer *buf, u32 offset_n=0, u32 count_n=1);

    void setBasicRenderState(bool alpha);
    void setRenderState(bool alpha, bool texShader);
    void setTexture(Texture2D *tex);
    void setUniforms(f32 thickness, bool texShader);
    void setClipRect(const recti &clipRect);
    
    void disableScissorTest();
private:
    void init2DShaders();
};
