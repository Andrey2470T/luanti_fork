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
extern std::array<u8, 4> primICounts;

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
        rectf texr;
		std::array<img::color8, 4> colors;

        Rectangle(const rectf &_r, const std::array<img::color8, 4> &_colors, const rectf &_texr=rectf())
            : Primitive(UIPrimitiveType::RECTANGLE), r(_r), texr(_texr), colors(_colors)
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
    void addRectangle(const rectf &r, const std::array<img::color8, 4> &colors, const rectf &texr=rectf()) {
        primitives.emplace_back((Primitive *)(new Rectangle(r, colors, texr)));
        updateMaxArea(maxArea, r.ULC, r.LRC, maxAreaInit);
    }
    void addEllipse(f32 a, f32 b, const v2f &center, const img::color8 &c) {
        primitives.emplace_back((Primitive *)(new Ellipse(a, b, center, c)));
        updateMaxArea(maxArea, center - v2f(a/2, b/2), center+v2f(a/2, b/2), maxAreaInit);
    }

    void updateLine(u32 n, const v2f &start_p, const v2f &end_p, const img::color8 &start_c, const img::color8 &end_c);
    void updateTriangle(u32 n, const v2f &p1, const v2f &p2, const v2f &p3, const img::color8 &c1, const img::color8 &c2, const img::color8 &c3);
    void updateRectangle(u32 n, const rectf &r, const std::array<img::color8, 4> &colors, const rectf &texr=rectf());
    void updateEllipse(u32 n, f32 a, f32 b, const v2f &center, const img::color8 &c);

    void movePrimitive(u32 n, const v2f &shift);
    void scalePrimitive(u32 n, const v2f &scale, std::optional<v2f> center);

    rectf getMaxArea() const
    {
        return maxArea;
    }

    static u32 countRequiredVCount(const std::vector<UIPrimitiveType> &primitives);
    static u32 countRequiredICount(const std::vector<UIPrimitiveType> &primitives);

    void removePrimitive(u32 i)
    {
        if (i >= getPrimitiveCount())
            return;

        primitives.erase(primitives.begin()+i);
        
        for (u32 i = 0; i < primitives.size(); i++) {
        	rectf primArea = getPrimitiveArea(i);
            updateMaxArea(maxArea, primArea.ULC, primArea.LRC, maxAreaInit);
        }
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

    friend class UISprite;
private:
    void appendToBuffer(MeshBuffer *buf, v2u imgSize=v2u());
    void updateBuffer(MeshBuffer *buf, u32 primitiveNum, bool pos_or_colors=true, v2u imgSize=v2u());
    void appendToBuffer(MeshBuffer *buf, u32 primitiveNum, v2u imgSize=v2u());
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
    bool visible = true;

    recti clipRect;
    
public:
    static const std::array<img::color8, 4> defaultColors;

    // Creates an empty sprite
    UISprite(render::Texture2D *tex, Renderer *_renderer, ResourceCache *_cache,
        bool streamTexture=false, bool staticUsage=true);

    // Creates a single rectangle mesh
    UISprite(render::Texture2D *tex, Renderer *_renderer,
        ResourceCache *_cache, const rectf &srcRect, const rectf &destRect,
        const std::array<img::color8, 4> &colors=defaultColors, bool streamTexture=false, bool staticUsage=true);

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

    void setTexture(render::Texture2D *tex)
    {
        texture = tex;
    }

    void rebuildMesh();
    void updateMesh(const std::vector<u32> &dirtyPrimitives, bool pos_or_colors=true);
    void updateMesh(bool pos_or_colors=true);

    void setClipRect(const recti &r)
    {
        clipRect = r;
    }

    void clear()
    {
        shape->clear();
        mesh->clear();
    }
    
    virtual void draw();

    void drawPart(u32 pOffset=0, u32 pCount=1);
};

class UITextSprite;
class FontManager;
class EnrichedString;
class RenderSystem;

struct ColoredRect
{
    rectf area;
    std::array<img::color8, 4> colors = UISprite::defaultColors;
};

typedef std::pair<u32, u32> AtlasTileAnim;

class UISpriteBank
{
    RenderSystem *rndsys;
    ResourceCache *cache;
    std::vector<std::unique_ptr<UISprite>> sprites;

    rectf maxArea;
    bool maxAreaInit = false;

    std::vector<std::vector<u32>> sprites_grid;

    v2f center;
    bool auto_align;
public:
    UISpriteBank(RenderSystem *_rndsys, ResourceCache *_cache, bool _auto_align=true)
        : rndsys(_rndsys), cache(_cache), auto_align(_auto_align)
    {}

    void setCenter(const v2f &c)
    {
        center = c;
    }
    // 'shift': '0' - shift to right, '1' - shift down
    // the sprites on each line are centered
    template<typename T, typename... Args>
    void addSprite(u8 shift, Args&&... args)
    {
        T *newSprite = new T(std::forward<Args>(args)...);
        sprites.emplace_back(dynamic_cast<UISprite *>(newSprite));
    }

    void addSprite(UISprite *sprite)
    {
        sprites.emplace_back(sprite);
    }
    void addSprite(const std::vector<ColoredRect> &rects, u8 shift, const recti *clipRect=nullptr);
    void addImageSprite(img::Image *img, u8 shift, std::optional<rectf> rect=std::nullopt, const recti *clipRect=nullptr,
        std::optional<AtlasTileAnim> anim=std::nullopt);
    void addTextSprite(FontManager *mgr, const EnrichedString &text, u8 shift,
        std::optional<v2f> pos=std::nullopt, const img::color8 &textColor=img::white, const recti *clipRect=nullptr);

    u32 getSpriteCount() const
    {
        return sprites.size();
    }
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

    void remove(u32 n)
    {
        if (n > sprites.size()-1)
            return;
        sprites.erase(sprites.begin()+n);
    }

    void clear()
    {
        sprites.clear();
    }

    void update()
    {
    	alignSpritesByCenter();
        for (auto &sprite : sprites)
            sprite->updateMesh();
    }
    
    void setClipRect(u32 n, const recti &r)
    {
    	assert(n < sprites.size());
    	sprites.at(n)->setClipRect(r);
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
    void alignSpritesByCenter();
private:
    void shiftRectByLastSprite(rectf &r, u8 shift);
};
