#version 320 core

/* Attributes */

layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;

/* Uniforms */

uniform float thickness;
uniform mat4 projection;

/* Outputs */

out vec2 outUV;
out vec4 outColor;

void main()
{
	gl_Position = projection * pos;
	gl_PointSize = thickness;
	outUV = uv;
	outColor = color.bgra;
}
