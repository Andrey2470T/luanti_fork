#pragma once

#include <memory>
#include <Utils/Rect.h>
#include <Render/Texture2D.h>
#include "client/mesh/meshbuffer.h"
#include <Render/DrawContext.h>

class Renderer;
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

void updateMaxArea(rectf &curMaxArea, const v2f &primULC, const v2f &primLRC, bool &init);

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

    rectf getPrimitiveArea(u32 n) const;

    u32 getPrimitiveCount() const
    {
        return primitives.size();
    }
	void addLine(const v2f &start_p, const v2f &end_p, const img::color8 &start_c, const img::color8 &end_c) {
        primitives.emplace_back((Primitive *)(new Line(start_p, end_p, start_c, end_c)));
        updateMaxArea(maxArea, start_p, end_p, maxAreaInit);
    }
    void addTriangle(const v2f &p1, const v2f &p2, const v2f &p3, const img::color8 &c1, const img::color8 &c2, const img::color8 &c3) {
        primitives.emplace_back((Primitive *)(new Triangle(p1, p2, p3, c1, c2, c3)));
        updateMaxArea(maxArea, v2f(p1.X, p3.Y), p2, maxAreaInit);
    }
    void addRectangle(const rectf &r, const std::array<img::color8, 4> &colors) {
        primitives.emplace_back((Primitive *)(new Rectangle(r, colors)));
        updateMaxArea(maxArea, r.ULC, r.LRC, maxAreaInit);
    }
    void addEllipse(f32 a, f32 b, const v2f &center, const img::color8 &c) {
        primitives.emplace_back((Primitive *)(new Ellipse(a, b, center, c)));
        updateMaxArea(maxArea, center - v2f(a/2, b/2), center+v2f(a/2, b/2), maxAreaInit);
    }

    void updateLine(u32 n, const v2f &start_p, const v2f &end_p, const img::color8 &start_c, const img::color8 &end_c);
    void updateTriangle(u32 n, const v2f &p1, const v2f &p2, const v2f &p3, const img::color8 &c1, const img::color8 &c2, const img::color8 &c3);
    void updateRectangle(u32 n, const rectf &r, const std::array<img::color8, 4> &colors);
    void updateEllipse(u32 n, f32 a, f32 b, const v2f &center, const img::color8 &c);

    void movePrimitive(u32 n, const v2f &shift);
    void scalePrimitive(u32 n, const v2f &scale, std::optional<v2f> center);

    rectf getMaxArea() const
    {
        return maxArea;
    }

    static u32 countRequiredVCount(const std::vector<UIPrimitiveType> &primitives);
    static u32 countRequiredICount(const std::vector<UIPrimitiveType> &primitives);
    void updateBuffer(MeshBuffer *buf, u32 primitiveNum, bool pos_or_colors=true);

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

    void move(const v2f &shift)
    {
        for (u32 i = 0; i < primitives.size(); i++)
            movePrimitive(i, shift);
    }

    void scale(const v2f &scale, std::optional<v2f> c)
    {
        v2f center = c.has_value() ? c.value() : maxArea.getCenter();

        for (u32 i = 0; i < primitives.size(); i++)
            scalePrimitive(i, scale, center);
    }
};

// 2D mesh consisting from some count of rectangles
// with some mapped texture on them (or without it)
class UISprite
{
protected:
    Renderer *renderer;
    ResourceCache *cache;
    std::unique_ptr<MeshBuffer> mesh;
    std::unique_ptr<UIShape> shape;

    render::Texture2D *texture = nullptr;

    bool streamTex = false;
    bool visible = false;
public:
    // Creates an empty sprite
    UISprite(render::Texture2D *tex, Renderer *_renderer, ResourceCache *_cache,
        bool streamTexture=false, bool staticUsage=true);

    // Creates a single rectangle mesh
    UISprite(render::Texture2D *tex, Renderer *_renderer,
        ResourceCache *_cache, const rectf &srcRect, const rectf &destRect,
        const std::array<img::color8, 4> &colors, bool streamTexture=false, bool staticUsage=true);

    // Creates (without buffer filling) multiple-primitive mesh
    UISprite(render::Texture2D *tex, Renderer *_renderer, ResourceCache *_cache,
        const std::vector<UIPrimitiveType> &primitives, bool streamTexture=false, bool staticUsage=true);

    virtual ~UISprite() = default;

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

    void drawPart(u32 pOffset=0, u32 pCount=1);
};

class UITextSprite;
class FontManager;

class UISpriteBank
{
    Renderer *rnd;
    ResourceCache *cache;
    std::vector<std::unique_ptr<UISprite>> sprites;

    rectf maxArea;
    bool maxAreaInit = false;

    std::vector<std::vector<u32>> sprites_grid;

    v2f center;
public:
    UISpriteBank(Renderer *_rnd, ResourceCache *_cache)
        : rnd(_rnd), cache(_cache)
    {}

    void setCenter(const v2f &c)
    {
        center = c;
    }
    // 'shift': '0' - shift to right, '1' - shift down
    // the sprites on each line are centered
    template<typename T, typename... Args>
    void addSprite(Args&&... args, u8 shift)
    {
        T *newSprite = new T(std::forward<Args>(args)...);
        sprites.emplace_back(dynamic_cast<UISprite *>(newSprite));
    }

    void addSprite(const rectf &r, u8 shift, const std::array<img::color8, 4> &colors={img::black, img::black, img::black, img::black});
    void addImageSprite(render::Texture2D *tex, const rectf &texPart, u8 shift);
    void addTextSprite(FontManager *mgr, const std::wstring &text, u8 shift);

    UISprite *getSprite(u32 n) const
    {
        assert(n < sprites.size());
        return sprites.at(n).get();
    }

    void move(const v2f &shift)
    {
        for (auto &sprite : sprites) {
            sprite->getShape()->move(shift);
            auto primArea = sprite->getShape()->getMaxArea();
            updateMaxArea(maxArea, primArea.ULC, primArea.LRC, maxAreaInit);
        }
    }

    void scale(const v2f &scale)
    {
        v2f center = maxArea.getCenter();

        for (auto &sprite : sprites) {
            sprite->getShape()->scale(scale, center);
            auto primArea = sprite->getShape()->getMaxArea();
            updateMaxArea(maxArea, primArea.ULC, primArea.LRC, maxAreaInit);
        }
    }

    void update()
    {
        for (auto &sprite : sprites) {
            for (u32 i = 0; i < sprite->getShape()->getPrimitiveCount(); i++)
                sprite->getShape()->updateBuffer(sprite->getBuffer(), i, true);

            sprite->flush();
        }
    }

    void drawSprite(u32 n)
    {
        assert(n < sprites.size());
        sprites.at(n)->draw();
    }

    void drawBank()
    {
        for (auto &sprite : sprites)
            sprite->draw();
    }
private:
    void shiftRectByLastSprite(rectf &r, u8 shift);
    void alignSpritesByCenter();
};
