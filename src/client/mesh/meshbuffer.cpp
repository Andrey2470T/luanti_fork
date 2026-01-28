#include "meshbuffer.h"
#include <assert.h>

MeshBuffer::MeshBuffer(bool createIBO, const render::VertexTypeDescriptor &descr,
    render::MeshUsage usage, bool deferUpload)
    : Type(createIBO ? MeshBufferType::VERTEX_INDEX : MeshBufferType::VERTEX)
{
    if (!deferUpload)
        VAO = std::make_shared<render::Mesh>(descr, createIBO, usage);
    initData(0, 0);
}
// Creates either VERTEX or VERTEX_INDEX buffer types allocating the storage for 'vertexCount' and 'indexCount'
MeshBuffer::MeshBuffer(u32 vertexCount, u32 indexCount, bool createIBO,
    const render::VertexTypeDescriptor &descr,
    render::MeshUsage usage, bool deferUpload)
    : Type(createIBO ? MeshBufferType::VERTEX_INDEX : MeshBufferType::VERTEX)
{
    if (!deferUpload)
        VAO = std::make_shared<render::Mesh>(vertexCount, indexCount, descr, createIBO, usage);
    initData(vertexCount, indexCount);
}


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

void MeshBuffer::setDirtyRange(s32 startN, s32 endN, bool vBuffer)
{
    if (vBuffer) {
        VBuffer.DirtyRange.first = startN;
        VBuffer.DirtyRange.second = endN;
    }
    else if (hasIBO()) {
        IBuffer.DirtyRange.first = startN;
        IBuffer.DirtyRange.second = endN;
    }
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

void MeshBuffer::setUInt8Attr(u8 value, u32 attrN, s32 vertexN)
{
    VBuffer.Data->setUInt8(value, attrN, vertexN);
    VBuffer.Dirty = true;
}
void MeshBuffer::setUInt16Attr(u16 value, u32 attrN, s32 vertexN)
{
    VBuffer.Data->setUInt16(value, attrN, vertexN);
    VBuffer.Dirty = true;
}
void MeshBuffer::setUInt32Attr(u32 value, u32 attrN, s32 vertexN)
{
    VBuffer.Data->setUInt32(value, attrN, vertexN);
    VBuffer.Dirty = true;
}
void MeshBuffer::setFloatAttr(f32 value, u32 attrN, s32 vertexN)
{
    VBuffer.Data->setFloat(value, attrN, vertexN);
    VBuffer.Dirty = true;
}
void MeshBuffer::setV2FAttr(const v2f &value, u32 attrN, s32 vertexN)
{
    VBuffer.Data->setV2F(value, attrN, vertexN);
    VBuffer.Dirty = true;
}
void MeshBuffer::setV3FAttr(const v3f &value, u32 attrN, s32 vertexN)
{
    VBuffer.Data->setV3F(value, attrN, vertexN);
    VBuffer.Dirty = true;
}
void MeshBuffer::setM4x4Attr(const matrix4 &value, u32 attrN, s32 vertexN)
{
    VBuffer.Data->setM4x4(value, attrN, vertexN);
    VBuffer.Dirty = true;
}
void MeshBuffer::setRGBAAttr(const img::color8 &value, u32 attrN, s32 vertexN)
{
    VBuffer.Data->setColor8(value, attrN, vertexN);
    VBuffer.Dirty = true;
}
void MeshBuffer::setRGBAFAttr(const img::colorf &value, u32 attrN, s32 vertexN)
{
    VBuffer.Data->setColorf(value, attrN, vertexN);
    VBuffer.Dirty = true;
}
void MeshBuffer::setIndexAt(u32 index, s32 pos)
{
    assert(hasIBO());

    IBuffer.Data->setUInt32(index, 0, pos);
    IBuffer.Dirty = true;
}

void MeshBuffer::reallocateData(u32 vertexCount, u32 indexCount)
{
    bool vbuffer_modified = vertexCount != VBuffer.Data->count();
    bool ibuffer_modified = hasIBO() && indexCount != IBuffer.Data->count();
    if (vbuffer_modified)
        VBuffer.Data->reallocate(vertexCount);

    if (ibuffer_modified)
        IBuffer.Data->reallocate(indexCount);

    if (VAO && (vbuffer_modified || ibuffer_modified))
        VAO->reallocate(vertexCount, hasIBO() ? indexCount : 0);
}

void MeshBuffer::uploadData()
{
    if (!VAO)
        return;
    if (VBuffer.Dirty) {
        u32 startV = VBuffer.DirtyRange.first != -1 ? VBuffer.DirtyRange.first : 0;
        u32 endV = VBuffer.DirtyRange.second != -1 ? VBuffer.DirtyRange.second : VBuffer.Data->count();
        VAO->uploadVertexData(VBuffer.Data->data(), endV - startV, startV);

        VBuffer.Dirty = false;
        VBuffer.DirtyRange = {-1, -1};
    }

    if (hasIBO() && IBuffer.Dirty) {
        u32 startI = IBuffer.DirtyRange.first != -1 ? IBuffer.DirtyRange.first : 0;
        u32 endI = IBuffer.DirtyRange.second != -1 ? IBuffer.DirtyRange.second : IBuffer.Data->count();
        VAO->uploadIndexData((u32 *)IBuffer.Data->data(), endI - startI, startI);

        IBuffer.Dirty = false;
        IBuffer.DirtyRange = {-1, -1};
    }
}

void MeshBuffer::flush()
{
    if (VAO)
        return;

    VAO = std::make_shared<render::Mesh>(
        VBuffer.Data->count(), hasIBO() ? IBuffer.Data->count() : 0, Descriptor, hasIBO());
    uploadData();
}

void MeshBuffer::clear()
{
    VBuffer.Data->clear();
    VBuffer.Dirty = false;
    VBuffer.DirtyRange = {-1, -1};

    IBuffer.Data->clear();
    IBuffer.Dirty = false;
    IBuffer.DirtyRange = {-1, -1};
}

MeshBuffer *MeshBuffer::copy() const
{
    MeshBuffer *new_mesh = new MeshBuffer(
        VBuffer.Data->count(), hasIBO() ? IBuffer.Data->count() : 0,
        Type != MeshBufferType::VERTEX, Descriptor);

    memcpy(new_mesh->VBuffer.Data->data(), VBuffer.Data->data(), VBuffer.Data->bytesCount());

    if (hasIBO())
        memcpy(new_mesh->IBuffer.Data->data(), IBuffer.Data->data(), IBuffer.Data->bytesCount());

    new_mesh->uploadData();
    new_mesh->setBoundingBox(getBoundingBox());

    return new_mesh;
}

void MeshBuffer::initData(u32 vertexCount, u32 indexCount)
{
    createSubMeshBuffer(vertexCount);

    if (hasIBO())
        createSubMeshBuffer(indexCount, false);
}

void MeshBuffer::createSubMeshBuffer(u32 count, bool vBuffer)
{
    if (vBuffer) {
        // Convert VertexTypeDescriptor to ByteArrayDescriptor for the VBuffer
        std::vector<ByteArrayElement> elements;
        elements.resize(Descriptor.Attributes.size());

        for (u32 i = 0; i < elements.size(); i++) {
            auto &elem = elements[i];
            auto &attribute = Descriptor.Attributes[i];

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
        ByteArrayDescriptor vBufferDesc(Descriptor.Name, elements);
        VBuffer.Data = std::make_unique<ByteArray>(vBufferDesc, count);
    }
    else {
        ByteArrayDescriptor iBufferDesc("Index", {{"Index", ByteArrayElementType::U32}});
        IBuffer.Data = std::make_unique<ByteArray>(iBufferDesc, count);
    }
}
