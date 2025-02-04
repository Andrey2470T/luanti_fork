#pragma once

#include "BasicIncludes.h"
#include "Render/Mesh.h"
#include "Utils/ByteArray.h"
#include "Utils/AABB.h"
#include "Image/Color.h"
#include <memory>
#include <optional>

enum class MeshBufferType
{
	VERTEX,
	INDEX,
	VERTEX_INDEX
};

class MeshBuffer
{
	MeshBufferType Type;

	union {
		struct {
			std::unique_ptr<ByteArray> VData;
			u32 VDataCount;
		} VBuffer;

		struct {
			std::unique_ptr<ByteArray> IData;
			u32 IDataCount;
		} IBuffer;

		struct {
			std::unique_ptr<ByteArray> VData;
			u32 VDataCount;
			std::unique_ptr<ByteArray> IData;
			u32 IDataCount;
		} VIBuffer;
	};

	std::unique_ptr<render::Mesh> VAO;

	aabbf BoundingBox;
	u32 VertexCmpCount;

public:
	MeshBuffer(MeshBufferType type=MeshBufferType::VERTEX_INDEX,
            const render::VertexTypeDescriptor &descr=render::DefaultVType)
		: Type(type), VAO(std::make_unique<render::Mesh>(descr))
	{
		for (auto &attr : VAO->getVertexType().Attributes)
			VertexCmpCount += attr.ComponentCount;
	}

	MeshBufferType getType() const
	{
        return Type;
	}

	u32 getVertexCount() const;
	u32 getIndexCount() const;

	render::Mesh *getVAO() const
	{
        return VAO.get();
	}

	template<typename T>
	T getAttrAt(u32 attrN, u32 vertexN) const
	{
		u64 attrNum = countElemsBefore(vertexN, attrN);

		auto &vertexAttr = VAO->getVertexType().Attributes.at(attrN);
        const ByteArray *vdata = getVertexData().value();
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
			default:
				return T(0, 0, 0);
			}
		}
		case 4: {
			return img::getColor8(vdata, attrNum);
		}
		default:
			return 0;
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
    void setAttrAt(T value, u32 attrN, std::optional<u32> vertexN=std::nullopt)
	{
        u64 attrNum = vertexN ? countElemsBefore(vertexN.value(), attrN)
			: countElemsBefore(getVertexCount(), attrN);

		auto &vertexAttr = VAO->getVertexType().Attributes.at(attrN);
        ByteArray *vdata = getVertexData().value();
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
	}

    void setIndexAt(u32 index, std::optional<u32> pos=std::nullopt);

	void uploadData(MeshBufferType type=MeshBufferType::VERTEX_INDEX);

	MeshBuffer *copy() const;
private:
	std::optional<ByteArray*> getVertexData();
	std::optional<const ByteArray*> getVertexData() const;

	std::optional<ByteArray*> getIndexData();
	std::optional<const ByteArray*> getIndexData() const;

    inline u64 countElemsBefore(u32 vertexNumber, u32 attrNumber) const;
    inline void checkIndexPos(u32 pos) const;

	// This method is used internally only in copy()
	void setData(const void* vdata, u32 vcount, const u32* idata, u32 icount);
};
