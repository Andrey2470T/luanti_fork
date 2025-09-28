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

    u8 getUInt8Attr(u32 attrN, u32 vertexN) const;
    s8 getCharAttr(u32 attrN, u32 vertexN) const;
    u16 getUInt16Attr(u32 attrN, u32 vertexN) const;
    s16 getShortAttr(u32 attrN, u32 vertexN) const;
    u32 getUInt32Attr(u32 attrN, u32 vertexN) const;
    s32 getIntAttr(u32 attrN, u32 vertexN) const;
    f32 getFloatAttr(u32 attrN, u32 vertexN) const;
    u64 getUInt64Attr(u32 attrN, u32 vertexN) const;
    s64 getLongIntAttr(u32 attrN, u32 vertexN) const;
    f64 getDoubleAttr(u32 attrN, u32 vertexN) const;

    v2u getV2UAttr(u32 attrN, u32 vertexN) const;
    v2i getV2IAttr(u32 attrN, u32 vertexN) const;
    v2f getV2FAttr(u32 attrN, u32 vertexN) const;

    v3u getV3UAttr(u32 attrN, u32 vertexN) const;
    v3i getV3IAttr(u32 attrN, u32 vertexN) const;
    v3f getV3FAttr(u32 attrN, u32 vertexN) const;
    img::color8 getRGBAttr(u32 attrN, u32 vertexN) const;

    img::color8 getRGBAAttr(u32 attrN, u32 vertexN) const;
    /*template<typename T>
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
                return *(reinterpret_cast<T *>((void *)&(vdata->getUInt8(attrNum)));
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
    }*/

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
    void setCharAttr(s8 value, u32 attrN, u32 vertexN);
    void setUInt16Attr(u16 value, u32 attrN, u32 vertexN);
    void setShortAttr(s16 value, u32 attrN, u32 vertexN);
    void setUInt32Attr(u32 value, u32 attrN, u32 vertexN);
    void setIntAttr(s32 value, u32 attrN, u32 vertexN);
    void setFloatAttr(f32 value, u32 attrN, u32 vertexN);
    void setUInt64Attr(u64 value, u32 attrN, u32 vertexN);
    void setLongIntAttr(s64 value, u32 attrN, u32 vertexN);
    void setDoubleAttr(f64 value, u32 attrN, u32 vertexN);

    void setV2UAttr(const v2u &value, u32 attrN, u32 vertexN);
    void setV2IAttr(const v2i &value, u32 attrN, u32 vertexN);
    void setV2FAttr(const v2f &value, u32 attrN, u32 vertexN);

    void setV3UAttr(const v3u &value, u32 attrN, u32 vertexN);
    void setV3IAttr(const v3i &value, u32 attrN, u32 vertexN);
    void setV3FAttr(const v3f &value, u32 attrN, u32 vertexN);
    void setRGBAttr(const img::color8 &value, u32 attrN, u32 vertexN);

    void setRGBAAttr(const img::color8 &value, u32 attrN, u32 vertexN);

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
    void checkAttr(BasicType requiredType, u8 requiredCmpsCount, u32 attrN) const;
    void update(BasicType requiredType, u8 requiredCmpsCount, u32 attrN, u32 vertexN);

    u64 countElemsBefore(u32 vertexNumber, u32 attrNumber) const;

    bool hasVBO() const
    {
        return Type == MeshBufferType::VERTEX || Type == MeshBufferType::VERTEX_INDEX;
    }
    bool hasIBO() const
    {
        return Type == MeshBufferType::INDEX || Type == MeshBufferType::VERTEX_INDEX;
    }
};
