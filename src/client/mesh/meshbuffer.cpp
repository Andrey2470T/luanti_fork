#include "meshbuffer.h"
#include <assert.h>

u32 MeshBuffer::getSubDataCount(u32 baIndex, bool vBuffer) const
{
    auto &buffer = vBuffer ? VBuffer : IBuffer;
    return buffer.Data.at(baIndex)->count();
}

u8 MeshBuffer::getUInt8Attr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getUInt8(dataOffset, attrN);
}
u16 MeshBuffer::getUInt16Attr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getUInt16(dataOffset, attrN);
}
u32 MeshBuffer::getUInt32Attr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getUInt32(dataOffset, attrN);
}
f32 MeshBuffer::getFloatAttr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getFloat(dataOffset, attrN);
}
v2f MeshBuffer::getV2FAttr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getV2F(dataOffset, attrN);
}
v3f MeshBuffer::getV3FAttr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getV3F(dataOffset, attrN);
}
matrix4 MeshBuffer::getM4x4Attr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getM4x4(dataOffset, attrN);
}
img::color8 MeshBuffer::getRGBAttr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getColorRGB8(dataOffset, attrN);
}
img::color8 MeshBuffer::getRGBAAttr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getColorRGBA8(dataOffset, attrN);
}
img::colorf MeshBuffer::getRGBAFAttr(u32 attrN, u32 vertexN) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    return VBuffer.Data.at(baIndex)->getColorf(dataOffset, attrN);
}

u32 MeshBuffer::getIndexAt(u32 pos) const
{
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(pos, baIndex, dataOffset, false);
    return IBuffer.Data.at(baIndex)->getUInt32(dataOffset, 0);
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
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    VBuffer.Data.at(baIndex)->setUInt8(value, dataOffset, attrN);
}
void MeshBuffer::setUInt16Attr(u16 value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    VBuffer.Data.at(baIndex)->setUInt16(value, dataOffset, attrN);
}
void MeshBuffer::setUInt32Attr(u32 value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    VBuffer.Data.at(baIndex)->setUInt32(value, dataOffset, attrN);
}
void MeshBuffer::setFloatAttr(f32 value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    VBuffer.Data.at(baIndex)->setFloat(value, dataOffset, attrN);
}
void MeshBuffer::setV2FAttr(const v2f &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    VBuffer.Data.at(baIndex)->setV2F(value, dataOffset, attrN);
}
void MeshBuffer::setV3FAttr(const v3f &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    VBuffer.Data.at(baIndex)->setV3F(value, dataOffset, attrN);
}
void MeshBuffer::setM4x4Attr(const matrix4 &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    VBuffer.Data.at(baIndex)->setM4x4(value, dataOffset, attrN);
}
void MeshBuffer::setRGBAAttr(const img::color8 &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    VBuffer.Data.at(baIndex)->setColor8(value, dataOffset, attrN);
}
void MeshBuffer::setRGBAFAttr(const img::colorf &value, u32 attrN, u32 vertexN)
{
    markDirty(vertexN);
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(vertexN, baIndex, dataOffset);
    VBuffer.Data.at(baIndex)->setColorf(value, dataOffset, attrN);
}
void MeshBuffer::setIndexAt(u32 index, u32 pos)
{
    assert(hasIBO());

    markDirty(pos, false);
    u32 baIndex = 0, dataOffset;
    calcDataIndexAndOffset(pos, baIndex, dataOffset, false);
    IBuffer.Data.at(baIndex)->setUInt32(index, dataOffset, 0);
}

void MeshBuffer::reallocateData(u32 vertexCount, u32 indexCount, u32 baIndex)
{
    for (u32 i = VBuffer.Data.size(); i <= baIndex; i++)
        addSubData();

    if (hasIBO()) {
        for (u32 i = IBuffer.Data.size(); i <= baIndex; i++)
            addSubData(0, false);
    }

    if (vertexCount == VBuffer.Data.at(baIndex)->count() && (hasIBO() && indexCount == IBuffer.Data.at(baIndex)->count()))
        return;

    if (hasIBO())
        IBuffer.Data.at(baIndex)->reallocate(indexCount);

    VBuffer.Data.at(baIndex)->reallocate(vertexCount);
}

void MeshBuffer::uploadData()
{
    if (!VBuffer.Dirty && (hasIBO() && !IBuffer.Dirty))
        return;

    auto totalVertices = mergeByteArrays();

    u8 *totalIndices = nullptr;
    u32 iboCount = 0;

    if (hasIBO()) {
        totalIndices = mergeByteArrays(false);
        iboCount = IBuffer.DataCount;
    }
    VAO->reallocate(totalVertices, VBuffer.DataCount, (u32 *)totalIndices, iboCount);

    delete[] totalVertices;
    delete[] totalIndices;

    unmarkDirty();
    unmarkDirty(false);
}

void MeshBuffer::uploadVertexData()
{
    if (VBuffer.DataCount == 0 || !VBuffer.Dirty)
        return;

    auto totalVertices = mergeByteArrays();
    u32 startV = VBuffer.StartChange.value();
    u32 endV = VBuffer.EndChange.value();
    VAO->uploadVertexData(totalVertices, endV - startV + 1, startV);

    delete[] totalVertices;

    unmarkDirty();
}

void MeshBuffer::uploadIndexData()
{
    if (Type == MeshBufferType::VERTEX || IBuffer.DataCount == 0 || !IBuffer.Dirty)
        return;

    auto totalIndices = mergeByteArrays(false);
    u32 startI = IBuffer.StartChange.value();
    u32 endI = IBuffer.EndChange.value();
    VAO->uploadIndexData((const u32 *)totalIndices, endI - startI + 1, startI);

    delete[] totalIndices;

    unmarkDirty(false);
}

void MeshBuffer::addSubData(u32 count, bool vBuffer)
{
    if (vBuffer) {
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
        VBuffer.Data.emplace_back(std::make_unique<ByteArray>(vBufferDesc, count));
    }
    else {
        ByteArrayDescriptor iBufferDesc("Index", {{"Index", ByteArrayElementType::U32}});
        IBuffer.Data.emplace_back(std::make_unique<ByteArray>(iBufferDesc, count));
    }
}

void MeshBuffer::clear()
{
    VBuffer.Data.clear();
    VBuffer.DataCount = 0;
    VBuffer.Dirty = false;

    IBuffer.Data.clear();
    IBuffer.DataCount = 0;
    IBuffer.Dirty = false;
}

MeshBuffer *MeshBuffer::copy() const
{
    MeshBuffer *new_mesh = new MeshBuffer(
        VBuffer.Data.at(0)->count(), hasIBO() ? IBuffer.Data.at(0)->count() : 0,
        Type != MeshBufferType::VERTEX, VAO->getVertexType());

    for (u32 ba_i = 0; ba_i < VBuffer.Data.size(); ba_i++) {
        if (ba_i > 0)
            new_mesh->addSubData(VBuffer.Data.at(ba_i)->count());
        memcpy(new_mesh->VBuffer.Data.at(ba_i)->data(), VBuffer.Data.at(ba_i)->data(), VBuffer.Data.at(ba_i)->bytesCount());
    }
    if (hasIBO()) {
        for (u32 ba_i = 0; ba_i < IBuffer.Data.size(); ba_i++) {
            if (ba_i > 0)
                new_mesh->addSubData(IBuffer.Data.at(ba_i)->count(), false);
            memcpy(new_mesh->IBuffer.Data.at(ba_i)->data(), IBuffer.Data.at(ba_i)->data(), IBuffer.Data.at(ba_i)->bytesCount());
        }
    }

    new_mesh->uploadData();
    new_mesh->setBoundingBox(getBoundingBox());

    return new_mesh;
}

void MeshBuffer::initData(u32 vertexCount, u32 indexCount)
{
    addSubData(vertexCount);

    if (hasIBO())
        addSubData(indexCount, false);
}

u8 *MeshBuffer::mergeByteArrays(bool vBuffer) const
{
    auto &buffer = vBuffer ? VBuffer : IBuffer;
    u32 totalBytes = 0;

    for (auto &ba : buffer.Data)
        totalBytes += ba->bytesCount();

    assert(totalBytes > 0);
    u8 *totalData = new u8[totalBytes];

    u32 bytesOffset = 0;
    for (auto &ba : buffer.Data) {
        u32 bytesCount = ba->bytesCount();
        memcpy(totalData + bytesOffset, ba->data(), ba->bytesCount());
        bytesOffset += ba->bytesCount();
    }

    return totalData;
}

void MeshBuffer::calcDataIndexAndOffset(u32 dataN, u32 &baIndex, u32 &dataOffset, bool vBuffer) const
{
    u32 startBAI = 0;

    auto &buffer = vBuffer ? VBuffer : IBuffer;
    for (auto &ba : buffer.Data) {
        if (startBAI + ba->count() <= dataN) {
            startBAI += ba->count();
            baIndex++;
        }
        else
            break;
    }

    dataOffset = dataN - startBAI;
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
