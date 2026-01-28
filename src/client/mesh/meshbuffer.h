#pragma once

#include "Render/Mesh.h"
#include "Utils/ByteArray.h"
#include "Utils/AABB.h"
#include "Image/Color.h"
#include <memory>
#include <optional>
#include <assert.h>

enum class MeshBufferType
{
	VERTEX,
	VERTEX_INDEX
};

class MeshBuffer
{
	MeshBufferType Type;

    struct SubMeshBuffer
    {
        std::unique_ptr<ByteArray> Data;
        bool Dirty = false;

        std::pair<s32, s32> DirtyRange = {-1, -1};
    };

    SubMeshBuffer VBuffer;
    SubMeshBuffer IBuffer;

    render::VertexTypeDescriptor Descriptor;

    std::shared_ptr<render::Mesh> VAO;

	aabbf BoundingBox;
public:
    // Creates either VERTEX or VERTEX_INDEX buffer types
    MeshBuffer(bool createIBO = true, const render::VertexTypeDescriptor &descr=render::DefaultVType,
        render::MeshUsage usage=render::MeshUsage::STATIC, bool deferUpload=false);
    // Creates either VERTEX or VERTEX_INDEX buffer types allocating the storage for 'vertexCount' and 'indexCount'
    MeshBuffer(u32 vertexCount, u32 indexCount=0, bool createIBO = true,
        const render::VertexTypeDescriptor &descr=render::DefaultVType,
        render::MeshUsage usage=render::MeshUsage::STATIC, bool deferUpload=false);

	MeshBufferType getType() const
	{
        return Type;
	}

    render::VertexTypeDescriptor getVertexType() const
    {
        return Descriptor;
    }

    u32 getVertexCount() const
    {
        return VBuffer.Data->count();
    }
    u32 getIndexCount() const
    {
        return hasIBO() ? IBuffer.Data->count() : 0;
    }

	render::Mesh *getVAO() const
	{
        return VAO ? VAO.get() : nullptr;
	}

    u8 getUInt8Attr(u32 attrN, u32 vertexN) const;
    u16 getUInt16Attr(u32 attrN, u32 vertexN) const;
    u32 getUInt32Attr(u32 attrN, u32 vertexNN) const;
    f32 getFloatAttr(u32 attrN, u32 vertexNN) const;
    v2f getV2FAttr(u32 attrN, u32 vertexN) const;
    v3f getV3FAttr(u32 attrN, u32 vertexN) const;
    matrix4 getM4x4Attr(u32 attrN, u32 vertexN) const;
    img::color8 getRGBAttr(u32 attrN, u32 vertexN) const;
    img::color8 getRGBAAttr(u32 attrN, u32 vertexN) const;
    img::colorf getRGBAFAttr(u32 attrN, u32 vertexN) const;

    u32 getIndexAt(u32 pos) const;

    u32 getVertexOffset() const
    {
        return VBuffer.Data->getElemsSetIndex();
    }
    u32 getIndexOffset() const
    {
        return hasIBO() ? IBuffer.Data->getElemsSetIndex() : 0;
    }

	const aabbf &getBoundingBox() const
	{
		return BoundingBox;
	}

	void setBoundingBox(const aabbf &newBox)
	{
		BoundingBox = newBox;
	}

    void setDirtyRange(s32 startN, s32 endN, bool vBuffer=true);

    void setVertexOffset(u32 offset)
    {
        VBuffer.Data->setElemsSetIndex(offset);
    }
    void setIndexOffset(u32 offset)
    {
        if (hasIBO())
            IBuffer.Data->setElemsSetIndex(offset);
    }

	void recalculateBoundingBox();

    void setUInt8Attr(u8 value, u32 attrN, s32 vertexN=-1);
    void setUInt16Attr(u16 value, u32 attrN, s32 vertexN=-1);
    void setUInt32Attr(u32 value, u32 attrN, s32 vertexN=-1);
    void setFloatAttr(f32 value, u32 attrN, s32 vertexN=-1);
    void setV2FAttr(const v2f &value, u32 attrN, s32 vertexN=-1);
    void setV3FAttr(const v3f &value, u32 attrN, s32 vertexN=-1);
    void setM4x4Attr(const matrix4 &value, u32 attrN, s32 vertexN=-1);
    void setRGBAAttr(const img::color8 &value, u32 attrN, s32 vertexN=-1);
    void setRGBAFAttr(const img::colorf &value, u32 attrN, s32 vertexN=-1);

    void setIndexAt(u32 index, s32 pos=-1);

    void reallocateData(u32 vertexCount, u32 indexCount=0);

    void uploadData();

    void flush();

    void clear();

	MeshBuffer *copy() const;

    bool hasIBO() const
    {
        return Type == MeshBufferType::VERTEX_INDEX;
    }

    bool operator==(const MeshBuffer *other_buffer) const
    {
        return VAO ? (VAO.get() == other_buffer->getVAO()) : false;
    }
private:
    void initData(u32 vertexCount=0, u32 indexCount=0);
    void createSubMeshBuffer(u32 count, bool vBuffer=true);
};
