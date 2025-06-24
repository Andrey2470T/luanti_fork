#version 320 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

uniform mat4 uWVPMatrix;
uniform mat4 uWVMatrix;
uniform mat4 uTMatrix0;

uniform float uThickness;

out vec2 vTextureCoord0;
out vec4 vVertexColor;
out float vFogCoord;

void main()
{
	gl_Position = uWVPMatrix * vec4(pos, 1.0);
	gl_PointSize = uThickness;

	vec4 TextureCoord0 = vec4(uv.x, uv.y, 1.0, 1.0);
	vTextureCoord0 = vec4(uTMatrix0 * TextureCoord0).xy;

	vVertexColor = color.bgra;

	vec3 Position = (uWVMatrix * vec4(pos, 1.0)).xyz;

	vFogCoord = length(Position);
}



