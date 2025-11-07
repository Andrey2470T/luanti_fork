#pragma once

#include "meshbuffer.h"

// Node vertex type (position, color, normal, uv, materialType)
extern const render::VertexTypeDescriptor NodeVType;
// Node vertex type with the second (hardware) color (position, color, normal, uv, materialType, hw_color)
extern const render::VertexTypeDescriptor TwoColorNodeVType;
// 2D Vertex type (xy position, rgba color, uv)
extern const render::VertexTypeDescriptor VType2D;
// Bone animated object vertex type (position, color, normal, uv, materialType, bones, weights)
extern const render::VertexTypeDescriptor AOVType;
// Skybox (clouds and stars) vertex type with the second (hardware) color (position, color, normal, uv, hw_color)
extern const render::VertexTypeDescriptor SkyboxVType;

// Getters used for DefaultVType and TwoColorVType
v3f svtGetPos(MeshBuffer *buf, u32 num);
v2f svtGetPos2D(MeshBuffer *buf, u32 num);
img::color8 svtGetColor(MeshBuffer *buf, u32 num);
v3f svtGetNormal(MeshBuffer *buf, u32 num);
v2f svtGetUV(MeshBuffer *buf, u32 num);
v3f svtGetUV3D(MeshBuffer *buf, u32 num);
u8 svtGetMType(MeshBuffer *buf, u32 num);
img::color8 svtGetHWColor(MeshBuffer *buf, u32 num);


// Setters used for DefaultVType and TwoColorVType
void svtSetPos(MeshBuffer *buf, const v3f &pos, u32 num);
void svtSetPos2D(MeshBuffer *buf, const v2f &pos, u32 num);
void svtSetColor(MeshBuffer *buf, const img::color8 &c, u32 num);
void svtSetNormal(MeshBuffer *buf, const v3f &normal, u32 num);
void svtSetUV(MeshBuffer *buf, const v2f &uv, u32 num);
void svtSetUV3D(MeshBuffer *buf, const v3f &uv, u32 num);
void svtSetMType(MeshBuffer *buf, u8 mt, u32 num);
void svtSetHWColor(MeshBuffer *buf, const img::color8 &hw_c, u32 num);

// Appends the attributes of the standard vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void appendSVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal=v3f(), const v2f &uv=v2f());
// Appends the attributes of the two color vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void appendNVT(
     MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal=v3f(), const v2f &uv=v2f(), u8 matType=0);
// Appends the attributes of the two color vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void appendTCNVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal=v3f(), const v2f &uv=v2f(), u8 matType=0, const img::color8 &hw_c=img::color8());
// Appends the attributes of the standard 2D vertex type in the end of the mesh buffer
// Note: 'buf' already must have a preallocated storage for this new vertex!
void appendVT2D(
    MeshBuffer *buf, const v2f &pos, const img::color8 &c, const v2f &uv=v2f());
void appendAOVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal=v3f(), const v2f &uv=v2f(), u8 matType=0, u64 bones=0, u64 weights=0
);
void appendSBVT(
    MeshBuffer *buf, const v3f &pos, const img::color8 &c,
    const v3f &normal=v3f(), const v2f &uv=v2f(), const img::color8 &hw_c=img::color8()
);

void appendIndex(MeshBuffer *buf, u32 index);
