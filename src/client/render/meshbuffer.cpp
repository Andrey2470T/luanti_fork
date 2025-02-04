#include "meshbuffer.h"
#include <assert.h>
#include <string.h>

u32 MeshBuffer::getVertexCount() const
{
	switch (Type) {
		case MeshBufferType::VERTEX:
			return VBuffer.VDataCount;
		case MeshBufferType::VERTEX_INDEX:
			return VIBuffer.VDataCount;
		default:
			return 0;
	};
}

u32 MeshBuffer::getIndexCount() const
{
	switch (Type) {
		case MeshBufferType::INDEX:
			return IBuffer.IDataCount;
		case MeshBufferType::VERTEX_INDEX:
			return VIBuffer.IDataCount;
		default:
			return 0;
	};
}

u32 MeshBuffer::getIndexAt(u32 pos) const
{
    checkIndexPos(pos);

    return getIndexData().value()->getUInt32(pos);
}

void MeshBuffer::recalculateBoundingBox()
{
	u32 vcount = getVertexCount();
	if (vcount) {
		BoundingBox.reset(getAttrAt<v3f>(0, 0));

		for (u32 i = 1; i < vcount; ++i)
			BoundingBox.addInternalPoint(getAttrAt<v3f>(0, i));
	} else
		BoundingBox.reset(0, 0, 0);
}

void MeshBuffer::setIndexAt(u32 index, std::optional<u32> pos)
{
	if (pos)
		checkIndexPos(pos.value());

    getIndexData().value()->setUInt32(index, pos);
}

void MeshBuffer::uploadData(MeshBufferType type)
{
    auto vdata = getVertexData();
    auto idata = getIndexData();

	u32 vcount = getVertexCount();
	u32 icount = getIndexCount();

	if (type == MeshBufferType::VERTEX)
		VAO->uploadData(vdata.value()->data(), vcount,
			nullptr, 0);
	else if (type == MeshBufferType::INDEX)
		VAO->uploadData(nullptr, 0,
            reinterpret_cast<const u32*>(idata.value()->data()), icount);
	else
		VAO->uploadData(vdata.value()->data(), vcount,
            reinterpret_cast<const u32*>(idata.value()->data()), icount);
}

MeshBuffer *MeshBuffer::copy() const
{
	MeshBuffer *new_mesh = new MeshBuffer(getType(), getVAO()->getVertexType());

	new_mesh->setData(getVertexData().value()->data(), getVertexCount(),
        reinterpret_cast<const u32*>(getIndexData().value()->data()), getIndexCount());
	new_mesh->setBoundingBox(getBoundingBox());
	new_mesh->uploadData();
}

std::optional<ByteArray*> MeshBuffer::getVertexData()
{
	switch (Type) {
		case MeshBufferType::VERTEX:
			return VBuffer.VData.get();
		case MeshBufferType::VERTEX_INDEX:
			return VIBuffer.VData.get();
		default:
			return std::nullopt;
	};
}

std::optional<const ByteArray*> MeshBuffer::getVertexData() const
{
	switch (Type) {
		case MeshBufferType::VERTEX:
			return const_cast<const ByteArray*>(VBuffer.VData.get());
		case MeshBufferType::VERTEX_INDEX:
			return const_cast<const ByteArray*>(VIBuffer.VData.get());
		default:
			return std::nullopt;
	};
}

std::optional<ByteArray*> MeshBuffer::getIndexData()
{
	switch (Type) {
		case MeshBufferType::INDEX:
			return IBuffer.IData.get();
		case MeshBufferType::VERTEX_INDEX:
			return VIBuffer.IData.get();
		default:
			return std::nullopt;
	};
}
std::optional<const ByteArray*> MeshBuffer::getIndexData() const
{
	switch (Type) {
		case MeshBufferType::INDEX:
			return const_cast<const ByteArray*>(IBuffer.IData.get());
		case MeshBufferType::VERTEX_INDEX:
			return const_cast<const ByteArray*>(VIBuffer.IData.get());
		default:
			return std::nullopt;
	};
}

inline u64 MeshBuffer::countElemsBefore(u32 vertexNumber, u32 attrNumber) const
{
    auto vdata = getVertexData();

    assert(vdata != std::nullopt);

	u32 vcount = getVertexCount();

    assert(vertexNumber <= vcount);
	auto &attr = VAO->getVertexType().Attributes.at(attrNumber);
	return VertexCmpCount * vertexNumber + attrNumber * attr.Offset;
}

inline void MeshBuffer::checkIndexPos(u32 pos) const
{
    auto idata = getIndexData();

    assert(idata != std::nullopt);

	u32 icount = getIndexCount();

    assert(pos < icount);
}

// This method is used internally only in copy()
void MeshBuffer::setData(const void* vdata, u32 vcount, const u32* idata, u32 icount)
{
	switch (Type) {
		case MeshBufferType::VERTEX:
            memcpy(VBuffer.VData.get()->data(), vdata, vcount);
			break;
		case MeshBufferType::INDEX:
            memcpy(IBuffer.IData.get()->data(), idata, icount);
			break;
		case MeshBufferType::VERTEX_INDEX:
            memcpy(VBuffer.VData.get()->data(), vdata, vcount);
            memcpy(IBuffer.IData.get()->data(), idata, icount);
			break;
	}
}
