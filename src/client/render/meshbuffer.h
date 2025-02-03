#pragma once

#include "BasicIncludes.h"
#include "Render/Mesh.h"
#include "Utils/ByteArray.h"
#include "Utils/AABB.h"
#include <unique_ptr>
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

	std::unique_ptr<render::Mesh> GPUBuffer;

	aabbf BoundingBox;
	u32 VertexCmpCount;

public:
	MeshBuffer(MeshBufferType type=MeshBufferType::VERTEX_INDEX,
		const VertexTypeDescriptor &descr=DefaultVType)
		: Type(type), std::make_unique<GPUBuffer>(descr)
	{
		for (auto &attr : GPUBuffer->getVertexType().Attributes)
			VertexCmpCount += attr.ComponentCount;
	}

	MeshBufferType getType() const
	{
		return MeshBufferType;
	}

	std::optional<ByteArray*> getVertexData();
	std::optional<const ByteArray*> getVertexData() const;

	std::optional<ByteArray*> getIndexData();
	std::optional<const ByteArray*> getIndexData() const;

	u32 getVertexCount() const;
	u32 getIndexCount() const;

	template<typename T>
	T getAttrAt(u32 attrN, u32 vertexN) const
	{
		u64 attrNum = countElemsBefore(vertexN, attrN);

		auto &vertexAttr = GPUBuffer->getVertexType().Attributes.at(attrN);
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
	void setAttrAt(T value, u32 attrN, std::optional<u32> vertexN)
	{
		u64 attrNum = vertexN ? countElemsBefore(vertexN, attrN)
			: countElemsBefore(getVertexCount(), attrN);

		auto &vertexAttr = GPUBuffer->getVertexType().Attributes.at(attrN);
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
		default:
			return;
		}
	}

	void setIndexAt(u32 index, u32 pos);
private:
	inline u64 countElemsBefore(u32 vertexNumber, u32 attrNumber);
	inline void checkIndexPos(u32 pos);
};
