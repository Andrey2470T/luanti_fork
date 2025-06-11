#pragma once

#include <memory>
#include <Utils/Rect.h>
#include <Render/Texture2D.h>
//#include <Utils/MathFuncs.h>

class Renderer2D;
class MeshBuffer;
class ImageFiltered;
class ResourceCache;

enum class UIPrimitiveType : u8
{
    LINE = 0,
	TRIANGLE,
	RECTANGLE,
	ELLIPSE
};

class UIShape
{
public:
	struct Primitive {
		UIPrimitiveType type;

        constexpr static std::array<u8, 4> primVCounts = {2, 3, 4, 9};

        Primitive(UIPrimitiveType _type)
            : type(_type)
        {}
        virtual ~Primitive() = default;
	};
	struct Line : public Primitive {
		v2f start_p, end_p;
		img::color8 start_c, end_c;

        Line(const v2f &_start_p, const v2f &_end_p, const img::color8 &_start_c, const img::color8 &_end_c)
            : Primitive(UIPrimitiveType::LINE), start_p(_start_p), end_p(_end_p), start_c(_start_c), end_c(_end_c)
        {}
	};
	struct Triangle : public Primitive {
		v2f p1, p2, p3;
		img::color8 c1, c2, c3;

        Triangle(const v2f &_p1, const v2f &_p2, const v2f &_p3, const img::color8 &_c1, const img::color8 &_c2, const img::color8 &_c3)
            : Primitive(UIPrimitiveType::TRIANGLE), p1(_p1), p2(_p2), p3(_p3), c1(_c1), c2(_c2), c3(_c3)
        {}
	};
	struct Rectangle : public Primitive {
		rectf r;
		std::array<img::color8, 4> colors;

        Rectangle(const rectf &_r, const std::array<img::color8, 4> &_colors)
            : Primitive(UIPrimitiveType::RECTANGLE), r(_r), colors(_colors)
        {}
	};
	struct Ellipse : public Primitive {
		f32 a, b;
		v2f center;
        img::color8 c;

        Ellipse(f32 _a, f32 _b, const v2f &_center, const img::color8 &_c)
            : Primitive(UIPrimitiveType::ELLIPSE), a(_a), b(_b), center(_center), c(_c)
        {}
	};

	UIShape() = default;
	
	void addLine(const v2f &start_p, const v2f &end_p, const img::color8 &start_c, const img::color8 &end_c) {
        primitives.emplace_back((Primitive *)(new Line(start_p, end_p, start_c, end_c)));
        updateMaxArea(start_p, end_p);
    }
    void addTriangle(const v2f &p1, const v2f &p2, const v2f &p3, const img::color8 &c1, const img::color8 &c2, const img::color8 &c3) {
        primitives.emplace_back((Primitive *)(new Triangle(p1, p2, p3, c1, c2, c3)));
        updateMaxArea(v2f(p1.X, p3.Y), p2);
    }
    void addRectangle(const rectf &r, const std::array<img::color8, 4> &colors) {
        primitives.emplace_back((Primitive *)(new Rectangle(r, colors)));
        updateMaxArea(r.ULC, r.LRC);
    }
    void addEllipse(f32 a, f32 b, const v2f &center, const img::color8 &c) {
        primitives.emplace_back((Primitive *)(new Ellipse(a, b, center, c)));
        updateMaxArea(center - v2f(a/2, b/2), center+v2f(a/2, b/2));
    }

    void updateLine(u32 n, const v2f &start_p, const v2f &end_p, const img::color8 &start_c, const img::color8 &end_c)
    {
        auto line = dynamic_cast<Line *>(getPrimitive(n));
        line->start_p = start_p;
        line->end_p = end_p;
        line->start_c = start_c;
        line->end_c = end_c;
        updateMaxArea(start_p, end_p);
    }
    
    Primitive *getPrimitive(u32 n) const
    {
        return primitives.at(n).get();
    }
    rectf getMaxArea() const
    {
        return maxArea;
    }
    void updateBuffer(MeshBuffer *buf, u32 primitiveNum, bool pos_or_colors=true);

    void updateMaxArea(const v2f &ulc, const v2f &lrc)
    {
        maxArea.ULC.X = maxAreaInit ? std::min<f32>(maxArea.ULC.X, ulc.X) : ulc.X;
        maxArea.ULC.Y = maxAreaInit ? std::min<f32>(maxArea.ULC.Y, ulc.Y) : ulc.Y;
        maxArea.LRC.X = maxAreaInit ? std::max<f32>(maxArea.LRC.X, lrc.X) : lrc.X;
        maxArea.LRC.Y = maxAreaInit ? std::max<f32>(maxArea.LRC.Y, lrc.Y) : lrc.Y;

        maxAreaInit = true;
    }

private:
    std::vector<std::unique_ptr<Primitive>> primitives;
    rectf maxArea;
    bool maxAreaInit = false;
};

// 2D mesh consisting from some count of rectangles
// with some mapped texture on them
class UISprite
{
protected:
	Renderer2D *renderer;
	std::unique_ptr<MeshBuffer> rect;
    std::unique_ptr<UIShape> shape;

    // The texture can be simple or filtered by the MT scaling filter
	std::unique_ptr<ImageFiltered> texture;
	
	bool dirty = true;
public:
    UISprite(render::Texture2D *_tex, Renderer2D *_renderer,
        ResourceCache *cache, const rectf &srcRect, const rectf &destRect,
        const std::array<img::color8, 4> &colors, bool applyFilter=false);

    v2u getSize() const
    {
        rectf area = shape->getMaxArea();
        return v2u(area.getWidth(), area.getHeight());
    }

    UIShape *getShape() const
    {
        return shape.get();
    }
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
