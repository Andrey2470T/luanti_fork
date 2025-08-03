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
	INDEX,
	VERTEX_INDEX
};

class MeshBuffer
{
	MeshBufferType Type;

    struct SubMeshBuffer
    {
        std::unique_ptr<ByteArray> Data;
        // As usually we allocate some data storage at once, 'DataCount' can be less 'Data.size()'
        u32 DataCount;
        bool Dirty = false;

        // Range of changed elements (vertices or indices)
        std::optional<u32> StartChange = std::nullopt;
        std::optional<u32> EndChange = std::nullopt;
    };

    SubMeshBuffer VBuffer;
    SubMeshBuffer IBuffer;

    std::shared_ptr<render::Mesh> VAO;

	aabbf BoundingBox;
    u32 VertexCmpCount; // summary count of the components per the used vertex type
public:
    // Creates either VERTEX or VERTEX_INDEX buffer types
    MeshBuffer(bool createIBO = true, const render::VertexTypeDescriptor &descr=render::DefaultVType,
        render::MeshUsage usage=render::MeshUsage::STATIC)
        : Type(createIBO ? MeshBufferType::VERTEX_INDEX : MeshBufferType::VERTEX),
          VAO(std::make_shared<render::Mesh>(descr, createIBO, usage))
	{
		for (auto &attr : VAO->getVertexType().Attributes)
			VertexCmpCount += attr.ComponentCount;
	}
    // Creates either VERTEX or VERTEX_INDEX buffer types allocating the storage for 'vertexCount' and 'indexCount'
    MeshBuffer(u32 vertexCount, u32 indexCount=0, bool createIBO = true,
        const render::VertexTypeDescriptor &descr=render::DefaultVType,
        render::MeshUsage usage=render::MeshUsage::STATIC)
        : Type(createIBO ? MeshBufferType::VERTEX_INDEX : MeshBufferType::VERTEX),
        VAO(std::make_shared<render::Mesh>(nullptr, vertexCount, nullptr, indexCount, descr, createIBO, usage))
    {
        for (auto &attr : VAO->getVertexType().Attributes)
            VertexCmpCount += attr.ComponentCount;

        reallocateData(vertexCount, indexCount);
    }
    // Creates always INDEX type not creating a new VAO
    MeshBuffer(const std::shared_ptr<render::Mesh> &sharedMesh)
        : Type(MeshBufferType::INDEX), VAO(sharedMesh)
    {
        for (auto &attr : VAO->getVertexType().Attributes)
            VertexCmpCount += attr.ComponentCount;
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

	render::Mesh *getVAO() const
	{
        return VAO.get();
	}

    template<typename T>
    T getAttrAt(u32 attrN, u32 vertexN) const
	{
		u64 attrNum = countElemsBefore(vertexN, attrN);

		auto &vertexAttr = VAO->getVertexType().Attributes.at(attrN);
        const ByteArray *vdata = VBuffer.Data.get();
		switch (vertexAttr.ComponentCount)
		{
		case 1: {
			switch (vertexAttr.ComponentType)
			{
			case BasicType::UINT8:
				return vdata->getUInt8(attrNum);
			case BasicType::CHAR:
				return vdata->getChar(attrNum);
			case BasicType::UINT16:
				return vdata->getUInt16(attrNum);
			case BasicType::SHORT:
				return vdata->getShort(attrNum);
			case BasicType::UINT32:
				return vdata->getUInt32(attrNum);
			case BasicType::INT:
				return vdata->getInt(attrNum);
			case BasicType::FLOAT:
				return vdata->getFloat(attrNum);
			case BasicType::UINT64:
				return vdata->getUInt64(attrNum);
			case BasicType::LONG_INT:
				return vdata->getLongInt(attrNum);
			case BasicType::DOUBLE:
				return vdata->getDouble(attrNum);
			default:
				return 0.0f;
			}
		}
		case 2: {
			switch (vertexAttr.ComponentType)
			{
			case BasicType::UINT32:
				return vdata->getV2U(attrNum);
			case BasicType::INT:
				return vdata->getV2I(attrNum);
			case BasicType::FLOAT:
				return vdata->getV2F(attrNum);
			default:
				return T(0, 0);
			}
		}
		case 3: {
			switch (vertexAttr.ComponentType)
			{
			case BasicType::UINT32:
				return vdata->getV3U(attrNum);
			case BasicType::INT:
				return vdata->getV3I(attrNum);
			case BasicType::FLOAT:
				return vdata->getV3F(attrNum);
            case BasicType::UINT8:
                return img::getColor8(vdata, attrNum, img::PF_RGB8);
			default:
				return T(0, 0, 0);
			}
		}
		case 4: {
			return img::getColor8(vdata, attrNum);
		}
		default:
            assert("MeshBuffer::getAttrAt() Couldn't extract the attribute");
		}
    }

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

    template<typename T>
    void setAttrAt(T value, u32 attrN, u32 vertexN)
	{
        u64 attrNum = countElemsBefore(vertexN, attrN);

		auto &vertexAttr = VAO->getVertexType().Attributes.at(attrN);
        ByteArray *vdata = VBuffer.Data.get();
		switch (vertexAttr.ComponentCount)
		{
		case 1: {
			switch (vertexAttr.ComponentType)
			{
			case BasicType::UINT8:
				vdata->setUInt8(value, attrNum);
				return;
			case BasicType::CHAR:
				vdata->setChar(value, attrNum);
				return;
			case BasicType::UINT16:
				vdata->setUInt16(value, attrNum);
				return;
			case BasicType::SHORT:
				vdata->setShort(value, attrNum);
				return;
			case BasicType::UINT32:
				vdata->setUInt32(value, attrNum);
				return;
			case BasicType::INT:
				vdata->setInt(value, attrNum);
				return;
			case BasicType::FLOAT:
				vdata->setFloat(value, attrNum);
				return;
			case BasicType::UINT64:
				vdata->setUInt64(value, attrNum);
				return;
			case BasicType::LONG_INT:
				vdata->setLongInt(value, attrNum);
				return;
			case BasicType::DOUBLE:
				vdata->setDouble(value, attrNum);
				return;
			default:
				return;
			}
		}
		case 2: {
			switch (vertexAttr.ComponentType)
			{
			case BasicType::UINT32:
				vdata->setV2U(value, attrNum);
				return;
			case BasicType::INT:
				vdata->setV2I(value, attrNum);
				return;
			case BasicType::FLOAT:
				vdata->setV2F(value, attrNum);
				return;
			default:
				return;
			}
		}
		case 3: {
			switch (vertexAttr.ComponentType)
			{
			case BasicType::UINT32:
				vdata->setV3U(value, attrNum);
				return;
			case BasicType::INT:
				vdata->setV3I(value, attrNum);
				return;
			case BasicType::FLOAT:
				vdata->setV3F(value, attrNum);
				return;
            case BasicType::UINT8:
                img::setColor8(vdata, value, attrNum, img::PF_RGB8);
                return;
			default:
				return;
			}
		}
		case 4: {
			img::setColor8(vdata, value, attrNum);
			return;
		}
		default:
			return;
		}

        VBuffer.DataCount = std::max(VBuffer.DataCount, vertexN);
        VBuffer.StartChange = VBuffer.StartChange != std::nullopt ?
            std::min(VBuffer.StartChange.value(), vertexN) : vertexN;
        VBuffer.EndChange = VBuffer.EndChange != std::nullopt ?
            std::max(VBuffer.EndChange.value(), vertexN) : vertexN;
        VBuffer.Dirty = true;
    }

    void setIndexAt(u32 index, u32 pos);

    void reallocateData(u32 vertexCount, u32 indexCount=0);

    void uploadData();
    void uploadVertexData();
    void uploadIndexData();

    void clear();

	MeshBuffer *copy() const;

    bool operator==(const MeshBuffer *other_buffer) const
    {
        return VAO.get() == other_buffer->getVAO();
    }
private:
    inline u64 countElemsBefore(u32 vertexNumber, u32 attrNumber) const;

    inline bool hasVBO() const
    {
        return Type == MeshBufferType::VERTEX || Type == MeshBufferType::VERTEX_INDEX;
    }
    inline bool hasIBO() const
    {
        return Type == MeshBufferType::INDEX || Type == MeshBufferType::VERTEX_INDEX;
    }
};
