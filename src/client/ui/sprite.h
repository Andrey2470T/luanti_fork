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
class UISprite
{
protected:
	Renderer2D *renderer;
	std::unique_ptr<MeshBuffer> rect;
	rectf area;

    // The texture can be simple or filtered by the MT scaling filter
	std::unique_ptr<ImageFiltered> texture;
	
	bool dirty = true;
public:
    UISprite(render::Texture2D *_tex, MeshCreator2D *_creator, Renderer2D *_renderer,
        ResourceCache *cache, const rectf &srcRect, const rectf &destRect,
        const std::array<img::color8, 4> &colors, bool applyFilter=false);

    v2u getSize() const;
    void updateRect(const rectf &r);
    void updateRect(const v2f &ulc, const v2f &lrc);
    void setClipRect(const recti &r);
    
    void move(const v2f &shift);
    void scale(const v2f &scale);
    
    void setColors(const std::array<img::color8, 4> &colors);
    
    void flush();
    virtual void draw();
};

struct UISpriteFrame
{
    UISpriteFrame() = default;

    UISpriteFrame(u32 _imgIndex, u32 _rectIndex)
        : imgIndex(_imgIndex), rectIndex(_rectIndex)
    {}

    u32 imgIndex;
    u32 rectIndex;
};

class UIAnimatedSprite : public UISprite
{
    std::vector<std::unique_ptr<UISpriteFrame>> frames;

    std::vector<img::Image *> framesImages;
    std::vector<rectf> framesRects;

    u32 curFrameNum = 0;
    u32 frameLength = 0;
public:
    UIAnimatedSprite(v2u texSize, u32 _frameLength, MeshCreator2D *_creator, Renderer2D *_renderer,
        ResourceCache *cache, const rectf &srcRect, const rectf &destRect,
        const std::array<img::color8, 4> &colors, bool applyFilter=false);

    void addFrame(img::Image *img, const rectf &r);

    std::vector<UISpriteFrame *> getFrames() const;

    void draw() override
    {
        drawFrame(0);
    }
    void drawFrame(u32 time, bool loop=false);
};
