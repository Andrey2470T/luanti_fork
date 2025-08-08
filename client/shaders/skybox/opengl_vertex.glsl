layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

#include <matrices>

uniform lowp vec4 materialColor;

out lowp vec4 vColor;
out highp vec3 vEyeVec;

void main(void)
{
	gl_Position = mMatrices.worldViewProj * vec4(pos, 1.0);

	vec4 col = color;

	col *= materialColor;
	vColor = col;

	vEyeVec = -(mMatrices.worldView * pos).xyz;
}
