#pragma once

#include "meshbuffer.h"

// Vertex type with the second (hardware) color (position, color, normal, uv, hw_color)
extern const render::VertexTypeDescriptor TwoColorVType;

// Getters used for DefaultVType and TwoColorVType
inline v3f svtGetPos(MeshBuffer *buf, u32 num);
inline img::color8 svtGetColor(MeshBuffer *buf, u32 num);
inline v3f svtGetNormal(MeshBuffer *buf, u32 num);
inline v2f svtGetUV(MeshBuffer *buf, u32 num);
inline img::color8 svtGetHWColor(MeshBuffer *buf, u32 num);


// Setters used for DefaultVType and TwoColorVType
inline void svtSetPos(MeshBuffer *buf, v3f pos, u32 num);
inline void svtSetColor(MeshBuffer *buf, img::color8 c, u32 num);
inline void svtSetNormal(MeshBuffer *buf, v3f normal, u32 num);
inline void svtSetUV(MeshBuffer *buf, v2f uv, u32 num);
inline void svtSetHWColor(MeshBuffer *buf, img::color8 hw_c, u32 num);

// Appends the attributes of the standard vertex type in the end of the mesh buffer
inline void appendSVT(MeshBuffer *buf, v3f pos, img::color8 c, v3f normal, v2f uv);
