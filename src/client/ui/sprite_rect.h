#pragma once

#include <memory>
#include "Utils/Rect.h"
#include "Render/Texture2D.h"

class MeshCreator2D;
class Renderer2D;
class MeshBuffer;
class ImageFiltered;
class ResourceCache;

// Rectangle mesh with some mapped texture on it
class SpriteRect
{
	Renderer2D *renderer;
	std::unique_ptr<MeshBuffer> rect;
	rectf area;

    // The texture can be simple or filtered by the MT scaling filter
	std::unique_ptr<ImageFiltered> texture;
public:
    SpriteRect(render::Texture2D *_tex, MeshCreator2D *_creator, Renderer2D *_renderer,
        ResourceCache *cache, const rectf &srcRect, const rectf &destRect, bool applyFilter=false);

    v2u getSize() const;
    void updateRect(const rectf &r);
    void updateRect(const v2f &ulc, const v2f &lrc);
    void setClipRect(const recti &r);
    
    void move(const v2f &shift);
    void scale(const v2f &scale);
    
    void flush();
    void draw() const;
};
