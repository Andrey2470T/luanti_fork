#version 320 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

layout (std140) uniform mMatrices {
	mat4 worldViewProj;
	mat4 worldView;
	mat4 world;
	mat4 texture0;
};

uniform float mThickness;

out vec2 vUV0;
out vec4 vVertexColor;
out float vFogCoord;

void main()
{
	gl_Position = mMatrices.worldViewProj * vec4(pos, 1.0);
	gl_PointSize = mThickness;

	vec4 TextureCoord0 = vec4(uv.x, uv.y, 1.0, 1.0);
	vUV0 = vec4(mMatrices.texture0 * TextureCoord0).xy;

	vVertexColor = color.bgra;

	vec3 Position = (mMatrices.worldView * vec4(pos, 1.0)).xyz;

	vFogCoord = length(Position);
}



