#pragma once

#include "FilesystemVersions.h"
#include "Utils/Rect.h"

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
struct ImageFiltered;

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
    void drawLine(MeshBuffer *line);
    void drawRectangle(MeshBuffer *rects, u32 n=0);
    void drawImage(MeshBuffer *rects, Texture2D *tex, u32 n=0);
    void drawImageFiltered(MeshBuffer *rects, ImageFiltered *img, u32 n=0);
    void draw9SlicedImage(Image2D9Slice *img);

    void setRenderState(bool alpha, bool texShader);
    void setTexture(Texture2D *tex);
    void setUniforms(f32 thickness, bool texShader);
    void setClipRect(const recti &clipRect);
    
    void disableScissorTest();
private:
    void init2DShaders();
};
