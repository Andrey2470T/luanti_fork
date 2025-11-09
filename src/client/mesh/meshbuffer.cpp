#include "meshbuffer.h"
#include <assert.h>

u8 MeshBuffer::getUInt8Attr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getUInt8(vertexN, attrN);
}
u16 MeshBuffer::getUInt16Attr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getUInt16(vertexN, attrN);
}
u32 MeshBuffer::getUInt32Attr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getUInt32(vertexN, attrN);
}
f32 MeshBuffer::getFloatAttr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getFloat(vertexN, attrN);
}
v2f MeshBuffer::getV2FAttr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getV2F(vertexN, attrN);
}
v3f MeshBuffer::getV3FAttr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getV3F(vertexN, attrN);
}
matrix4 MeshBuffer::getM4x4Attr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getM4x4(vertexN, attrN);
}
img::color8 MeshBuffer::getRGBAttr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getColorRGB8(vertexN, attrN);
}
img::color8 MeshBuffer::getRGBAAttr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getColorRGBA8(vertexN, attrN);
}
img::colorf MeshBuffer::getRGBAFAttr(u32 attrN, u32 vertexN) const
{
    return VBuffer.Data->getColorf(vertexN, attrN);
}

u32 MeshBuffer::getIndexAt(u32 pos) const
{
    return IBuffer.Data->getUInt32(pos, 0);
}

void MeshBuffer::recalculateBoundingBox()
{
	u32 vcount = getVertexCount();
	if (vcount) {
        BoundingBox.reset(getV3FAttr(0, 0));

		for (u32 i = 1; i < vcount; ++i)
            BoundingBox.addInternalPoint(getV3FAttr(0, i));
	} else
		BoundingBox.reset(0, 0, 0);
}

void MeshBuffer::setUInt8Attr(u8 value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    VBuffer.Data->setUInt8(value, vertexN, attrN);
}
void MeshBuffer::setUInt16Attr(u16 value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    VBuffer.Data->setUInt16(value, vertexN, attrN);
}
void MeshBuffer::setUInt32Attr(u32 value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    VBuffer.Data->setUInt32(value, vertexN, attrN);
}
void MeshBuffer::setFloatAttr(f32 value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    VBuffer.Data->setFloat(value, vertexN, attrN);
}
void MeshBuffer::setV2FAttr(const v2f &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    VBuffer.Data->setV2F(value, vertexN, attrN);
}
void MeshBuffer::setV3FAttr(const v3f &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    VBuffer.Data->setV3F(value, vertexN, attrN);
}
void MeshBuffer::setM4x4Attr(const matrix4 &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    VBuffer.Data->setM4x4(value, vertexN, attrN);
}
void MeshBuffer::setRGBAAttr(const img::color8 &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    VBuffer.Data->setColor8(value, vertexN, attrN);
}
void MeshBuffer::setRGBAFAttr(const img::colorf &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    VBuffer.Data->setColorf(value, vertexN, attrN);
}
void MeshBuffer::setIndexAt(u32 index, u32 pos)
{
    assert(hasIBO());

    markDirty(pos, false);
    IBuffer.Data->setUInt32(index, pos, 0);
}

void MeshBuffer::reallocateData(u32 vertexCount, u32 indexCount)
{
    if (!VBuffer.Data || (hasIBO() && !IBuffer.Data))
        return;

    if (vertexCount == VBuffer.Data->count() && indexCount == IBuffer.Data->count())
        return;

    if (hasIBO())
        IBuffer.Data->reallocate(indexCount);

    VBuffer.Data->reallocate(vertexCount);
}

void MeshBuffer::uploadData()
{
    if (!VBuffer.Dirty && (hasIBO() && !IBuffer.Dirty))
        return;

    const u32 *iboData = nullptr;
    u32 iboCount = 0;

    if (hasIBO()) {
        iboData = (const u32 *)IBuffer.Data->data();
        iboCount = IBuffer.DataCount;
    }
    VAO->reallocate(VBuffer.Data->data(), VBuffer.DataCount, iboData, iboCount);
    unmarkDirty();
    unmarkDirty(false);
}

void MeshBuffer::uploadVertexData()
{
    if (VBuffer.DataCount == 0 || !VBuffer.Dirty)
        return;

    u32 startV = VBuffer.StartChange.value();
    u32 endV = VBuffer.EndChange.value();
    VAO->uploadVertexData(VBuffer.Data->data(), endV - startV + 1, startV);

    unmarkDirty();
}

void MeshBuffer::uploadIndexData()
{
    if (Type == MeshBufferType::VERTEX || IBuffer.DataCount == 0 || !IBuffer.Dirty)
        return;

    u32 startI = IBuffer.StartChange.value();
    u32 endI = IBuffer.EndChange.value();
    VAO->uploadIndexData((const u32 *)IBuffer.Data->data(), endI - startI + 1, startI);

    unmarkDirty(false);
}

void MeshBuffer::clear()
{
    if (VBuffer.Data) {
        VBuffer.Data->clear();
        VBuffer.DataCount = 0;
        VBuffer.Dirty = false;
    }

    if (hasIBO() && IBuffer.Data) {
        IBuffer.Data->clear();
        IBuffer.DataCount = 0;
        IBuffer.Dirty = false;
    }
}

MeshBuffer *MeshBuffer::copy() const
{
    MeshBuffer *new_mesh = new MeshBuffer(
        VBuffer.Data->count(), hasIBO() ? IBuffer.Data->count() : 0,
        Type != MeshBufferType::VERTEX, VAO->getVertexType());

    memcpy(new_mesh->VBuffer.Data->data(), VBuffer.Data->data(), VBuffer.Data->bytesCount());
    if (hasIBO())
        memcpy(new_mesh->IBuffer.Data->data(), IBuffer.Data->data(), IBuffer.Data->bytesCount());

    new_mesh->uploadData();
    new_mesh->setBoundingBox(getBoundingBox());

    return new_mesh;
}

void MeshBuffer::initData(u32 vertexCount, u32 indexCount)
{
    auto vType = VAO->getVertexType();

    // Convert VertexTypeDescriptor to ByteArrayDescriptor for the VBuffer
    std::vector<ByteArrayElement> elements;
    elements.resize(vType.Attributes.size());

    for (u32 i = 0; i < elements.size(); i++) {
        auto &elem = elements[i];
        auto &attribute = vType.Attributes[i];

        elem.Name = attribute.Name;

        if (attribute.ComponentType == BasicType::FLOAT) {
            if (attribute.ComponentCount == 2)
                elem.Type = ByteArrayElementType::V2F;
            else if (attribute.ComponentCount == 3)
                elem.Type = ByteArrayElementType::V3F;
            else if (attribute.ComponentCount == 4)
                elem.Type = ByteArrayElementType::COLORF;
            else if (attribute.ComponentCount == 16)
                elem.Type = ByteArrayElementType::MAT4;
            else
                elem.Type = ByteArrayElementType::FLOAT;
        }
        else if (attribute.ComponentType == BasicType::UINT8) {
            if (attribute.ComponentCount == 3)
                elem.Type = ByteArrayElementType::COLOR_RGB8;
            else if (attribute.ComponentCount == 4)
                elem.Type = ByteArrayElementType::COLOR_RGBA8;
            else
                elem.Type = ByteArrayElementType::U8;
        }
        else
            elem.Type = ByteArrayElementType::U16;
    }
    ByteArrayDescriptor vBufferDesc(vType.Name, elements);

    VBuffer.Data = std::make_unique<ByteArray>(vBufferDesc, vertexCount);

    if (hasIBO()) {
        ByteArrayDescriptor iBufferDesc("Index", {{"Index", ByteArrayElementType::U32}});
        IBuffer.Data = std::make_unique<ByteArray>(iBufferDesc, indexCount);
    }
}

void MeshBuffer::markDirty(u32 vertexN, bool vBuffer)
{
    auto &buffer = vBuffer ? VBuffer : IBuffer;
    buffer.DataCount = std::max(buffer.DataCount, vertexN+1);
    buffer.StartChange = buffer.StartChange != std::nullopt ?
        std::min(buffer.StartChange.value(), vertexN) : vertexN;
    buffer.EndChange = buffer.EndChange != std::nullopt ?
        std::max(buffer.EndChange.value(), vertexN) : vertexN;
    buffer.Dirty = true;
}
void MeshBuffer::unmarkDirty(bool vBuffer)
{
    auto &buffer = vBuffer ? VBuffer : IBuffer;
    buffer.StartChange = std::nullopt;
    buffer.EndChange = std::nullopt;
    buffer.Dirty = false;
}
