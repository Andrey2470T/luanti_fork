#pragma once

#include "BasicIncludes.h"
#include <memory>
#include "FilesystemVersions.h"
#include "Utils/Rect.h"
#include "settings.h"

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
	
public:
    Renderer2D(DrawContext *ctxt, ResourceCache *res_cache)
        : Context(ctxt), resCache(res_cache)
    {
        init2DShaders();
    }
    
    void draw2DLine(MeshBuffer *line);
    void draw2DRectangle(MeshBuffer *rect);
    void draw2DImage(MeshBuffer *rect, img::Image *img, const recti &srcRect);
    
    void setDefaultRenderState();
    void setRenderState(bool alpha, bool texShader);
    void setClipRect(const recti &clipRect);
    
    void disableScissorTest();
private:
    void init2DShaders();
   
};
