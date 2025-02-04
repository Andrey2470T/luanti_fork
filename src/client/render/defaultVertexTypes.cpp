#include "defaultVertexTypes.h"

// Vertex type with the second (hardware) color (position, color, normal, uv, hw_color)
const render::VertexTypeDescriptor TwoColorVType{
	"TwoColor3D",
    {{"HWColor", 4, BasicType::UINT8, render::VertexAttribute::DataFormat::Normalized}},
	true,
	true,
	2
};

// Getters used for DefaultVType and TwoColorVType
inline v3f svtGetPos(MeshBuffer *buf, u32 num)
{
	return buf->getAttrAt<v3f>(0, num);
}
inline img::color8 svtGetColor(MeshBuffer *buf, u32 num)
{
	return buf->getAttrAt<img::color8>(1, num);
}
inline v3f svtGetNormal(MeshBuffer *buf, u32 num)
{
	return buf->getAttrAt<v3f>(2, num);
}
inline v2f svtGetUV(MeshBuffer *buf, u32 num)
{
	return buf->getAttrAt<v2f>(3, num);
}
inline img::color8 svtGetHWColor(MeshBuffer *buf, u32 num)
{
	return buf->getAttrAt<img::color8>(4, num);
}


// Setters used for DefaultVType and TwoColorVType
inline void svtSetPos(MeshBuffer *buf, v3f pos, u32 num)
{
	buf->setAttrAt<v3f>(pos, 0, num);
}
inline void svtSetColor(MeshBuffer *buf, img::color8 c, u32 num)
{
	buf->setAttrAt<img::color8>(c, 1, num);
}
inline void svtSetNormal(MeshBuffer *buf, v3f normal, u32 num)
{
	buf->setAttrAt<v3f>(normal, 2, num);
}
inline void svtSetUV(MeshBuffer *buf, v2f uv, u32 num)
{
	buf->setAttrAt<v2f>(uv, 3, num);
}
inline void svtSetHWColor(MeshBuffer *buf, img::color8 hw_c, u32 num)
{
	buf->setAttrAt<img::color8>(hw_c, 4, num);
}


// Appends the attributes of the standard vertex type in the end of the mesh buffer
inline void appendSVT(MeshBuffer *buf, v3f pos, img::color8 c, v3f normal, v2f uv)
{
	buf->setAttrAt<v3f>(pos, 0);
	buf->setAttrAt<img::color8>(c, 1);
	buf->setAttrAt<v3f>(normal, 2);
	buf->setAttrAt<v2f>(uv, 3);
}
