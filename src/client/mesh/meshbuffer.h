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
        std::vector<std::unique_ptr<ByteArray>> Data;
        // 'DataCount' is a count of vertices/indices
        // As usually we allocate some data storage at once, 'DataCount' can be less 'Data.at(baIndex)->count()'
        u32 DataCount = 0;
        bool Dirty = false;

        // Range of changed elements (vertices or indices)
        std::optional<u32> StartChange = std::nullopt;
        std::optional<u32> EndChange = std::nullopt;
    };

    SubMeshBuffer VBuffer;
    SubMeshBuffer IBuffer;

    std::shared_ptr<render::Mesh> VAO;

	aabbf BoundingBox;
public:
    // Creates either VERTEX or VERTEX_INDEX buffer types
    MeshBuffer(bool createIBO = true, const render::VertexTypeDescriptor &descr=render::DefaultVType,
        render::MeshUsage usage=render::MeshUsage::STATIC, bool uploadData=true)
        : Type(createIBO ? MeshBufferType::VERTEX_INDEX : MeshBufferType::VERTEX)
	{
        if (uploadData)
            VAO = std::make_shared<render::Mesh>(descr, createIBO, usage);
        else
            VAO = std::make_shared<render::Mesh>(descr, usage);
        initData(0, 0);
	}
    // Creates either VERTEX or VERTEX_INDEX buffer types allocating the storage for 'vertexCount' and 'indexCount'
    MeshBuffer(u32 vertexCount, u32 indexCount=0, bool createIBO = true,
        const render::VertexTypeDescriptor &descr=render::DefaultVType,
        render::MeshUsage usage=render::MeshUsage::STATIC, bool uploadData=true)
        : Type(createIBO ? MeshBufferType::VERTEX_INDEX : MeshBufferType::VERTEX)
    {
        if (uploadData)
            VAO = std::make_shared<render::Mesh>(nullptr, vertexCount, nullptr, indexCount, descr, createIBO, usage);
        else
            VAO = std::make_shared<render::Mesh>(descr, usage);
        initData(vertexCount, indexCount);
    }

	MeshBufferType getType() const
	{
        return Type;
	}

    u32 getVertexCount() const
    {
        return VBuffer.DataCount;
    }
    u32 getIndexCount() const
    {
        return IBuffer.DataCount;
    }
    u32 getSubDataCount(u32 baIndex, bool vBuffer=true) const;

	render::Mesh *getVAO() const
	{
        return VAO.get();
	}

    u8 getUInt8Attr(u32 attrN, u32 vertexN) const;
    u16 getUInt16Attr(u32 attrN, u32 vertexN) const;
    u32 getUInt32Attr(u32 attrN, u32 vertexN) const;
    f32 getFloatAttr(u32 attrN, u32 vertexN) const;
    v2f getV2FAttr(u32 attrN, u32 vertexN) const;
    v3f getV3FAttr(u32 attrN, u32 vertexN) const;
    matrix4 getM4x4Attr(u32 attrN, u32 vertexN) const;
    img::color8 getRGBAttr(u32 attrN, u32 vertexN) const;
    img::color8 getRGBAAttr(u32 attrN, u32 vertexN) const;
    img::colorf getRGBAFAttr(u32 attrN, u32 vertexN) const;

    u32 getIndexAt(u32 pos) const;

	const aabbf &getBoundingBox() const
	{
		return BoundingBox;
	}

	void setBoundingBox(const aabbf &newBox)
	{
		BoundingBox = newBox;
	}

	void recalculateBoundingBox();

    void setUInt8Attr(u8 value, u32 attrN, u32 vertexN);
    void setUInt16Attr(u16 value, u32 attrN, u32 vertexN);
    void setUInt32Attr(u32 value, u32 attrN, u32 vertexN);
    void setFloatAttr(f32 value, u32 attrN, u32 vertexN);
    void setV2FAttr(const v2f &value, u32 attrN, u32 vertexN);
    void setV3FAttr(const v3f &value, u32 attrN, u32 vertexN);
    void setM4x4Attr(const matrix4 &value, u32 attrN, u32 vertexN);
    void setRGBAAttr(const img::color8 &value, u32 attrN, u32 vertexN);
    void setRGBAFAttr(const img::colorf &value, u32 attrN, u32 vertexN);

    void setIndexAt(u32 index, u32 pos);

    void reallocateData(u32 vertexCount, u32 indexCount=0, u32 baIndex=0);

    void uploadData();
    void uploadVertexData();
    void uploadIndexData();

    void addSubData(u32 count=0, bool vBuffer=true);

    void clear();

	MeshBuffer *copy() const;

    bool hasIBO() const
    {
        return Type == MeshBufferType::VERTEX_INDEX;
    }

    bool operator==(const MeshBuffer *other_buffer) const
    {
        return VAO.get() == other_buffer->getVAO();
    }
private:
    void initData(u32 vertexCount=0, u32 indexCount=0);

    u8 *mergeByteArrays(bool vBuffer=true) const;
    void calcDataIndexAndOffset(u32 dataN, u32 &baIndex, u32 &dataOffset, bool vBuffer=true) const;
    void markDirty(u32 vertexN, bool vBuffer=true);
    void unmarkDirty(bool vBuffer=true);
};
