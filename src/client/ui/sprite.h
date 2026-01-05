#pragma once

#include <memory>
#include <Utils/Rect.h>
#include <Render/Texture2D.h>
#include "Utils/TypeConverter.h"
#include "client/mesh/meshbuffer.h"
#include "gui/GUIEnums.h"
#include <Render/DrawContext.h>
#include <variant>

class RenderSystem;
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

        Line(const v2f &_start_p, const v2f &_end_p,
             const img::color8 &_start_c, const img::color8 &_end_c)
            : Primitive(UIPrimitiveType::LINE), start_p(_start_p), end_p(_end_p),
              start_c(_start_c), end_c(_end_c)
        {}
	};
	struct Triangle : public Primitive {
		v2f p1, p2, p3;
		img::color8 c1, c2, c3;

        Triangle(const v2f &_p1, const v2f &_p2, const v2f &_p3,
                 const img::color8 &_c1, const img::color8 &_c2, const img::color8 &_c3)
            : Primitive(UIPrimitiveType::TRIANGLE), p1(_p1), p2(_p2), p3(_p3),
              c1(_c1), c2(_c2), c3(_c3)
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
    std::vector<u32> dirtyPrimitives;

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
    void addLine(
        const v2f &start_p, const v2f &end_p,
        const img::color8 &start_c, const img::color8 &end_c);
    void addTriangle(
        const v2f &p1, const v2f &p2, const v2f &p3,
        const img::color8 &c1, const img::color8 &c2, const img::color8 &c3);
    void addRectangle(
        const rectf &r,
        const std::array<img::color8, 4> &colors,
        const rectf &texr=rectf());
    void addEllipse(f32 a, f32 b, const v2f &center, const img::color8 &c);

    void updateLine(
        u32 n, const v2f &start_p, const v2f &end_p,
        const img::color8 &start_c, const img::color8 &end_c);
    void updateTriangle(
        u32 n, const v2f &p1, const v2f &p2, const v2f &p3,
        const img::color8 &c1, const img::color8 &c2, const img::color8 &c3);
    void updateRectangle(
        u32 n, const rectf &r, const std::array<img::color8, 4> &colors, const rectf &texr=rectf());
    void updateEllipse(u32 n, f32 a, f32 b, const v2f &center, const img::color8 &c);

    void movePrimitive(u32 n, const v2f &shift);
    void scalePrimitive(u32 n, const v2f &scale, std::optional<v2f> center);

    rectf getMaxArea() const
    {
        return maxArea;
    }

    static u32 countRequiredVCount(const std::vector<UIPrimitiveType> &primitives);
    static u32 countRequiredICount(const std::vector<UIPrimitiveType> &primitives);

    void removePrimitive(u32 i);
    void clear();

    void move(const v2f &shift);
    void scale(const v2f &scale, std::optional<v2f> c);

    friend class UISprite;
private:
    void appendToBuffer(
        MeshBuffer *buf, v2u imgSize=v2u(), bool toUV=false);
    void updateBuffer(
        MeshBuffer *buf, bool positions=true, bool colors=true,
        v2u imgSize=v2u(), bool toUV=false);
    void updateBuffer(
        MeshBuffer *buf, u32 primitiveNum, bool positions=true, bool colors=true,
        v2u imgSize=v2u(), bool toUV=false);
    void appendToBuffer(
        MeshBuffer *buf, u32 primitiveNum, v2u imgSize=v2u(), bool toUV=false);
};

class SpriteDrawBatch;

// 2D mesh consisting from some count of rectangles
// with some mapped texture on them (or without it)
class UISprite
{
protected:
    ResourceCache *cache;
    SpriteDrawBatch *drawBatch;

    UIShape shape;

    bool visible = true;

    recti clipRect;
public:
    static const std::array<img::color8, 4> defaultColors;

    UISprite(ResourceCache *_cache, SpriteDrawBatch *_drawBatch)
        : cache(_cache), drawBatch(_drawBatch)
    {}

    virtual ~UISprite() = default;

    v2u getSize() const
    {
        rectf area = shape.getMaxArea();
        return toV2T<u32>(area.getSize());
    }
    rectf getArea() const
    {
        return shape.getMaxArea();
    }

    UIShape &getShape()
    {
        return shape;
    }
    const UIShape &getShape() const
    {
        return shape;
    }

    bool isVisible() const
    {
        return visible;
    }

    void setVisible(bool yes)
    {
        visible = yes;
    }

    void setClipRect(const recti &r)
    {
        clipRect = r;
    }

    void clear()
    {
        shape.clear();
    }
    
    // Appends new draw chunks in the draw batch or update the meshbuffer directly
    virtual void updateBatch() = 0;
    
    // Draws all or part of primitives
    /*virtual void draw(
        std::optional<u32> primOffset=std::nullopt,
        std::optional<u32> primCount=std::nullopt) = 0;*/
//protected:
    //bool checkPrimitives(std::optional<u32> &offset, std::optional<u32> &count);
    //void drawPart(u32 pOffset=0, u32 pCount=1);
};

/*enum class BankAlignmentType : u8
{
    HORIZONTAL,
    VERTICAL,
    DISABLED
};

class BankAutoAlignment
{
    BankAlignmentType alignType;

    v2f center;

    UISpriteBank *bank = nullptr;
public:
    BankAutoAlignment(BankAlignmentType _alignType, UISpriteBank *_bank)
        : alignType(_alignType), bank(_bank)
    {
        assert(alignType != BankAlignmentType::DISABLED);
    }

    void setCenter(const v2f &c)
    {
        center = c;
    }

    void centerBank();
    void alignSprite(u32 spriteID, std::optional<rectf> overrideRect=std::nullopt);
};*/

typedef std::pair<u32, u32> AtlasTileAnim;

struct TexturedRect
{
    rectf area;
    std::array<img::color8, 4> colors = UISprite::defaultColors;
    img::Image *image = nullptr;
    std::optional<AtlasTileAnim> anim=std::nullopt;
};

class UIRects;
class UITextSprite;

struct SpriteDrawChunk
{
    render::Texture2D *texture = nullptr;
    rectf clipRect;

    u32 indexOffset = 0; // index offset inside the meshbuffer
    u32 indexCount;
};

class SpriteDrawBatch
{
    RenderSystem *rndsys;
    ResourceCache *cache;
    std::vector<std::unique_ptr<UISprite>> sprites;

    std::unique_ptr<MeshBuffer> buffer;

    std::vector<SpriteDrawChunk> chunks;

    rectf maxArea;
    bool maxAreaInit = false;
    bool groupByTexture = false;
public:
    SpriteDrawBatch(RenderSystem *_rndsys, ResourceCache *_cache)
        : rndsys(_rndsys), cache(_cache)
    {}

    v2u getSize() const
    {
        return toV2T<u32>(maxArea.getSize());
    }
    rectf getArea() const
    {
        return maxArea;
    }

    void addSprite(UISprite *sprite)
    {
        sprites.emplace_back(sprite);
    }
    UIRects *addRectsSprite(
        const std::vector<TexturedRect> &rects,
        const recti *clipRect=nullptr);
    UITextSprite *addTextSprite(
        const std::wstring &text,
        std::optional<std::variant<rectf, v2f>> shift=std::nullopt,
        const img::color8 &textColor=img::white,
        const recti *clipRect=nullptr,
        bool wordWrap=false,
        GUIAlignment horizAlign=GUIAlignment::Center,
        GUIAlignment vertAlign=GUIAlignment::Center);

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
            sprite->getShape().move(shift);
            auto primArea = sprite->getShape().getMaxArea();
            updateMaxArea(maxArea, primArea.ULC, primArea.LRC, maxAreaInit);
        }
    }

    void scale(const v2f &scale)
    {
        v2f center = maxArea.getCenter();

        for (auto &sprite : sprites) {
            sprite->getShape().scale(scale, center);
            auto primArea = sprite->getShape().getMaxArea();
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
        chunks.clear();
    }

    void addSpriteChunks(const std::vector<SpriteDrawChunk> &chunks);
    
    void setClipRect(u32 n, const recti &r)
    {
    	assert(n < sprites.size());
    	sprites.at(n)->setClipRect(r);
    }

    void update();

    void draw()
    {
        for (auto &sprite : sprites)
            sprite->draw();
    }
};
