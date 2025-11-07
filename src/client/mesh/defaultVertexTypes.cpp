#include "defaultVertexTypes.h"

// Node vertex type (position, color, normal, uv, materialType)
const render::VertexTypeDescriptor NodeVType{
    "Node3D",
    {{"materialType", 1, BasicType::UINT8}},
    3,
    4,
    true,
    true,
    2
};
// Node vertex type with the second (hardware) color (position, color, normal, uv, materialType, hw_color)
const render::VertexTypeDescriptor TwoColorNodeVType{
    "TwoColorNode3D",
    {
        {"materialType", 1, BasicType::UINT8},
        {"HWColor", 3, BasicType::UINT8, render::VertexAttribute::DataFormat::Normalized}
    },
    3,
    4,
	true,
	true,
	2
};
// 2D Vertex type (xy position, rgba color, uv)
const render::VertexTypeDescriptor VType2D{
    "Standard2D",
    {},
    2,
    4,
    false,
    true,
    2
};
// Bone animated object vertex type (position, color, normal, uv, materialType, bones, weights)
const render::VertexTypeDescriptor AOVType{
    "AnimatedObject3D",
    {
        {"MaterialType", 1, BasicType::UINT8},
        {"Bones", 2, BasicType::UINT32},
        {"Weights", 2, BasicType::UINT32}
    },
    3,
    4,
    true,
    true,
    2
};

// Skybox (clouds and stars) vertex type with the second (hardware) color (position, color, normal, uv, hw_color)
const render::VertexTypeDescriptor SkyboxVType{
    "Skybox3D",
    {
        {"HWColor", 4, BasicType::UINT8, render::VertexAttribute::DataFormat::Normalized}
    },
    3,
    4,
    true,
    true,
    2
};

// Getters used for DefaultVType and TwoColorVType
v3f svtGetPos(MeshBuffer *buf, u32 num)
{
    return buf->getV3FAttr(0, num);
}
v2f svtGetPos2D(MeshBuffer *buf, u32 num)
{
    return buf->getV2FAttr(0, num);
}
img::color8 svtGetColor(MeshBuffer *buf, u32 num)
{
    return buf->getRGBAAttr(1, num);
}
v3f svtGetNormal(MeshBuffer *buf, u32 num)
{
    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitNormal)
        return buf->getV3FAttr(2, num);
    return v3f();
}
v2f svtGetUV(MeshBuffer *buf, u32 num)
{
    auto vertexType = buf->getVAO()->getVertexType();
    if (vertexType.Name == "Standard2D")
        return buf->getV2FAttr(2, num);
    else
        return buf->getV2FAttr(3, num);
}
v3f svtGetUV3D(MeshBuffer *buf, u32 num)
{
    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitUV && vertexType.UVCount == 3)
        return buf->getV3FAttr(3, num);
    return v3f();
}
u8 svtGetMType(MeshBuffer *buf, u32 num)
{
    return buf->getUInt8Attr(4, num);
}
img::color8 svtGetHWColor(MeshBuffer *buf, u32 num)
{
    u32 attr_n = 4;

    if (buf->getVAO()->getVertexType().Name == "TwoColorNode3D") {
        attr_n = 5;
        return buf->getRGBAttr(attr_n, num);
    }
    else
        return buf->getRGBAAttr(attr_n, num);
}


// Setters used for DefaultVType and TwoColorVType
void svtSetPos(MeshBuffer *buf, const v3f &pos, u32 num)
{
    buf->setV3FAttr(pos, 0, num);
}
void svtSetPos2D(MeshBuffer *buf, const v2f &pos, u32 num)
{
    buf->setV2FAttr(pos, 0, num);
}
void svtSetColor(MeshBuffer *buf, const img::color8 &c, u32 num)
{
    buf->setRGBAAttr(c, 1, num);
}
void svtSetNormal(MeshBuffer *buf, const v3f &normal, u32 num)
{
    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitNormal)
        buf->setV3FAttr(normal, 2, num);
}
void svtSetUV(MeshBuffer *buf, const v2f &uv, u32 num)
{
    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitUV && vertexType.UVCount == 2) {
        if (vertexType.Name == "Standard2D")
            buf->setV2FAttr(uv, 2, num);
        else
            buf->setV2FAttr(uv, 3, num);
    }
}

void svtSetUV3D(MeshBuffer *buf, const v3f &uv, u32 num)
{
    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitUV && vertexType.UVCount == 3)
        buf->setV3FAttr(uv, 3, num);
}

void svtSetMType(MeshBuffer *buf, u8 mt, u32 num)
{
    buf->setUInt8Attr(mt, 4, num);
}

void svtSetHWColor(MeshBuffer *buf, const img::color8 &hw_c, u32 num)
{
    u32 attr_n = 4;

    if (buf->getVAO()->getVertexType().Name == "TwoColorNode3D") {
        attr_n = 5;
        buf->setRGBAttr(hw_c, attr_n, num);
    }
    else
        buf->setRGBAAttr(hw_c, attr_n, num);
}


void fillEmptyCustomAttribs(MeshBuffer *buf, u8 firstCustomAtrribIndex)
{
    auto vertexType = buf->getVAO()->getVertexType();

    u32 lastVNum = buf->getVertexCount()-1;
    for (u8 i = firstCustomAtrribIndex; i < vertexType.Attributes.size(); i++) {
        auto &attrib = vertexType.Attributes[i];

        switch (attrib.ComponentCount) {
        case 1: {

            switch (attrib.ComponentType) {
            case BasicType::UINT8:
                buf->setUInt8Attr(0, firstCustomAtrribIndex, lastVNum);
                break;
             case BasicType::CHAR:
                buf->setCharAttr(0, firstCustomAtrribIndex, lastVNum);
                break;
             case BasicType::UINT16:
                buf->setUInt16Attr(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::SHORT:
                buf->setShortAttr(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::UINT32:
                buf->setUInt32Attr(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::INT:
                buf->setIntAttr(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::FLOAT:
                buf->setFloatAttr(0.0f, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::UINT64:
                buf->setUInt64Attr(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::LONG_INT:
                 buf->setLongIntAttr(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::DOUBLE:
                buf->setDoubleAttr(0.0, firstCustomAtrribIndex, lastVNum);
                break;
            default:
                break;
            }
            break;
        }
        case 2: {
            switch (attrib.ComponentType) {
            case BasicType::UINT32:
                buf->setV2UAttr(v2u(), firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::INT:
                buf->setV2IAttr(v2i(), firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::FLOAT:
                buf->setV2FAttr(v2f(), firstCustomAtrribIndex, lastVNum);
                break;
            default:
                break;
            }
            break;
        }
        case 3: {
            switch (attrib.ComponentType) {
            case BasicType::UINT32:
                buf->setV3UAttr(v3u(), firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::INT:
                buf->setV3IAttr(v3i(), firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::FLOAT:
                buf->setV3FAttr(v3f(), firstCustomAtrribIndex, lastVNum);
                break;
            default:
                break;
            }
            break;
        }
        }
    }
}

// Appends the attributes of the standard vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void appendSVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv)
{
    u32 newVNum = buf->getVertexCount();
    u8 firstCustomAttrIndex = 2;
    buf->setV3FAttr(pos, 0, newVNum);
    buf->setRGBAAttr(c, 1, newVNum);

    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitNormal) {
        buf->setV3FAttr(normal, 2, newVNum);
        firstCustomAttrIndex = 3;
    }
    if (vertexType.InitUV) {
        buf->setV2FAttr(uv, 3, newVNum);
        firstCustomAttrIndex = 4;
    }

    fillEmptyCustomAttribs(buf, firstCustomAttrIndex);
}

// Appends the attributes of the two color vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void appendNVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, u8 matType)
{
    assert(buf->getVAO()->getVertexType().Name == "Node3D");

    u32 newVNum = buf->getVertexCount();
    buf->setV3FAttr(pos, 0, newVNum);
    buf->setRGBAAttr(c, 1, newVNum);
    buf->setV3FAttr(normal, 2, newVNum);
    buf->setV2FAttr(uv, 3, newVNum);
    buf->setUInt8Attr(matType, 4, newVNum);
}
// Appends the attributes of the two color vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void appendTCNVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, u8 matType, const img::color8 &hw_c)
{
    assert(buf->getVAO()->getVertexType().Name == "TwoColorNode3D");

    u32 newVNum = buf->getVertexCount();
    buf->setV3FAttr(pos, 0, newVNum);
    buf->setRGBAAttr(c, 1, newVNum);
    buf->setV3FAttr(normal, 2, newVNum);
    buf->setV2FAttr(uv, 3, newVNum);
    buf->setUInt8Attr(matType, 4, newVNum);
    buf->setRGBAAttr(hw_c, 5, newVNum);
}

// Appends the attributes of the standard 2D vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void appendVT2D(
    MeshBuffer *buf, const v2f &pos, const img::color8 &c, const v2f &uv)
{
    assert(buf->getVAO()->getVertexType().Name == "Standard2D");

    u32 newVNum = buf->getVertexCount();
    buf->setV2FAttr(pos, 0, newVNum);
    buf->setRGBAAttr(c, 1, newVNum);
    buf->setV2FAttr(uv, 2, newVNum);
}

void appendAOVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, u8 matType, u64 bones, u64 weights)
{
    assert(buf->getVAO()->getVertexType().Name == "AnimatedObject3D");

    u32 newVNum = buf->getVertexCount();
    buf->setV3FAttr(pos, 0, newVNum);
    buf->setRGBAAttr(c, 1, newVNum);
    buf->setV3FAttr(normal, 2, newVNum);
    buf->setV2FAttr(uv, 3, newVNum);
    buf->setUInt8Attr(matType, 4, newVNum);
    buf->setUInt64Attr(bones, 5, newVNum);
    buf->setUInt64Attr(weights, 6, newVNum);
}

void appendSBVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, const img::color8 &hw_c)
{
    assert(buf->getVAO()->getVertexType().Name == "Skybox3D");

    u32 newVNum = buf->getVertexCount();
    buf->setV3FAttr(pos, 0, newVNum);
    buf->setRGBAAttr(c, 1, newVNum);
    buf->setV3FAttr(normal, 2, newVNum);
    buf->setV2FAttr(uv, 3, newVNum);
    buf->setRGBAAttr(hw_c, 4, newVNum);
}

void appendIndex(MeshBuffer *buf, u32 index)
{
    u32 newINum = buf->getIndexCount();
    buf->setIndexAt(index, newINum);
}
