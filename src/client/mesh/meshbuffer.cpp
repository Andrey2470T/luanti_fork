#include "meshbuffer.h"
#include <assert.h>

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
		BoundingBox.reset(getAttrAt<v3f>(0, 0));

		for (u32 i = 1; i < vcount; ++i)
			BoundingBox.addInternalPoint(getAttrAt<v3f>(0, i));
	} else
		BoundingBox.reset(0, 0, 0);
}

void MeshBuffer::setIndexAt(u32 index, u32 pos)
{
    assert(hasIBO() && IBuffer.Data && pos < IBuffer.Data->count());

    IBuffer.Data->setUInt32(index, pos);

    IBuffer.DataCount = pos+1;
    IBuffer.StartChange = IBuffer.StartChange != std::nullopt ?
        std::min<u32>(VBuffer.StartChange.value(), pos) : pos;
    IBuffer.EndChange = IBuffer.EndChange != std::nullopt ?
        std::max<u32>(IBuffer.EndChange.value(), pos) : pos;
    IBuffer.Dirty = true;
}

void MeshBuffer::reallocateData(u32 vertexCount, u32 indexCount)
{
    auto vType = VAO->getVertexType();
    if (vertexCount == VBuffer.Data->count() / vType.Attributes.size() && indexCount == IBuffer.Data->count())
        return;

    if (hasIBO() && IBuffer.Data && indexCount > 0)
        IBuffer.Data->reallocate(indexCount, sizeof(u32) * indexCount);

    if (Type == MeshBufferType::INDEX)
        return;

    VBuffer.Data->reallocate(vType.Attributes.size() * vertexCount, render::sizeOfVertexType(vType) * vertexCount);
}

void MeshBuffer::uploadData()
{
    if (!VBuffer.Dirty && !IBuffer.Dirty)
        return;

    VAO->reallocate(VBuffer.Data.get(), VBuffer.DataCount, (const u32 *)IBuffer.Data.get(), IBuffer.DataCount);
    VBuffer.Dirty = false;
    IBuffer.Dirty = false;
}

void MeshBuffer::uploadVertexData()
{
    if (Type == MeshBufferType::INDEX || VBuffer.DataCount == 0 || !VBuffer.Dirty)
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
    VBuffer.Data->clear();
    VBuffer.DataCount = 0;
    VBuffer.Dirty = false;

    if (hasIBO() && IBuffer.Data) {
        IBuffer.Data->clear();
        IBuffer.DataCount = 0;
        IBuffer.Dirty = false;
    }
}

MeshBuffer *MeshBuffer::copy() const
{
    MeshBuffer *new_mesh = nullptr;
    if (Type == MeshBufferType::INDEX) new_mesh = new MeshBuffer(VAO);
    else new_mesh = new MeshBuffer(Type != MeshBufferType::VERTEX, getVAO()->getVertexType());

    if (hasVBO())
        memcpy(new_mesh->VBuffer.Data.get()->data(), VBuffer.Data.get()->data(), getVertexCount());
    if (hasIBO())
        memcpy(new_mesh->IBuffer.Data.get()->data(), IBuffer.Data.get()->data(), getIndexCount());

    new_mesh->reallocateData(hasVBO() ? getVertexCount() : 0, hasIBO() ? getIndexCount() : 0);
    new_mesh->setBoundingBox(getBoundingBox());

    return new_mesh;
}

inline u64 MeshBuffer::countElemsBefore(u32 vertexNumber, u32 attrNumber) const
{
    auto vType = VAO->getVertexType();
    assert(hasVBO() && VBuffer.Data && vertexNumber <= VBuffer.Data->count() / vType.Attributes.size());

	auto &attr = VAO->getVertexType().Attributes.at(attrNumber);
	return VertexCmpCount * vertexNumber + attrNumber * attr.Offset;
}
