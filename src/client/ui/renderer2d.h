#pragma once

#include "BasicIncludes.h"
#include <memory>
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
}

class MeshBuffer;
class ResourceCache;

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
    
    void draw2DLine(MeshBuffer *line);
    void draw2DRectangle(MeshBuffer *rect);
    void draw2DImage(MeshBuffer *rect, Texture2D *tex);

    void setRenderState(bool alpha, bool texShader);
    void setTexture(Texture2D *tex);
    void setUniforms(f32 thickness, bool texShader);
    void setClipRect(const recti &clipRect);
    
    void disableScissorTest();
private:
    void init2DShaders();
};
