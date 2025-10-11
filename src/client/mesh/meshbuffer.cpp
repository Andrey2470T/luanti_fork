#include "meshbuffer.h"
#include <assert.h>

u8 MeshBuffer::getUInt8Attr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::UINT8, 1, attrN);
    return VBuffer.Data->getUInt8(countElemsBefore(vertexN, attrN));
}
s8 MeshBuffer::getCharAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::CHAR, 1, attrN);
    return VBuffer.Data->getChar(countElemsBefore(vertexN, attrN));
}
u16 MeshBuffer::getUInt16Attr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::UINT16, 1, attrN);
    return VBuffer.Data->getUInt16(countElemsBefore(vertexN, attrN));
}
s16 MeshBuffer::getShortAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::SHORT, 1, attrN);
    return VBuffer.Data->getShort(countElemsBefore(vertexN, attrN));
}
u32 MeshBuffer::getUInt32Attr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::UINT32, 1, attrN);
    return VBuffer.Data->getUInt32(countElemsBefore(vertexN, attrN));
}
s32 MeshBuffer::getIntAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::INT, 1, attrN);
    return VBuffer.Data->getInt(countElemsBefore(vertexN, attrN));
}
f32 MeshBuffer::getFloatAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::FLOAT, 1, attrN);
    return VBuffer.Data->getFloat(countElemsBefore(vertexN, attrN));
}
u64 MeshBuffer::getUInt64Attr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::UINT64, 1, attrN);
    return VBuffer.Data->getUInt64(countElemsBefore(vertexN, attrN));
}
s64 MeshBuffer::getLongIntAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::LONG_INT, 1, attrN);
    return VBuffer.Data->getLongInt(countElemsBefore(vertexN, attrN));
}
f64 MeshBuffer::getDoubleAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::DOUBLE, 1, attrN);
    return VBuffer.Data->getDouble(countElemsBefore(vertexN, attrN));
}

v2u MeshBuffer::getV2UAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::UINT32, 2, attrN);
    return VBuffer.Data->getV2U(countElemsBefore(vertexN, attrN));
}
v2i MeshBuffer::getV2IAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::INT, 2, attrN);
    return VBuffer.Data->getV2I(countElemsBefore(vertexN, attrN));
}
v2f MeshBuffer::getV2FAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::FLOAT, 2, attrN);
    return VBuffer.Data->getV2F(countElemsBefore(vertexN, attrN));
}

v3u MeshBuffer::getV3UAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::UINT32, 3, attrN);
    return VBuffer.Data->getV3U(countElemsBefore(vertexN, attrN));
}
v3i MeshBuffer::getV3IAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::INT, 3, attrN);
    return VBuffer.Data->getV3I(countElemsBefore(vertexN, attrN));
}
v3f MeshBuffer::getV3FAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::FLOAT, 3, attrN);
    return VBuffer.Data->getV3F(countElemsBefore(vertexN, attrN));
}
img::color8 MeshBuffer::getRGBAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::UINT8, 3, attrN);
    return img::getColor8(VBuffer.Data.get(), countElemsBefore(vertexN, attrN), img::PF_RGB8);
}

img::color8 MeshBuffer::getRGBAAttr(u32 attrN, u32 vertexN) const
{
    checkAttr(BasicType::UINT8, 4, attrN);
    return img::getColor8(VBuffer.Data.get(), countElemsBefore(vertexN, attrN));
}

u32 MeshBuffer::getIndexAt(u32 pos) const
{
    assert(hasIBO() && IBuffer.Data && pos < getIndexCount());
    return IBuffer.Data->getUInt32(pos);
}

void MeshBuffer::recalculateBoundingBox()
{
    if (!hasVBO()) return;
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
    update(BasicType::UINT8, 1, attrN, vertexN);
    VBuffer.Data->setUInt8(value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setCharAttr(s8 value, u32 attrN, u32 vertexN)
{
    update(BasicType::CHAR, 1, attrN, vertexN);
    VBuffer.Data->setChar(value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setUInt16Attr(u16 value, u32 attrN, u32 vertexN)
{
    update(BasicType::UINT16, 1, attrN, vertexN);
    VBuffer.Data->setUInt16(value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setShortAttr(s16 value, u32 attrN, u32 vertexN)
{
    update(BasicType::SHORT, 1, attrN, vertexN);
    VBuffer.Data->setShort(value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setUInt32Attr(u32 value, u32 attrN, u32 vertexN)
{
    update(BasicType::UINT32, 1, attrN, vertexN);
    VBuffer.Data->setUInt32(value, countElemsBefore(vertexN, attrN));
}
void MeshBuffer::setIntAttr(s32 value, u32 attrN, u32 vertexN)
{
    update(BasicType::INT, 1, attrN, vertexN);
    VBuffer.Data->setInt(value, countElemsBefore(vertexN, attrN));
}
void MeshBuffer::setFloatAttr(f32 value, u32 attrN, u32 vertexN)
{
    update(BasicType::FLOAT, 1, attrN, vertexN);
    VBuffer.Data->setFloat(value, countElemsBefore(vertexN, attrN));
}
void MeshBuffer::setUInt64Attr(u64 value, u32 attrN, u32 vertexN)
{
    update(BasicType::UINT64, 1, attrN, vertexN);
    VBuffer.Data->setUInt64(value, countElemsBefore(vertexN, attrN));
}
void MeshBuffer::setLongIntAttr(s64 value, u32 attrN, u32 vertexN)
{
    update(BasicType::LONG_INT, 1, attrN, vertexN);
    VBuffer.Data->setLongInt(value, countElemsBefore(vertexN, attrN));
}
void MeshBuffer::setDoubleAttr(f64 value, u32 attrN, u32 vertexN)
{
    update(BasicType::DOUBLE, 1, attrN, vertexN);
    VBuffer.Data->setDouble(value, countElemsBefore(vertexN, attrN));
}


void MeshBuffer::setV2UAttr(const v2u &value, u32 attrN, u32 vertexN)
{
    update(BasicType::UINT32, 2, attrN, vertexN);
    VBuffer.Data->setV2U(value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setV2IAttr(const v2i &value, u32 attrN, u32 vertexN)
{
    update(BasicType::INT, 2, attrN, vertexN);
    VBuffer.Data->setV2I(value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setV2FAttr(const v2f &value, u32 attrN, u32 vertexN)
{
    update(BasicType::FLOAT, 2, attrN, vertexN);
    VBuffer.Data->setV2F(value, countElemsBefore(vertexN, attrN));
}


void MeshBuffer::setV3UAttr(const v3u &value, u32 attrN, u32 vertexN)
{
    update(BasicType::UINT32, 3, attrN, vertexN);
    VBuffer.Data->setV3U(value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setV3IAttr(const v3i &value, u32 attrN, u32 vertexN)
{
    update(BasicType::INT, 3, attrN, vertexN);
    VBuffer.Data->setV3I(value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setV3FAttr(const v3f &value, u32 attrN, u32 vertexN)
{
    update(BasicType::FLOAT, 3, attrN, vertexN);
    VBuffer.Data->setV3F(value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setRGBAttr(const img::color8 &value, u32 attrN, u32 vertexN)
{
    update(BasicType::UINT8, 3, attrN, vertexN);
    img::setColor8(VBuffer.Data.get(), value, countElemsBefore(vertexN, attrN), img::PF_RGB8);
}


void MeshBuffer::setRGBAAttr(const img::color8 &value, u32 attrN, u32 vertexN)
{
    update(BasicType::UINT8, 4, attrN, vertexN);
    img::setColor8(VBuffer.Data.get(), value, countElemsBefore(vertexN, attrN));
}

void MeshBuffer::setIndexAt(u32 index, u32 pos)
{
    assert(hasIBO() && IBuffer.Data && pos < IBuffer.Data->count());

    IBuffer.Data->setUInt32(index, pos);

    IBuffer.DataCount = std::max(IBuffer.DataCount, pos);
    IBuffer.StartChange = IBuffer.StartChange != std::nullopt ?
        std::min(VBuffer.StartChange.value(), pos) : pos;
    IBuffer.EndChange = IBuffer.EndChange != std::nullopt ?
        std::max(IBuffer.EndChange.value(), pos) : pos;
    IBuffer.Dirty = true;
}

void MeshBuffer::reallocateData(u32 vertexCount, u32 indexCount)
{
    auto vType = VAO->getVertexType();

    if (!VBuffer.Data || (hasIBO() && !IBuffer.Data))
        return;

    if (vertexCount == VBuffer.Data->count() / vType.Attributes.size() && indexCount == IBuffer.Data->count())
        return;

    if (hasIBO())
        IBuffer.Data->reallocate(indexCount, sizeof(u32) * indexCount);

    //if (Type == MeshBufferType::INDEX)
    //    return;

    VBuffer.Data->reallocate(vType.Attributes.size() * vertexCount, render::sizeOfVertexType(vType) * vertexCount);
}

void MeshBuffer::uploadData()
{
    if (!VBuffer.Dirty && (hasIBO() && !IBuffer.Dirty))
        return;

    const u32 *iboData = nullptr;
    u32 iboCount = 0;

    if (hasIBO()) {
        iboData = (const u32 *)IBuffer.Data.get();
        iboCount = IBuffer.DataCount;
    }
    VAO->reallocate(VBuffer.Data.get(), VBuffer.DataCount, iboData, iboCount);
    VBuffer.Dirty = false;
    IBuffer.Dirty = false;
}

void MeshBuffer::uploadVertexData()
{
    if (VBuffer.DataCount == 0 || !VBuffer.Dirty)
        return;

    u32 startV = VBuffer.StartChange.value();
    u32 endV = VBuffer.EndChange.value();
    VAO->uploadVertexData(VBuffer.Data.get(), endV - startV + 1, startV);

    VBuffer.StartChange = std::nullopt;
    VBuffer.EndChange = std::nullopt;
    VBuffer.Dirty = false;
}

void MeshBuffer::uploadIndexData()
{
    if (Type == MeshBufferType::VERTEX || IBuffer.DataCount == 0 || !IBuffer.Dirty)
        return;

    u32 startI = IBuffer.StartChange.value();
    u32 endI = IBuffer.EndChange.value();
    VAO->uploadIndexData((const u32 *)IBuffer.Data.get(), endI - startI + 1, startI);

    IBuffer.StartChange = std::nullopt;
    IBuffer.EndChange = std::nullopt;
    IBuffer.Dirty = false;
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
    MeshBuffer *new_mesh = new MeshBuffer(Type != MeshBufferType::VERTEX, getVAO()->getVertexType());

    if (hasVBO())
        memcpy(new_mesh->VBuffer.Data.get()->data(), VBuffer.Data.get()->data(), getVertexCount());
    if (hasIBO())
        memcpy(new_mesh->IBuffer.Data.get()->data(), IBuffer.Data.get()->data(), getIndexCount());

    new_mesh->reallocateData(hasVBO() ? getVertexCount() : 0, hasIBO() ? getIndexCount() : 0);
    new_mesh->setBoundingBox(getBoundingBox());

    return new_mesh;
}

void MeshBuffer::initData(u32 vertexCount, u32 indexCount)
{
    auto vType = VAO->getVertexType();
    if (!VBuffer.Data) {
        VBuffer.Data = std::make_unique<ByteArray>(
            vType.Attributes.size() * vertexCount, render::sizeOfVertexType(vType) * vertexCount);
    }
    if (hasIBO())
        IBuffer.Data = std::make_unique<ByteArray>(indexCount, sizeof(u32) * indexCount);
}

void MeshBuffer::checkAttr(BasicType requiredType, u8 requiredCmpsCount, u32 attrN) const
{
    auto &vertexAttr = VAO->getVertexType().Attributes.at(attrN);
    assert(vertexAttr.ComponentType == requiredType && vertexAttr.ComponentCount == requiredCmpsCount);
}

void MeshBuffer::update(BasicType requiredType, u8 requiredCmpsCount, u32 attrN, u32 vertexN)
{
    checkAttr(requiredType, requiredCmpsCount, attrN);

    VBuffer.DataCount = std::max(VBuffer.DataCount, vertexN);
    VBuffer.StartChange = VBuffer.StartChange != std::nullopt ?
        std::min(VBuffer.StartChange.value(), vertexN) : vertexN;
    VBuffer.EndChange = VBuffer.EndChange != std::nullopt ?
        std::max(VBuffer.EndChange.value(), vertexN) : vertexN;
    VBuffer.Dirty = true;
}

u64 MeshBuffer::countElemsBefore(u32 vertexNumber, u32 attrNumber) const
{
    auto vType = VAO->getVertexType();
    assert(hasVBO() && VBuffer.Data && vertexNumber <= VBuffer.Data->count() / vType.Attributes.size());

	auto &attr = VAO->getVertexType().Attributes.at(attrNumber);
	return VertexCmpCount * vertexNumber + attrNumber * attr.Offset;
}
