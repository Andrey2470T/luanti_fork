#pragma once

#include <memory>
#include "Utils/Rect.h"
#include "Render/Texture2D.h"

class Renderer2D;
class MeshBuffer;
class ImageFiltered;
class ResourceCache;

// 2D mesh consisting from some count of rectangles
// with some mapped texture on them
class UISprite
{
protected:
	Renderer2D *renderer;
	std::unique_ptr<MeshBuffer> rect;
	rectf area;

    std::vector<rectf> subRects;

    // The texture can be simple or filtered by the MT scaling filter
	std::unique_ptr<ImageFiltered> texture;
	
	bool dirty = true;
public:
    UISprite(render::Texture2D *_tex, Renderer2D *_renderer,
        ResourceCache *cache, const rectf &srcRect, const rectf &destRect,
        const std::array<img::color8, 4> &colors, bool applyFilter=false);

    v2u getSize() const;

    void addRect(const rectf &srcRect, const rectf &destRect,
        const std::array<img::color8, 4> &colors);
    void updateRect(u32 n, const rectf &r);
    void updateRect(u32 n, const v2f &ulc, const v2f &lrc);
    void setClipRect(const recti &r);
    
    void move(u32 n, const v2f &shift);
    void scale(u32 n, const v2f &scale);
    
    void setColors(u32 n, const std::array<img::color8, 4> &colors);
    void setColor(u32 n, const img::color8 &color)
    {
        setColors(n, {color, color, color, color});
    }
    
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

// 2D animated mesh consisting from one rectangle and frames images
class UIAnimatedSprite : public UISprite
{
    std::vector<std::unique_ptr<UISpriteFrame>> frames;

    std::vector<img::Image *> framesImages;
    std::vector<rectf> framesRects;

    u32 curFrameNum = 0;
    u32 frameLength = 0;
public:
    UIAnimatedSprite(v2u texSize, u32 _frameLength, Renderer2D *_renderer,
        ResourceCache *cache, const std::array<img::color8, 4> &colors, bool applyFilter=false);

    void addFrame(img::Image *img, const rectf &r);

    std::vector<UISpriteFrame *> getFrames() const;

    void draw() override
    {
        drawFrame(0);
    }
    void drawFrame(u32 time, bool loop=false);
};

class UISpriteBank
{
    ResourceCache *cache;
    Renderer2D *renderer;
    std::vector<std::unique_ptr<UIAnimatedSprite>> sprites;
public:
    UISpriteBank(ResourceCache *_cache, Renderer2D *_renderer)
        : cache(_cache), renderer(_renderer)
    {}
    UISpriteBank &operator=(const UISpriteBank &bank)
    {
        clear();
        
        sprites.resize(bank.getSpritesCount());
        for (u32 i = 0; i < bank.getSpritesCount(); i++)
            sprites.at(i).reset(bank.getSprite(i));

        return *this;
    }

    std::vector<UIAnimatedSprite *> getSprites() const;

    u32 getSpritesCount() const
    {
        return sprites.size();
    }

    UIAnimatedSprite *getSprite(u32 index) const
    {
        if (index > sprites.size()-1)
            return nullptr;

        return sprites.at(index).get();
    }

    void addSprite(const rectf &r, img::Image *img, u32 frameLength,
        const std::array<img::color8, 4> &colors=std::array<img::color8, 4>(), bool applyFilter=false)
    {
        sprites.emplace_back(img->getSize(), frameLength, renderer, cache, colors, applyFilter);
        sprites.back().get()->addFrame(img, r);
    }
    void addImageAsSprite(img::Image *img, u32 frameLength)
    {
        v2u size = img->getSize();
        addSprite(rectf(v2f(size.X, size.Y)), img, frameLength);
    }
    void clear()
    {
        sprites.clear();
    }

    void drawSprite(u32 index, u32 curTime, bool loop=false);
    void drawSprites(u32 curTime, bool loop=false)
    {
    	for (u32 i = 0; i < sprites.size(); i++)
            drawSprite(i, curTime, loop);
    }
};
