layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;
layout (location = 4) in vec4 hwcolor;

#include <matrices>

out lowp vec4 vColor;
out highp vec3 vEyeVec;

void main(void)
{
	gl_Position = Matrices.worldViewProj * vec4(pos, 1.0);

	vColor = color;

	vEyeVec = -(Matrices.worldView * vec4(pos, 1.0)).xyz;
}
