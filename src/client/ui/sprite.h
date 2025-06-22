#pragma once

#include <memory>
#include <Utils/Rect.h>
#include <Render/Texture2D.h>
#include "client/render/meshbuffer.h"

class Renderer2D;
class MeshBuffer;
class ResourceCache;

enum class UIPrimitiveType : u8
{
    LINE = 0,
	TRIANGLE,
	RECTANGLE,
	ELLIPSE
};

extern std::array<u8, 4> primVCounts;

class UIShape
{
	struct Primitive {
		UIPrimitiveType type;

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

    std::vector<std::unique_ptr<Primitive>> primitives;
    rectf maxArea;
    bool maxAreaInit = false;
public:
	UIShape() = default;

    UIPrimitiveType getPrimitiveType(u32 n) const
    {
        auto p = primitives.at(n).get();
        return p->type;
    }
    u32 getPrimitiveCount() const
    {
        return primitives.size();
    }
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

    void updateLine(u32 n, const v2f &start_p, const v2f &end_p, const img::color8 &start_c, const img::color8 &end_c);
    void updateTriangle(u32 n, const v2f &p1, const v2f &p2, const v2f &p3, const img::color8 &c1, const img::color8 &c2, const img::color8 &c3);
    void updateRectangle(u32 n, const rectf &r, const std::array<img::color8, 4> &colors);
    void updateEllipse(u32 n, f32 a, f32 b, const v2f &center, const img::color8 &c);

    void movePrimitive(u32 n, const v2f &shift);
    void scalePrimitive(u32 n, const v2f &scale);

    rectf getMaxArea() const
    {
        return maxArea;
    }

    static u32 countRequiredVCount(const std::vector<UIPrimitiveType> &primitives);
    static u32 countRequiredICount(const std::vector<UIPrimitiveType> &primitives);
    void updateBuffer(MeshBuffer *buf, u32 primitiveNum, bool pos_or_colors=true);
    void updateMaxArea(const v2f &ulc, const v2f &lrc);

    void removePrimitive(u32 i)
    {
        if (i >= getPrimitiveCount())
            return;

        primitives.erase(primitives.begin()+i);
    }
    void clear()
    {
        primitives.clear();
        maxArea = rectf();
        maxAreaInit = false;
    }
};

// 2D mesh consisting from some count of rectangles
// with some mapped texture on them
class UISprite
{
protected:
	Renderer2D *renderer;
    ResourceCache *cache;
    std::unique_ptr<MeshBuffer> mesh;
    std::unique_ptr<UIShape> shape;

    render::Texture2D *texture = nullptr;

    bool streamTex = false;
    bool visible = false;
public:
    // Creates an empty sprite
    UISprite(render::Texture2D *tex, Renderer2D *_renderer, ResourceCache *_cache,
        bool streamTexture=false, bool staticUsage=true);

    // Creates a single rectangle mesh
    UISprite(render::Texture2D *tex, Renderer2D *_renderer,
        ResourceCache *_cache, const rectf &srcRect, const rectf &destRect,
        const std::array<img::color8, 4> &colors, bool streamTexture=false, bool staticUsage=true);

    // Creates (without buffer filling) multiple-primitive mesh
    UISprite(render::Texture2D *tex, Renderer2D *_renderer, ResourceCache *_cache,
        const std::vector<UIPrimitiveType> &primitives, bool streamTexture=false, bool staticUsage=true);

    v2u getSize() const
    {
        rectf area = shape->getMaxArea();
        return v2u(area.getWidth(), area.getHeight());
    }

    UIShape *getShape() const
    {
        return shape.get();
    }
    MeshBuffer *getBuffer() const
    {
        return mesh.get();
    }

    bool isVisible() const
    {
        return visible;
    }

    void setVisible(bool yes)
    {
        visible = yes;
    }

    void reallocateBuffer();

    virtual void setClipRect(const recti &r);
    
    void flush()
    {
        mesh->uploadVertexData();
    }
    virtual void draw();
};


// 2D animated mesh consisting from one rectangle and frames images
class UIAnimatedSprite : public UISprite
{
    std::vector<img::Image *> framesImages;
    std::vector<u32> frames;

    u32 curFrameNum = 0;
    u32 frameLength = 0;
public:
    UIAnimatedSprite(render::Texture2D *tex, u32 _frameLength, Renderer2D *_renderer,
        ResourceCache *cache, const std::array<img::color8, 4> &colors);

    u32 addImage(img::Image *img);
    void addFrame(u32 i)
    {
        frames.push_back(i);
    }

    void draw() override
    {
        drawFrame(0);
    }
    void drawFrame(u32 time, bool loop=false);
};
