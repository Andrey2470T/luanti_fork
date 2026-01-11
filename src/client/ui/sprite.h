#pragma once

#include <memory>
#include <Utils/Rect.h>
#include <Render/Texture2D.h>
#include "Utils/TypeConverter.h"
#include "client/mesh/meshbuffer.h"
#include "gui/GUIEnums.h"
#include <Render/DrawContext.h>
#include <unordered_set>
#include <variant>
#include <tuple>

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

struct RectColors
{
    static const std::array<img::color8, 4> defaultColors;
    std::array<img::color8, 4> colors=defaultColors;

    RectColors(const img::color8 &color=img::white)
        : colors{color, color, color, color}
    {}
    RectColors(const img::color8 &color1, const img::color8 &color2, const img::color8 &color3, const img::color8 &color4)
        : colors{color1, color2, color3, color4}
    {}
    RectColors(const std::array<img::color8, 4> &_colors)
        : colors(_colors)
    {}

    const img::color8 &operator[](u8 index) const
    {
        return colors.at(index);
    }
    img::color8 &operator[](u8 index)
    {
        return colors.at(index);
    }
};

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
        RectColors colors;

        Rectangle(const rectf &_r, const RectColors &_colors, const rectf &_texr=rectf())
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
    std::unordered_set<u32> dirtyPrimitives;

    rectf maxArea;
    bool maxAreaInit = false;
public:
	UIShape() = default;

    UIShape(const UIShape&) = delete;
    UIShape& operator=(const UIShape&) = delete;

    UIShape(UIShape&&) = default;
    UIShape& operator=(UIShape&&) = default;

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
        const RectColors &colors,
        const rectf &texr=rectf(), // in pixel coords
        const v2u &imgSize=v2u(1,1));
    void addEllipse(f32 a, f32 b, const v2f &center, const img::color8 &c);

    void updateLine(
        u32 n,
        const v2f &start_p, const v2f &end_p,
        const img::color8 &start_c, const img::color8 &end_c);
    void updateTriangle(
        u32 n,
        const v2f &p1, const v2f &p2, const v2f &p3,
        const img::color8 &c1, const img::color8 &c2, const img::color8 &c3);
    void updateRectangle(
        u32 n,
        const rectf &r, const RectColors &colors,
        const rectf &texr=rectf(),  const v2u &imgSize=v2u(1,1));
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

    void updateBuffer(MeshBuffer *buf, u32 vertexOffset=0, u32 indexOffset=0);
private:
    void updatePrimInBuffer(MeshBuffer *buf, u32 primitiveNum, u32 vertexOffset=0, u32 indexOffset=0);
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

    // The toppest rendered sprite has the highest level
    u8 depthLevel = 0;

public:
    bool changed = false;

    UISprite(ResourceCache *_cache, SpriteDrawBatch *_drawBatch, u32 _depthLevel=0)
        : cache(_cache), drawBatch(_drawBatch), depthLevel(_depthLevel)
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
    u8 getDepthLevel() const
    {
        return depthLevel;
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
        changed = true;
    }

    void clear()
    {
        shape.clear();
        changed = true;
    }
    
    // Appends new draw chunks in the draw batch
    virtual void appendToBatch() = 0;
    //  Update the meshbuffer directly
    virtual void updateBatch() = 0;
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
    RectColors colors = RectColors::defaultColors;
    img::Image *image = nullptr;
    std::optional<AtlasTileAnim> anim=std::nullopt;

    TexturedRect() = default;

    TexturedRect(const rectf& _area, const RectColors& _colors, img::Image* _image = nullptr,
        std::optional<AtlasTileAnim> _anim=std::nullopt)
        : area(_area), colors(_colors), image(_image), anim(_anim)
    {}

    TexturedRect(const rectf& _area, const img::color8& singleColor, img::Image* _image = nullptr,
        std::optional<AtlasTileAnim> _anim=std::nullopt)
        : area(_area), colors(singleColor), image(_image), anim(_anim)
    {}
};

class UIRects;
class Image2D9Slice;
class UITextSprite;

struct SpriteDrawChunk
{
    render::Texture2D *texture = nullptr;
    recti clipRect;
    u32 rectsN;

    SpriteDrawChunk() = default;

    SpriteDrawChunk(render::Texture2D *_texture, const recti &_clipRect, u32 _rectsN)
        : texture(_texture), clipRect(_clipRect), rectsN(_rectsN)
    {}
};

class SpriteDrawBatch
{
    RenderSystem *rndsys;
    ResourceCache *cache;
    std::vector<std::unique_ptr<UISprite>> sprites;

    std::unique_ptr<MeshBuffer> buffer;

    std::unordered_map<UISprite *, std::vector<SpriteDrawChunk>> chunks;

    typedef std::unordered_map<render::Texture2D *,
        std::vector<std::pair<recti, std::vector<std::tuple<UISprite *, u32, u32>>>>> DrawSubBatch;

    std::unordered_map<u32, DrawSubBatch> subBatches; // map depth level to its subbatch

    rectf maxArea;
    bool maxAreaInit = false;

    bool updateBatch = false;
public:
    SpriteDrawBatch(RenderSystem *_rndsys, ResourceCache *_cache);

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
        const recti *clipRect=nullptr,
        u32 depthLevel=0);
    Image2D9Slice *addImage2D9Slice(
        const rectf &src_rect, const rectf &dest_rect,
        const rectf &middle_rect, img::Image *baseImg,
        u32 depthLevel=0,
        const RectColors &colors=RectColors::defaultColors,
        std::optional<AtlasTileAnim> anim=std::nullopt);
    UITextSprite *addTextSprite(
        const std::wstring &text,
        std::optional<std::variant<rectf, v2f>> shift=std::nullopt,
        const img::color8 &textColor=img::white,
        const recti *clipRect=nullptr,
        u32 depthLevel=0,
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

    void move(const v2f &shift);

    void scale(const v2f &scale);

    void remove(u32 n);

    void clear()
    {
        sprites.clear();
        chunks.clear();
    }

    void addSpriteChunks(UISprite *sprite, const std::vector<SpriteDrawChunk> &addchunks)
    {
        auto &chunk = chunks[sprite];
        chunk.clear();
        chunk.insert(chunk.begin(), addchunks.begin(), addchunks.end());

        updateBatch = true;
    }
    
    void setClipRect(u32 n, const recti &r)
    {
    	assert(n < sprites.size());
    	sprites.at(n)->setClipRect(r);

        updateBatch = true;
    }

    void rebuild();
    void update();

    void draw();
private:
    void batch();
    bool extendRectsRange(std::tuple<UISprite *, u32, u32> &curRange, const std::pair<u32, u32> &newRange);
};
