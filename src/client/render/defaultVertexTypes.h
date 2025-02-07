#pragma once

#include "meshbuffer.h"

// Vertex type with the second (hardware) color (position, color, normal, uv, hw_color)
extern const render::VertexTypeDescriptor TwoColorVType;

// Getters used for DefaultVType and TwoColorVType
v3f svtGetPos(MeshBuffer *buf, u32 num);
img::color8 svtGetColor(MeshBuffer *buf, u32 num);
v3f svtGetNormal(MeshBuffer *buf, u32 num);
v2f svtGetUV(MeshBuffer *buf, u32 num);
v3f svtGetUV3F(MeshBuffer *buf, u32 num);
img::color8 svtGetHWColor(MeshBuffer *buf, u32 num);


// Setters used for DefaultVType and TwoColorVType
void svtSetPos(MeshBuffer *buf, const v3f &pos, u32 num);
void svtSetColor(MeshBuffer *buf, const img::color8 &c, u32 num);
void svtSetNormal(MeshBuffer *buf, const v3f &normal, u32 num);
void svtSetUV(MeshBuffer *buf, const v2f &uv, u32 num);
void svtSetUV3F(MeshBuffer *buf, const v3f &uv, u32 num);
void svtSetHWColor(MeshBuffer *buf, const img::color8 &hw_c, u32 num);

// Appends the attributes of the standard vertex type in the end of the mesh buffer
void appendSVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal=v3f(), const v2f &uv=v2f());
// Appends the attributes of the two color vertex type in the end of the mesh buffer
void appendTCVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal=v3f(), const v2f &uv=v2f(), const img::color8 &hw_c=img::color8());
