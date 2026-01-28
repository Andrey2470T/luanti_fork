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
    auto vertexType = buf->getVertexType();

    if (vertexType.InitNormal)
        return buf->getV3FAttr(2, num);
    return v3f();
}
v2f svtGetUV(MeshBuffer *buf, u32 num)
{
    auto vertexType = buf->getVertexType();
    if (vertexType.Name == "Standard2D")
        return buf->getV2FAttr(2, num);
    else
        return buf->getV2FAttr(3, num);
}
v3f svtGetUV3D(MeshBuffer *buf, u32 num)
{
    auto vertexType = buf->getVertexType();

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

    if (buf->getVertexType().Name == "TwoColorNode3D") {
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
    auto vertexType = buf->getVertexType();

    if (vertexType.InitNormal)
        buf->setV3FAttr(normal, 2, num);
}
void svtSetUV(MeshBuffer *buf, const v2f &uv, u32 num)
{
    auto vertexType = buf->getVertexType();

    if (vertexType.InitUV && vertexType.UVCount == 2) {
        if (vertexType.Name == "Standard2D")
            buf->setV2FAttr(uv, 2, num);
        else
            buf->setV2FAttr(uv, 3, num);
    }
}

void svtSetUV3D(MeshBuffer *buf, const v3f &uv, u32 num)
{
    auto vertexType = buf->getVertexType();

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

    if (buf->getVertexType().Name == "TwoColorNode3D") {
        attr_n = 5;
        buf->setRGBAAttr(hw_c, attr_n, num);
    }
    else
        buf->setRGBAAttr(hw_c, attr_n, num);
}


void fillEmptyCustomAttribs(MeshBuffer *buf, u8 firstCustomAtrribIndex)
{
    auto vertexType = buf->getVertexType();

    for (u8 i = firstCustomAtrribIndex; i < vertexType.Attributes.size(); i++) {
        auto &attrib = vertexType.Attributes[i];

        switch (attrib.ComponentCount) {
        case 1: {

            switch (attrib.ComponentType) {
            case BasicType::UINT8:
                buf->setUInt8Attr(0, firstCustomAtrribIndex);
                break;
             case BasicType::UINT16:
                buf->setUInt16Attr(0, firstCustomAtrribIndex);
                break;
            case BasicType::FLOAT:
                buf->setFloatAttr(0.0f, firstCustomAtrribIndex);
                break;
            default:
                break;
            }
            break;
        }
        case 2: {
            buf->setV2FAttr(v2f(), firstCustomAtrribIndex);
            break;
        }
        case 3: {
            switch (attrib.ComponentType) {
            case BasicType::UINT8:
                buf->setRGBAAttr(img::color8(img::PF_RGB8), firstCustomAtrribIndex);
                break;
            case BasicType::FLOAT:
                buf->setV3FAttr(v3f(), firstCustomAtrribIndex);
                break;
            default:
                break;
            }
            break;
        }
        case 4: {
            switch (attrib.ComponentType) {
            case BasicType::UINT8:
                buf->setRGBAAttr(img::color8(img::PF_RGBA8), firstCustomAtrribIndex);
                break;
            case BasicType::FLOAT:
                buf->setRGBAFAttr(img::colorf(), firstCustomAtrribIndex);
            default:
                break;
            }
            break;
        }
        case 16: {
            buf->setM4x4Attr(matrix4(), firstCustomAtrribIndex);
            break;
        }
        default:
            break;
        }
    }
}

// Appends the attributes of the standard vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void SVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv)
{
    u8 firstCustomAttrIndex = 2;
    buf->setV3FAttr(pos, 0);
    buf->setRGBAAttr(c, 1);

    auto vertexType = buf->getVertexType();

    if (vertexType.InitNormal) {
        buf->setV3FAttr(normal, 2);
        firstCustomAttrIndex = 3;
    }
    if (vertexType.InitUV) {
        buf->setV2FAttr(uv, 3);
        firstCustomAttrIndex = 4;
    }

    fillEmptyCustomAttribs(buf, firstCustomAttrIndex);
}

// Appends the attributes of the two color vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void NVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, u8 matType)
{
    assert(buf->getVertexType().Name == "Node3D");

    buf->setV3FAttr(pos, 0);
    buf->setRGBAAttr(c, 1);
    buf->setV3FAttr(normal, 2);
    buf->setV2FAttr(uv, 3);
    buf->setUInt8Attr(matType, 4);
}
// Appends the attributes of the two color vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void TCNVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, u8 matType, const img::color8 &hw_c)
{
    assert(buf->getVertexType().Name == "TwoColorNode3D");

    buf->setV3FAttr(pos, 0);
    buf->setRGBAAttr(c, 1);
    buf->setV3FAttr(normal, 2);
    buf->setV2FAttr(uv, 3);
    buf->setUInt8Attr(matType, 4);
    buf->setRGBAAttr(hw_c, 5);
}

// Appends the attributes of the standard 2D vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void VT2D(
    MeshBuffer *buf, const v2f &pos, const img::color8 &c, const v2f &uv)
{
    assert(buf->getVertexType().Name == "Standard2D");

    buf->setV2FAttr(pos, 0);
    buf->setRGBAAttr(c, 1);
    buf->setV2FAttr(uv, 2);
}

void AOVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, u8 matType, const v2f &bones, const v2f &weights)
{
    assert(buf->getVertexType().Name == "AnimatedObject3D");

    buf->setV3FAttr(pos, 0);
    buf->setRGBAAttr(c, 1);
    buf->setV3FAttr(normal, 2);
    buf->setV2FAttr(uv, 3);
    buf->setUInt8Attr(matType, 4);
    buf->setV2FAttr(bones, 5);
    buf->setV2FAttr(weights, 6);
}

void SBVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal, const v2f &uv, const img::color8 &hw_c)
{
    assert(buf->getVertexType().Name == "Skybox3D");

    buf->setV3FAttr(pos, 0);
    buf->setRGBAAttr(c, 1);
    buf->setV3FAttr(normal, 2);
    buf->setV2FAttr(uv, 3);
    buf->setRGBAAttr(hw_c, 4);
}
