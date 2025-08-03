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
        {"HWColor", 4, BasicType::UINT8, render::VertexAttribute::DataFormat::Normalized}
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
    }
};

// Getters used for DefaultVType and TwoColorVType
inline v3f svtGetPos(MeshBuffer *buf, u32 num)
{
	return buf->getAttrAt<v3f>(0, num);
}
inline v2f svtGetPos2D(MeshBuffer *buf, u32 num)
{
    return buf->getAttrAt<v2f>(0, num);
}
inline img::color8 svtGetColor(MeshBuffer *buf, u32 num)
{
	return buf->getAttrAt<img::color8>(1, num);
}
inline v3f svtGetNormal(MeshBuffer *buf, u32 num)
{
	return buf->getAttrAt<v3f>(2, num);
}
inline v2f svtGetUV(MeshBuffer *buf, u32 num)
{
	return buf->getAttrAt<v2f>(3, num);
}
inline v3f svtGetUV3D(MeshBuffer *buf, u32 num)
{
    return buf->getAttrAt<v3f>(3, num);
}
inline u8 svtGetMType(MeshBuffer *buf, u32 num)
{
    return buf->getAttrAt<u8>(4, num);
}
inline img::color8 svtGetHWColor(MeshBuffer *buf, u32 num)
{
    return buf->getAttrAt<img::color8>(5, num);
}


// Setters used for DefaultVType and TwoColorVType
inline void svtSetPos(MeshBuffer *buf, const v3f &pos, u32 num)
{
	buf->setAttrAt<v3f>(pos, 0, num);
}
inline void svtSetPos2D(MeshBuffer *buf, const v2f &pos, u32 num)
{
    buf->setAttrAt<v2f>(pos, 0, num);
}
inline void svtSetColor(MeshBuffer *buf, const img::color8 &c, u32 num)
{
	buf->setAttrAt<img::color8>(c, 1, num);
}
inline void svtSetNormal(MeshBuffer *buf, const v3f &normal, u32 num)
{
    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitNormal)
        buf->setAttrAt<v3f>(normal, 2, num);
}
inline void svtSetUV(MeshBuffer *buf, const v2f &uv, u32 num)
{
    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitUV && vertexType.UVCount == 2)
        buf->setAttrAt<v2f>(uv, 3, num);
}

inline void svtSetUV3D(MeshBuffer *buf, const v3f &uv, u32 num)
{
    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitUV && vertexType.UVCount == 3)
        buf->setAttrAt<v3f>(uv, 3, num);
}

inline void svtSetMType(MeshBuffer *buf, u8 mt, u32 num)
{
    buf->setAttrAt<u8>(mt, 4, num);
}

inline void svtSetHWColor(MeshBuffer *buf, const img::color8 &hw_c, u32 num)
{
    buf->setAttrAt<img::color8>(hw_c, 5, num);
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
                buf->setAttrAt<u8>(0, firstCustomAtrribIndex, lastVNum);
                break;
             case BasicType::CHAR:
                buf->setAttrAt<char>(0, firstCustomAtrribIndex, lastVNum);
                break;
             case BasicType::UINT16:
                buf->setAttrAt<u16>(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::SHORT:
                buf->setAttrAt<s16>(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::UINT32:
                buf->setAttrAt<u32>(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::INT:
                buf->setAttrAt<s32>(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::FLOAT:
                buf->setAttrAt<f32>(0.0f, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::UINT64:
                buf->setAttrAt<u64>(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::LONG_INT:
                 buf->setAttrAt<long int>(0, firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::DOUBLE:
                buf->setAttrAt<f64>(0.0, firstCustomAtrribIndex, lastVNum);
                break;
            default:
                break;
            }
        }
        case 2: {
            switch (attrib.ComponentType) {
            case BasicType::UINT32:
                buf->setAttrAt<v2u>(v2u(), firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::INT:
                buf->setAttrAt<v2i>(v2i(), firstCustomAtrribIndex, lastVNum);
            case BasicType::FLOAT:
                buf->setAttrAt<v2f>(v2f(), firstCustomAtrribIndex, lastVNum);
            default:
                break;
            }
        }
        case 3: {
            switch (attrib.ComponentType) {
            case BasicType::UINT32:
                buf->setAttrAt<v3u>(v3u(), firstCustomAtrribIndex, lastVNum);
                break;
            case BasicType::INT:
                buf->setAttrAt<v3i>(v3i(), firstCustomAtrribIndex, lastVNum);
            case BasicType::FLOAT:
                buf->setAttrAt<v3f>(v3f(), firstCustomAtrribIndex, lastVNum);
            default:
                break;
            }
        }
        }
    }
}

// Appends the attributes of the standard vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
inline void appendSVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv)
{
    u32 newVNum = buf->getVertexCount();
    u8 firstCustomAttrIndex = 2;
    buf->setAttrAt<v3f>(pos, 0, newVNum);
    buf->setAttrAt<img::color8>(c, 1, newVNum);

    auto vertexType = buf->getVAO()->getVertexType();

    if (vertexType.InitNormal) {
        buf->setAttrAt<v3f>(normal, 2, newVNum);
        firstCustomAttrIndex = 3;
    }
    if (vertexType.InitUV) {
        buf->setAttrAt<v2f>(uv, 3, newVNum);
        firstCustomAttrIndex = 4;
    }

    fillEmptyCustomAttribs(buf, firstCustomAttrIndex);
}

// Appends the attributes of the two color vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
inline void appendNVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, u8 matType)
{
    assert(buf->getVAO()->getVertexType().Name == "Node3D");

    u32 newVNum = buf->getVertexCount();
    buf->setAttrAt<v3f>(pos, 0, newVNum);
    buf->setAttrAt<img::color8>(c, 1, newVNum);
    buf->setAttrAt<v3f>(normal, 2, newVNum);
    buf->setAttrAt<v2f>(uv, 3, newVNum);
    buf->setAttrAt<u8>(matType, 4, newVNum);
}
// Appends the attributes of the two color vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
inline void appendTCNVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, u8 matType, const img::color8 &hw_c)
{
    assert(buf->getVAO()->getVertexType().Name == "TwoColorNode3D");

    u32 newVNum = buf->getVertexCount();
    buf->setAttrAt<v3f>(pos, 0, newVNum);
    buf->setAttrAt<img::color8>(c, 1, newVNum);
    buf->setAttrAt<v3f>(normal, 2, newVNum);
    buf->setAttrAt<v2f>(uv, 3, newVNum);
    buf->setAttrAt<u8>(matType, 4, newVNum);
    buf->setAttrAt<img::color8>(hw_c, 5, newVNum);
}

// Appends the attributes of the standard 2D vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
inline void appendVT2D(
    MeshBuffer *buf, const v2f &pos, const img::color8 &c, const v2f &uv)
{
    assert(buf->getVAO()->getVertexType().Name == "Standard2D");

    u32 newVNum = buf->getVertexCount();
    buf->setAttrAt<v2f>(pos, 0, newVNum);
    buf->setAttrAt<img::color8>(c, 1, newVNum);
    buf->setAttrAt<v2f>(uv, 2, newVNum);
}

inline void appendIndex(MeshBuffer *buf, u32 index)
{
    u32 newINum = buf->getIndexCount();
    buf->setIndexAt(index, newINum);
}
